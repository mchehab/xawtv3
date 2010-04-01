/*
 * (c) 2000-2002 Gerd Knorr <kraxel@goldbach.in-berlin.de>
 *
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>

/* xawtv */
#include "channel.h"
#include "frequencies.h"
#include "grab-ng.h"
#include "commands.h"

/* libvbi */
#include "vt.h"
#include "misc.h"
#include "fdset.h"
#include "vbi.h"
#include "lang.h"
#include "dllist.h"

int debug = 0;
int have_dga = 0;

int timeout = 3;
char xpacket[64];
char header[64];
char scratch[1024*256];

/*---------------------------------------------------------------------*/

static void
grabber_init(void)
{
    drv = ng_vid_open(ng_dev.video,NULL,0,&h_drv);
    if (NULL == drv) {
	fprintf(stderr,"no grabber device available\n");
	exit(1);
    }
    f_drv = drv->capabilities(h_drv);
    add_attrs(drv->list_attrs(h_drv));
}

static void
event(struct dl_head *reqs, struct vt_event *ev)
{
    unsigned char *p;
    struct vt_page *vtp;
    
    switch (ev->type) {
    case EV_HEADER:
	p = ev->p1;
	if (debug)
	    fprintf(stderr,"header %.32s\n", p+8);
	memcpy(header,p+8,32);
	header[32] = 0;
	break;
    case EV_PAGE:
	vtp = ev->p1;
	if (debug)
	    fprintf(stderr,"vtx page %x.%02x  \r", vtp->pgno, vtp->subno);
	break;
    case EV_XPACKET:
	p = ev->p1;
	if (debug)
	    fprintf(stderr,"xpacket %x %x %x %x - %.20s\n",
		    p[0],p[1],p[3],p[5],p+20);
	memcpy(xpacket,p+20,20);
	xpacket[20] = 0;
	break;
    }
}

static char*
get_vbi_name(void)
{
    int start;
    char *name,*h;

    xpacket[0] = 0;
    header[0] = 0;
    start = time(NULL);
    for (;;) {
	if (fdset_select(fds, 1000 * timeout) == 0) {
	    break;
	}
	if (time(NULL) > start+timeout) {
	    break;
	}
	if (xpacket[0] && header[0]) {
	    break;
	}
    }
    if (xpacket[0]) {
	for (h = xpacket+19; h >= xpacket; h--) {
	    if (32 != *h)
		break;
	    *h = 0;
	}
	for (name = xpacket; *name == 32; name++)
	    ;
	return name;
    }
#if 0
    if (header[0]) {
	for (name = header; *name && name < header+20; name++)
	    if (isalpha(*name))
		break;
	for (h = name; *name && name < header+20; name++)
	    if (!isalpha(*h)) {
		*h = 0;
		break;
	    }
	if (*name)
	    return name;
    }
#endif
    return NULL;
}

static int
menu(char *intro, struct STRTAB *tab, char *opt)
{
    int i,ret;
    char line[80];

    if (NULL != opt) {
	/* cmd line option -- non-interactive mode */
	for (i = 0; tab[i].str != NULL; i++)
	    if (0 == strcasecmp(tab[i].str,opt))
		return tab[i].nr;
	fprintf(stderr,"%s: not found\n",opt);
	exit(1);
    }

    fprintf(stderr,"\n%s\n",intro);
    for (i = 0; tab[i].str != NULL; i++)
	fprintf(stderr,"  %2ld: %s\n",tab[i].nr,tab[i].str);

    for (;;) {
	fprintf(stderr,"nr ? ");
	fgets(line,79,stdin);
	ret = atoi(line);
	for (i = 0; tab[i].str != NULL; i++)
	    if (ret == tab[i].nr)
		return ret;
	fprintf(stderr,"invalid choice\n");
    }
}

static void
usage(FILE *out, char *prog, char *outfile)
{
    fprintf(out,
	    "This tool scan for tv stations and writes "
	    "a initial xawtv config file.\n"
	    "usage: %s [ options ]\n"
	    "options:\n"
	    "   -h           print this text\n"
	    "   -o outfile   set output file.        [%s]\n"
	    "   -n norm      set tv norm.\n"
	    "   -f table     set frequency table.\n"
	    "   -c device    set video device file.  [%s]\n"
	    "   -C device    set vbi device file.    [%s]\n"
	    "   -s           skip channel scan\n",
	    prog,
	    outfile ? outfile : "stdout",
	    ng_dev.video,ng_dev.vbi);
}

int
main(int argc, char **argv)
{
    struct vbi *vbi;
    struct ng_attribute *attr;
    int c,f,f1,f2,fc,fi,on,tuned,i,j,scan=1,fullscan=0;
    char *name,dummy[32];
    char *tvnorm  = NULL;
    char *freqtab = NULL;
    char *outfile = NULL;
    FILE *conf = stdout;

    setprgname(argv[0]);

    /* parse options */
    ng_init();
    for (;;) {
	if (-1 == (c = getopt(argc, argv, "hsadn:f:o:c:C:")))
	    break;
	switch (c) {
	case 'd':
	    debug++;
	    break;
	case 's':
	    scan=0;
	    break;
	case 'a':
	    fullscan=1;
	    break;
	case 'n':
	    tvnorm = optarg;
	    break;
	case 'f':
	    freqtab = optarg;
	    break;
	case 'o':
	    outfile = optarg;
	    break;
	case 'c':
	    ng_dev.video = optarg;
	    break;
	case 'C':
	    ng_dev.vbi = optarg;
	    break;
	case 'h':
	    usage(stdout,argv[0],outfile);
	    exit(0);
	default:
	    usage(stderr,argv[0],outfile);
	    exit(1);
	}
    }

    if (outfile) {
	if (NULL == (conf = fopen(outfile,"w"))) {
	    fprintf(stderr,"open %s: %s\n",argv[1],strerror(errno));
	    exit(1);
	}
    }

    /* video */
    if (NULL == drv)
	grabber_init();
    if (NULL == drv) {
	fprintf(stderr,"can't open video device\n");
	exit(1);
    }

    attr_init();
    audio_init();

    /* ask the user some questions ... */
    attr = ng_attr_byid(attrs,ATTR_ID_NORM);
    i = menu("please select your TV norm",attr->choices,tvnorm);
    j = menu("please select a frequency table",chanlist_names,freqtab);

    fprintf(conf,"[global]\n");
    fprintf(conf,"freqtab = %s\n",chanlist_names[j].str);
    fprintf(conf,"\n");
    fprintf(conf,"[defaults]\n");
    fprintf(conf,"input = Television\n");
    fprintf(conf,"norm = %s\n",ng_attr_getstr(attr,i));
    fprintf(conf,"\n");
    fflush(conf);
    if (!scan)
	exit(0);

    if (!(f_drv & CAN_TUNE)) {
	fprintf(stderr,"device has no tuner, exiting\n");
	exit(0);
    }
    set_defaults();
    do_va_cmd(2,"setinput","television");
    do_va_cmd(2,"setnorm",ng_attr_getstr(attr,i));
    do_va_cmd(2,"setfreqtab",chanlist_names[j].str);

    /* vbi */
    fdset_init(fds);
    if (not(vbi = vbi_open(ng_dev.vbi, 0, 0, -1)))
	fatal("cannot open %s", ng_dev.vbi);
    vbi_add_handler(vbi, event, NULL);

    if (!fullscan) {
	/* scan channels */
	fprintf(stderr,"\nscanning channel list %s...\n",
		chanlist_names[j].str);
	for (i = 0; i < chancount; i++) {
	    fprintf(stderr,"%-4s (%6.2f MHz): ",chanlist[i].name,
		    (float)chanlist[i].freq/1000);
	    do_va_cmd(2,"setchannel",chanlist[i].name);
	    usleep(200000); /* 0.2 sec */
	    if (0 == drv->is_tuned(h_drv)) {
		fprintf(stderr,"no station\n");
		continue;
	    }
	    fdset_select(fds, 1000 * timeout);
	    name = get_vbi_name();
	    fprintf(stderr,"%s\n",name ? name : "???");
	    if (NULL == name) {
		sprintf(dummy,"unknown (%s)",chanlist[i].name);
		name = dummy;
	    }
	    fprintf(conf,"[%s]\nchannel = %s\n\n",name,chanlist[i].name);
	    fflush(conf);
	}
    } else {
	/* scan freqnencies */
	fprintf(stderr,"\nscanning freqencies...\n");
	on = 0;
	fc = 0;
	f1 = 0;
	f2 = 0;
	fi = -1;
	for (f = 44*16; f <= 958*16; f += 4) {
	    for (i = 0; i < chancount; i++)
		if (chanlist[i].freq * 16 == f * 1000)
		    break;
	    fprintf(stderr,"?? %6.2f MHz (%-4s): ",f/16.0,
		    (i == chancount) ? "-" : chanlist[i].name);
	    drv->setfreq(h_drv,f);
	    usleep(200000); /* 0.2 sec */
	    tuned = drv->is_tuned(h_drv);

	    /* state machine */
	    if (0 == on && 0 == tuned) {
		fprintf(stderr,"|   no\n");
		continue;
	    }
	    if (0 == on && 0 != tuned) {
		fprintf(stderr," \\  raise\n");
		f1 = f;
		if (i != chancount) {
		    fi = i;
		    fc = f;
		}
		on = 1;
		continue;
	    }
	    if (0 != on && 0 != tuned) {
		fprintf(stderr,"  | yes\n");
		if (i != chancount) {
		    fi = i;
		    fc = f;
		}
		continue;
	    }
	    /* if (on != 0 && 0 == tuned)  --  found one, read name from vbi */
	    fprintf(stderr," /  fall\n");
	    f2 = f;
	    if (0 == fc)
		fc = (f1+f2)/2;

	    fprintf(stderr,"=> %6.2f MHz (%-4s): ", fc/16.0,
		    (-1 != fi) ? chanlist[fi].name : "-");
	    drv->setfreq(h_drv,fc);
	    
	    fdset_select(fds, 1000 * timeout);
	    name = get_vbi_name();
	    fprintf(stderr,"%s\n",name ? name : "???");
	    if (NULL == name) {
		sprintf(dummy,"unknown (%s)",chanlist[fi].name);
		name = dummy;
	    }
	    if (-1 != fi) {
		if (NULL == name) {
		    sprintf(dummy,"unknown (%s)",chanlist[fi].name);
		    name = dummy;
		}
		fprintf(conf,"[%s]\nchannel = %s\n\n",name,chanlist[fi].name);
	    } else {
		if (NULL == name) {
		    sprintf(dummy,"unknown (%.3f)", fc/16.0);
		    name = dummy;
		}
		fprintf(conf,"[%s]\nfreq = %.3f\n\n", name, fc/16.0);
	    }
	    fflush(conf);

	    on = 0;
	    fc = 0;
	    f1 = 0;
	    f2 = 0;
	    fi = -1;
	}
    }

    /* cleanup */
    audio_off();
    drv->close(h_drv);

    vbi_del_handler(vbi, event, NULL);
    vbi_close(vbi);

    exit(0);
}

/*
 * Local variables:
 * compile-command: "(cd ..; make)"
 * End:
 */
