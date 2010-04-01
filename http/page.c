#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "httpd.h"

/* libvbi */
#include "vt.h"
#include "misc.h"
#include "fdset.h"
#include "vbi.h"
#include "lang.h"
#include "dllist.h"
#include "export.h"

void fmt_page(struct export *e, struct fmt_page *pg, struct vt_page *vtp);

#define BUFSIZE 4096

/* ---------------------------------------------------------------------- */

static char stylesheet[] =
#include "alevt.css.h"
;

static char page_about[] =
#include "about.html.h"
;

static char page_top[] =
#include "top.html.h"
;

static char page_bottom[] =
#include "bottom.html.h"
;

/*                          01234567890123456789012345678901 */
static char graphic1[32] = " ����'������-���.:::.;;;.}/}.�/#";
/*                          012345 6789 0123 456789012345678901 */
static char graphic2[32] = ".:::.\\{{.\\||.\\�#_:::.��[.||].###";

/* ---------------------------------------------------------------------- */

/* fix bcd issues ... */
static int calc_page(int pagenr, int offset)
{
    int result;
    
    result = pagenr + offset;
    if (offset < 0) {
	while ((result & 0x0f) > 0x09)
	    result -= 0x01;
	while ((result & 0xf0) > 0x90)
	    result -= 0x10;
	if (result < 0x100)
	    result = 0x100;
    }
    if (offset > 0) {
	while ((result & 0x0f) > 0x09)
	    result += 0x01;
	while ((result & 0xf0) > 0x90)
	    result += 0x10;
	if (result > 0x899)
	    result = 0x899;
    }
    return result;
}

static void vbipage(struct REQUEST *req, struct vt_page *page,
		    int pagenr, int subnr, int html)
{
    char *out;
    int size,len,x,y;
    int color,lcolor,link;
    struct fmt_page pg[1];

    struct fmt_char l[W+2];
#define L (l+1)

    fmt_page(fmt,pg,page);
    size = 2*BUFSIZE;
    out = malloc(size);
    len = 0;

    if (html)
	len += sprintf(out+len,page_top,page->pgno,page->subno);
    for (y = 0; y < H; y++) {
	if (~pg->hid & (1 << y)) {  /* !hidden */
	    for (x = 0; x < W; ++x) {
		struct fmt_char c = pg->data[y][x];
		switch (c.ch) {
		case 0x00:
		case 0xa0:
		    c.ch = ' ';
		    break;
		case 0x7f:
		    c.ch = '*';
		    break;
		case BAD_CHAR:
		    c.ch = '?';
		    break;
		default:
		    if (c.attr & EA_GRAPHIC) {
			if (ascii_art) {
			    if (!(c.ch & 128))
				c.ch = graphic1[c.ch];
			    else
				c.ch = graphic2[c.ch-128];
			} else {
			    c.ch = '#';
			}
		    }
		    break;
		}
		L[x] = c;
	    }

	    /* delay fg and attr changes as far as possible */
	    for (x = 0; x < W; ++x)
		if (L[x].ch == ' ') {
		    L[x].fg = L[x-1].fg;
		    L[x].attr = L[x-1].attr;
		}

	    /* move fg and attr changes to prev bg change point */
	    for (x = W-1; x >= 0; x--)
		if (L[x].ch == ' ' && L[x].bg == L[x+1].bg) {
		    L[x].fg = L[x+1].fg;
		    L[x].attr = L[x+1].attr;
		}

	    /* now emit the whole line */
	    lcolor = -1; link = -1;
	    for (x = 0; x < W; ++x) {
		/* close link tags */
		if (html && link >= 0) {
		    if (0 == link)
			len += sprintf(out+len,"</a>");
		    link--;
		}

		/* color handling */
		color = (L[x].fg&0x0f) * 10 + (L[x].bg&0x0f);
		if (html && color != lcolor) {
		    if (-1 != lcolor)
			len += sprintf(out+len,"</span>");
		    len += sprintf(out+len,"<span class=\"c%02d\">",color);
		    lcolor = color;
		}

		/* check for references to other pages */
		if (html && y > 0 && -1 == link && x < W-2 &&
		    isdigit(L[x].ch) &&
		    isdigit(L[x+1].ch) &&
		    isdigit(L[x+2].ch) &&
		    !isdigit(L[x+3].ch) &&
		    !isdigit(L[x-1].ch)) {
		    len += sprintf(out+len,"<a href=\"/%c%c%c/\">",
				   L[x].ch, L[x+1].ch, L[x+2].ch);
		    link = 2;
		}
		if (html && y > 0 && -1 == link && x < W-1 &&
		    '>' == L[x].ch &&
		    '>' == L[x+1].ch) {
		    len += sprintf(out+len,"<a href=\"/%03x/\">",
				   page->pgno+1);
		    link = 1;
		}
		if (html && y > 0 && -1 == link && x < W-1 &&
		    '<' == L[x].ch &&
		    '<' == L[x+1].ch) {
		    len += sprintf(out+len,"<a href=\"/%03x/\">",
				   page->pgno-1);
		    link = 1;
		}

		/* check for references to WWW pages */
		if (html && y > 0 && -1 == link && x < W-9 &&
		    (((tolower(L[x+0].ch) == 'w') &&
		      (tolower(L[x+1].ch) == 'w') &&
		      (tolower(L[x+2].ch) == 'w') &&
		      (L[x+3].ch == '.')) ||
		     ((tolower(L[x+0].ch) == 'h') &&
		      (tolower(L[x+1].ch) == 't') &&
		      (tolower(L[x+2].ch) == 't') &&
		      (tolower(L[x+3].ch) == 'p')))) {
		    int offs = 0;

		    len += sprintf(out+len,"<a href=\"");
		    if(tolower(L[x].ch == 'w'))
			len += sprintf(out+len,"http://");
		    while ((L[x+offs].ch!=' ') && (x+offs < W)) {
			len += sprintf(out+len,"%c",tolower(L[x+offs].ch));
			offs++;
		    }
		    while ( (*(out+len-1)<'a') || (*(out+len-1)>'z') ) {
			len--;
			offs--;
		    }
		    len += sprintf(out+len,"\">");
		    link = offs - 1;
		}

		/* check for references to other subpages */
		if (html && y > 0 && -1 == link && x < W-2 &&
		    page->subno > 0 &&
		    isdigit(L[x].ch) &&
		    '/' == L[x+1].ch &&
		    isdigit(L[x+2].ch) &&
		    !isdigit(L[x+3].ch) &&
		    !isdigit(L[x-1].ch)) {
		    if (L[x].ch == L[x+2].ch) {
			len += sprintf(out+len,"<a href=\"01.html\">");
		    } else {
			len += sprintf(out+len,"<a href=\"%02x.html\">",
				       L[x].ch+1-'0');
		    }
		    link = 2;
		}
		/* check for FastText links */
		if (html && page->flof && -1 == link && x<W-2 &&
		    24 == y &&
		    L[x].fg>0 &&
		    L[x].fg<8 &&
		    x>0 &&
		    !isspace(L[x].ch)) {
	            link=(L[x].fg==6?3:L[x].fg-1);
		    if(page->link[link].subno == ANY_SUB)
	            {
		        len+=sprintf(out+len,"<a href=\"/%03x/\">",
		            page->link[link].pgno);
		    }
		    else
	            {
		        len+=sprintf(out+len,"<a href=\"/%03x/%02x.html\">",
		            page->link[link].pgno,
			    page->link[link].subno);
		    }
		    link=0;
		    while((L[x+link].fg == L[x].fg) && (x+link<W))
		    {
		        link++;
		    }
		    link--;
		    if(link<1)
		    {
	                link=1;
		    }
		}
	        out[len++] = L[x].ch;
	    }
	    /* close any tags + put newline */
	    if (html && link >= 0)
		len += sprintf(out+len,"</a>");
	    if (html && -1 != lcolor)
		len += sprintf(out+len,"</span>");
	    out[len++] = '\n';

	    /* check bufsize */
	    if (len + BUFSIZE > size) {
		size += BUFSIZE;
		out = realloc(out,size);
	    }
	}
    }
    if (html) {
#define MAXSUBPAGES    32
        int subpage;

	/* close preformatted text header */
	len+=sprintf(out+len,"</pre>\n<div class=quick>\n") ;
	if (vbi->cache->op->get(vbi->cache,pagenr,1)) {
	    /* link all subpages */
	    len += sprintf(out+len,"Subpages:");
	    for (subpage = 1; subpage <= MAXSUBPAGES; subpage++) {
		if (NULL == vbi->cache->op->get(vbi->cache,pagenr,subpage))
		    continue;
		if (subpage != subnr) {
		    len += sprintf(out+len," <a href=\"/%03x/%02x.html\">%02x</a>",
				   pagenr, subpage, subpage);
		} else {
		    len += sprintf(out+len," %02x", subpage);
		}
	    }
	    len += sprintf(out+len,"<br>\n") ;
	}
	len += sprintf(out+len,"<a href=\"/%03x/\"><<</a> &nbsp;",
		       calc_page(pagenr, -0x10));
	len += sprintf(out+len,"<a href=\"/%03x/\"><</a> &nbsp;",
		       calc_page(pagenr, -0x01));
	if (!subnr)
	    len += sprintf(out+len,"<a href=\"/%03x/\">o</a> &nbsp;", pagenr);
	else
	    len += sprintf(out+len,"<a href=\"/%03x/%02x.html\">o</a> &nbsp;", pagenr, subnr);
	len += sprintf(out+len,"<a href=\"/%03x/\">></a> &nbsp;",
		       calc_page(pagenr, +0x01));
	len += sprintf(out+len,"<a href=\"/%03x/\">>></a>",
		       calc_page(pagenr, +0x10));
	len += sprintf(out+len,"<br>\n") ;
	len += sprintf(out+len,"%s",page_bottom);
	req->mime = "text/html; charset=\"iso-8859-1\"";
    } else {
	req->mime = "text/plain; charset=\"iso-8859-1\"";
    }

    req->body  = out;
    req->lbody = len;
    req->free_the_mallocs = 1;
    mkheader(req,200,-1);
}

/* ---------------------------------------------------------------------- */

void buildpage(struct REQUEST *req)
{
    int pagenr, subpage;
    char type;
    struct vt_page *page;

    /* style sheet */
    if (0 == strcmp(req->path,"/alevt.css")) {
	req->mime  = "text/css";
	req->body  = stylesheet;
	req->lbody = sizeof(stylesheet);
	mkheader(req,200,start);
	return;
    }

    /* about */
    if (0 == strcmp(req->path,"/about.html")) {
	req->mime  = "text/html; charset=\"iso-8859-1\"";
	req->body  = page_about;
	req->lbody = sizeof(page_about);
	mkheader(req,200,start);
	return;
    }

    /* entry page */
    if (0 == strcmp(req->path,"/")) {
	strcpy(req->path,"/100/");
	mkredirect(req);
	return;
    }

    /* page with subpages */
    if (3 == sscanf(req->path,"/%3x/%2x.%c",&pagenr,&subpage,&type)) {
	if (debug)
	    fprintf(stderr,"trying %03x/%02x\n",pagenr,subpage);
	page = vbi->cache->op->get(vbi->cache,pagenr,subpage);
	if (NULL != page) {
	    vbipage(req,page,pagenr,subpage,type=='h');
	    return;
	}
	mkerror(req,404,1);
	return;
    }

    /* ... without subpage */
    if (1 == sscanf(req->path,"/%3x/",&pagenr)) {
	if (debug)
	    fprintf(stderr,"trying %03x\n",pagenr);
	page = vbi->cache->op->get(vbi->cache,pagenr,0);
	if (NULL != page) {
	    vbipage(req,page,pagenr,0,1);
	    return;
	}
	page = vbi->cache->op->get(vbi->cache,pagenr,ANY_SUB);
	if (NULL != page) {
	    sprintf(req->path,"/%03x/%02x.html",pagenr,page->subno);
	    mkredirect(req);
	    return;
	}
	mkerror(req,404,1);
	return;
    }

    /* goto form */
    if (1 == sscanf(req->path,"/goto/?p=%d",&pagenr)) {
	sprintf(req->path,"/%d/",pagenr);
	mkredirect(req);
	return;
    }
    
    mkerror(req,404,1);
    return;
}
