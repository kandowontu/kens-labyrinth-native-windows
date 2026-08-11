#include "lab3d.h"
#include <math.h>

/* Draw wall number walnume-1, topleft at (x,y), magnified siz>>8 times
   (i.e. at a size of siz>>2*siz>>2 pixels). Colour 255 is transparent.
   Clip to y=[0, dside[. 
   Z-Buffering done using height[] (which contains height of object in each
   column and can be considered a 1D inverse Z-buffer (alternatively add
   a Z parameter?). */

void spridraw(K_INT16 x, K_INT16 y, K_INT16 siz, K_INT16 walnume)
{
    pictur(x+(siz>>3),y+(siz>>3),siz,0,walnume);
}

/* Draw an xsiz wide, ysiz high part of texture walnume-1 (from texel
   (picx,picy)) to (x,y) on screen. Shift it a bit to the right to compensate
   for emulating 360x240 rather than 320x200. */

void statusbardraw(K_UINT16 picx, K_UINT16 picy, K_UINT16 xsiz,
		   K_UINT16 ysiz, K_UINT16 x, K_UINT16 y, K_INT16 walnume)
{
    x+=20;

    drawtooverlay(picx,picy,xsiz,ysiz,x,y,walnume-1,0);
}

/* Draw a w wide, h high part of texture walnum (from texel
   (picx,picy)) to (x,y) on screen, adding coloff to colour index. */

void drawtooverlay(K_UINT16 picx, K_UINT16 picy, K_UINT16 w,
		   K_UINT16 h, K_UINT16 x, K_UINT16 y, K_INT16 walnum,
		   unsigned char coloff) {
    int a,b;
    unsigned char *pic,*buf;

    y+=spriteyoffset;
    y+=visiblescreenyoffset;

    if (x+w>=screenbufferwidth)
	fprintf(stderr, "Drawing off right side!");
    if (y+h>=screenbufferheight) {
	fprintf(stderr, "Drawing off bottom side! %d %d\n",y,h);
    }

    for(a=0;a<w;a++) {
	pic=walseg[walnum]+(((picx+a)<<6)+picy);
	buf=screenbuffer+(screenbufferwidth*y+(x+a));
	for(b=0;b<h;b++) {
	    if ((*pic)!=255) *buf=(*pic)+coloff;
	    pic++;
	    buf+=screenbufferwidth;
	}
    }

    gfxUploadPartialOverlay(x,y,w,h);
}

/* Wipe a rectangular area of the overlay. */

void wipeoverlay(K_UINT16 x,K_UINT16 y,K_UINT16 w, K_UINT16 h) {
    int a;
    
    for(a=y;a<y+h;a++)
	memset(screenbuffer+((screenbufferwidth*a)+x),ingame?255:0x50,w);

    gfxUploadPartialOverlay(x,y,w,h);    
}
