/*
   Copyright © 2011 by Mauro Carvalho Chehab <mchehab@redhat.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <dirent.h>
#include "get_media_devices.h"

static char *device_type_str[] = {
	[UNKNOWN]	= "unknown",
	[V4L_VIDEO]	= "video",
	[V4L_VBI]	= "vbi",
	[DVB_FRONTEND]	= "dvb frontend",
	[DVB_DEMUX]	= "dvb demux",
	[DVB_DVR]	= "dvb dvr",
	[DVB_NET]	= "dvb net",
	[DVB_CA]	= "dvb conditional access",
	[SND_CARD]	= "sound card",
	[SND_CAP]	= "pcm capture",
	[SND_OUT]	= "pcm output",
	[SND_CONTROL]	= "mixer",
	[SND_HW]	= "sound hardware",
};

typedef int (*fill_data_t)(struct media_devices *md);

#define DEVICE_STR "devices"

static int get_class(char *class,
		     struct media_devices **md,
		     unsigned int *md_size,
		     fill_data_t fill)
{
	DIR		*dir;
	struct dirent	*entry;
	char		dname[512];
	char		fname[512];
	char		link[1024];
	int		err = -2;
	struct		media_devices *md_ptr = NULL;
	int		size;
	char		*p, *class_node, *device;

	sprintf(dname, "/sys/class/%s", class);
	dir = opendir(dname);
	if (!dir) {
		perror(dname);
		return 0;
	}
	for (entry = readdir(dir); entry; entry = readdir(dir)) {
		sprintf(fname, "%s/%s", dname, entry->d_name);

		size = readlink(fname, link, sizeof(link));
		if (size > 0) {
			link[size] = '\0';

			/* Keep just the name of the cass node */
			p = strrchr(fname, '/');
			if (!p)
				goto error;
			class_node = p + 1;

			/* Canonicalize the device name */

			/* remove the ../../devices/ from the name */
			p = strstr(link, DEVICE_STR);
			if (!p)
				goto error;
			device = p + sizeof(DEVICE_STR);

			/* Remove the subsystem/class_name from the string */
			p = strstr(link, class);
			if (!p)
				goto error;
			*(p -1) = '\0';

			/* Remove USB sub-devices from the path */
			if (strstr(device,"usb")) {
				do {
					p = strrchr(device, '/');
					if (!p)
						goto error;
					if (!strpbrk(p,":."))
						break;
					*p = '\0';
				} while (1);
			}

			/* Don't handle virtual devices */
			if (!strcmp(device,"virtual"))
				continue;

			/* Add one more element to the devices struct */
			*md = realloc(*md, (*md_size + 1) * sizeof(*md_ptr));
			if (!*md)
				goto error;
			md_ptr = (*md) + *md_size;
			(*md_size)++;

			/* Cleans previous data and fills it with device/node */
			md_ptr->type = UNKNOWN;
			md_ptr->device = malloc(strlen(device) + 1);
			strcpy(md_ptr->device, device);
			md_ptr->node = malloc(strlen(class_node) + 1);
			strcpy(md_ptr->node, class_node);

			/* Used to identify the type of node */
			fill(md_ptr);
		}
	}
	err = 0;
error:
	closedir(dir);
	return err;
}

static int add_v4l_class(struct media_devices *md)
{
	if (strstr(md->node, "video"))
		md->type = V4L_VIDEO;
	else if (strstr(md->node, "vbi"))
		md->type = V4L_VBI;

	return 0;
};

static int add_snd_class(struct media_devices *md)
{
	unsigned c = 65535, d = 65535;
	char *new;

	if (strstr(md->node, "card")) {
		sscanf(md->node, "card%u", &c);
		md->type = SND_CARD;
	} else if (strstr(md->node, "hw")) {
		sscanf(md->node, "hwC%uD%u", &c, &d);
		md->type = SND_HW;
	} else if (strstr(md->node, "control")) {
		sscanf(md->node, "controlC%u", &c);
		md->type = SND_CONTROL;
	} else if (strstr(md->node, "pcm")) {
		sscanf(md->node, "pcmC%uD%u", &c, &d);
		if (md->node[strlen(md->node) - 1] == 'p')
			md->type = SND_OUT;
		else if (md->node[strlen(md->node) - 1] == 'c')
			md->type = SND_CAP;
	}

	if (c == 65535)
		return 0;

	/* Reformat device to be useful for alsa userspace library */
	if (d == 65535) {
		if (asprintf(&new, "hw:%u", c) > 0) {
			free(md->node);
			md->node = new;
		}
		return 0;
	}

	if (asprintf(&new, "hw:%u,%u", c, d) > 0) {
		free(md->node);
		md->node = new;
	}

	return 0;
};

static int add_dvb_class(struct media_devices *md)
{
	if (strstr(md->node, "frontend"))
		md->type = DVB_FRONTEND;
	else if (strstr(md->node, "demux"))
		md->type = DVB_DEMUX;
	else if (strstr(md->node, "dvr"))
		md->type = DVB_DVR;
	else if (strstr(md->node, "net"))
		md->type = DVB_NET;
	else if (strstr(md->node, "ca"))
		md->type = DVB_CA;

	return 0;
};

static int sort_media_devices(const void *a, const void *b)
{
	const struct media_devices *md_a = a;
	const struct media_devices *md_b = b;
	int cmp;

	cmp = strcmp(md_a->device, md_b->device);
	if (cmp)
		return cmp;
	cmp = (int)md_a->type - (int)md_b->type;
	if (cmp)
		return cmp;

	return strcmp(md_a->node, md_b->node);
}


/* Public functions */

struct media_devices *discover_media_devices(unsigned int *md_size)
{
	struct media_devices *md = NULL;

	*md_size = 0;
	if (get_class("video4linux", &md, md_size, add_v4l_class))
		return NULL;
	if (get_class("sound", &md, md_size, add_snd_class))
		return NULL;
	if (get_class("dvb", &md, md_size, add_dvb_class))
		return NULL;

	if (!md)
		return NULL;

	qsort(md, *md_size, sizeof(*md), sort_media_devices);

	return md;
}

void free_media_devices(struct media_devices *md, unsigned int md_size)
{
	struct media_devices *md_ptr = md;
	int i;
	for (i = 0; i < md_size; i++) {
		free(md_ptr->node);
		free(md_ptr->device);
		md_ptr++;
	}
	free(md);
}

void display_media_devices(struct media_devices *md, unsigned int size)
{
	int i;
	char *prev = "";

	for (i = 0; i < size; i++) {
		if (strcmp(prev, md->device)) {
			printf ("\nDevice %s:\n\t", md->device);
			prev = md->device;
		}
		printf ("%s(%s) ", md->node, device_type_str[md->type]);
		md++;
	}
	printf("\n");
}

char *get_first_alsa_cap_device(struct media_devices *md, unsigned int size,
				char *v4l_device)
{
	int i;
	char *prev;

	/* Step 1: Find the V4L node */
	for (i = 0; i < size; i++, md++) {
		if (md->type == V4L_VIDEO) {
			if (!strcmp(v4l_device,  md->node))
				break;
		}
	}
	if (i == size)
		return NULL;

	/* Step 2: find the alsa node */
	prev = md->device;
	for (i++, md++;i < size && !strcmp(prev,md->device); i++, md++) {
		if (md->type == SND_CAP)
			return md->node;
	}

	return NULL;
}

char *get_first_no_video_out_device(struct media_devices *md, unsigned int size)
{
	struct media_devices *md_ptr = md;
	int i, skip = 0;
	char *prev = "";

	/* Step 1: Find a device without video4linux node */
	for (i = 0; i < size; i++, md++) {
		if (md->type == V4L_VIDEO)
			skip = 1;
		else if (strcmp(prev, md->device)) {
			prev = md->device;
			skip = 0;
		}
		if (!skip && md->type == SND_OUT)
			return md->node;
	}

	/*
	 * Step 2: Fallback: Find any alsa out node. Useful if a machine
	 * doesn't have an internal board, but an USB device like the
	 * Sirius webcam also provides an alsa output node
	 */
	md = md_ptr;
	for (i = 0; i < size; i++, md++) {
		if (!skip && md->type == SND_OUT)
			return md->node;
	}

	return NULL;
}
