/*
   Copyright © 2011 by Mauro Carvalho Chehab <mchehab@redhat.com>

   The get_media_devices is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The libv4l2util Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the libv4l2util Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.
 */

/*
 * Version of the API
 */
#define GET_MEDIA_DEVICES_VERSION	0x0100

/**
 * enum device_type - Enumerates the type for each device
 *
 * The device_type is used to sort the media devices array.
 * So, the order is relevant. The first device should be
 * V4L_VIDEO.
 */
enum device_type {
	UNKNOWN = 65535,
	V4L_VIDEO = 0,
	V4L_VBI,
	DVB_FRONTEND,
	DVB_DEMUX,
	DVB_DVR,
	DVB_NET,
	DVB_CA,
	/* TODO: Add dvb full-featured nodes */
	SND_CARD,
	SND_CAP,
	SND_OUT,
	SND_CONTROL,
	SND_HW,
};

/**
 * struct media_devices - Describes all devices found
 *
 * @device:		sysfs name for a device.
 *			PCI devices are like: pci0000:00/0000:00:1b.0
 *			USB devices are like: pci0000:00/0000:00:1d.7/usb1/1-8
 * @node:		Device node, in sysfs or alsa hw identifier
 * @device_type:	Type of the device (V4L_*, DVB_*, SND_*)
 */
struct media_devices {
	char *device;
	char *node;
	enum device_type type;
};

/**
 * discover_media_devices() - Returns a list of the media devices
 * @md_size:	Returns the size of the media devices found
 *
 * This function reads the /sys/class nodes for V4L, DVB and sound,
 * and returns an ordered list of devices. The devices are ordered
 * by device, type and node. At return, md_size is updated.
 */
struct media_devices *discover_media_devices(unsigned int *md_size);

/**
 * free_media_devices() - Frees the media devices array
 *
 * @md:		media devices array
 * @md_size:	size of the array
 *
 * As discover_media_devices() dynamically allocate space for the
 * strings, feeing the list requires also to free those data. So,
 * the safest and recommended way is to call this function.
 */
void free_media_devices(struct media_devices *md, unsigned int md_size);

/**
 * display_media_devices() - prints a list of media devices
 *
 * @md:		media devices array
 * @size:	size of the list
 */
void display_media_devices(struct media_devices *md, unsigned int size);

/**
 * get_first_alsa_cap_device() - Gets the first alsa capture device for a
 *				 video node
 *
 * @md:		media devices array
 * @size:	size of the list
 * @v4l_device:	name of the video device
 *
 * This function seeks inside the media_devices struct for the first alsa
 * capture device (SND_CAP) that belongs to the same device where the video
 * node exists. The video node should belong to V4L_VIDEO type (video0,
 * video1, etc).
 */
char *get_first_alsa_cap_device(struct media_devices *md, unsigned int size,
				char *v4l_device);

/**
 * get_first_no_video_out_device() - Gets the first alsa playback device
 *				     that is not associated to a video device.
 *
 * @md:		media devices array
 * @size:	size of the list
 * @v4l_device:	name of the video device
 *
 * This function seeks inside the media_devices struct for the first alsa
 * playback device (SND_OUT) that is not associated to a video device.
 * The returned value is at the format expected by alsa libraries (like hw:0,0)
 *
 * If there's no playback device that are not associated to a video device,
 * the routine falls back to any existing audio playback device. This is useful
 * in the cases where there's no audio sound device on a system, and an external
 * USB device that provides both video and audio playback is connected.
 *
 * Note that, as the devices are ordered by the devices ID, this routine
 * may not return hw:0,0. That's OK, as, if the media card is probed before
 * the audio card, the media card will receive the lowest address.
 *
 * In general, it will return the alsa device for the default audio playback
 * device.
 */
char *get_first_no_video_out_device(struct media_devices *md,
				    unsigned int size);

/*
 * A typical usecase for the above API is:
 *
 *	struct media_devices *md;
 *	unsigned int size = 0;
 *	char *alsa_cap, *alsa_out, *p;
 *	char *video_dev = "/dev/video0";
 *
 *	md = discover_media_devices(&size);
 *	p = strrchr(video_dev, '/');
 *	alsa_cap = get_first_alsa_cap_device(md, size, p + 1);
 *	alsa_out = get_first_no_video_out_device(md, size);
 *	if (alsa_cap && alsa_out)
 *		alsa_handler(alsa_out, alsa_cap);
 *	free_media_devices(md, size);
 *
 * Where alsa_handler() is some function that will need to handle
 * both alsa capture and playback devices.
 */
