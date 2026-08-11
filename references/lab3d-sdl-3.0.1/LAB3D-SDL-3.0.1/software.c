#define _USE_MATH_DEFINES
#include <math.h>
#include "lab3d.h"

#define EPSILON 0.0000001

static SDL_Surface *screen, *back;
static SDL_Surface *current;

static K_INT16 *hitwall, *hitheight, *hitu;

static int doublebuf=1;

void softwareInit() {
    SDL_Surface *icon;
    Uint32 flags;

    SDL_ShowCursor(0);

    fprintf(stderr,"Activating video (software/SDL)...\n");

    hitu=malloc(screenwidth*sizeof(K_INT16));
    hitwall=malloc(screenwidth*sizeof(K_INT16));
    hitheight=malloc(screenwidth*sizeof(K_INT16));    

    if (hitu==NULL||hitwall==NULL||hitheight==NULL) {
	fprintf(stderr,"Render buffer allocate failed (low on memory).\n");
	SDL_Quit();
	exit(-1);
    }

    icon=SDL_LoadBMP("ken.bmp");
    if (icon==NULL) {
	fprintf(stderr,"Warning: ken.bmp (icon file) not found.\n");
    }
    SDL_WM_SetIcon(icon,NULL);
    flags = SDL_DOUBLEBUF|SDL_HWSURFACE;
    if (fullscreen)
	flags |= SDL_FULLSCREEN;
    if (SDL_VideoModeOK(screenwidth, screenheight, 8,
			flags|SDL_HWPALETTE)!=8)
	fprintf(stderr,"8-bit colour failed; taking whatever is available.\n");
    if ((screen=SDL_SetVideoMode(screenwidth, screenheight, 8, 
				 flags|SDL_HWPALETTE))==
	 NULL) {
	if ((screen=SDL_SetVideoMode(screenwidth, screenheight, 0, 
				     flags))==
	    NULL) {
	    fprintf(stderr,"Video mode set failed.\n");
	    SDL_Quit();
	    exit(-1);
	}
    }	

    if (!(screen->flags&SDL_DOUBLEBUF)) {
	fprintf(stderr, "Double buffering not available.\n");
	doublebuf=0;
    }

    SDL_SetGamma(gammalevel,gammalevel,gammalevel);
    if (doublebuf) {
	back=SDL_CreateRGBSurface(SDL_HWSURFACE, screenwidth, screenheight, 8, 
				  0, 0, 0, 0);
    } else
	back=NULL;
    current=screen;

    fprintf(stderr, "Opened software renderer.\n");

    screenbufferwidth=374;
    screenbufferheight=746;

    screenbuffer=malloc(screenbufferwidth*screenbufferheight);    
    SDL_WM_SetCaption("Ken's Labyrinth", "Ken's Labyrinth");
}

void softwareSwapBuffers() {
//    SDL_BlitSurface(back, NULL, screen, NULL);
    if (SDL_Flip(screen))
	fprintf(stderr, "Buffer flip failed!\n");
}

void softwareClearScreen() {
    SDL_FillRect(current, NULL, 0);
}

void softwareClearScreenScrollyPalette(void) {
    SDL_FillRect(current, NULL, 255);
}

/* Darkened palette for end of game sequences... */

void softwareSetDarkenedPalette() {
    SDL_Color c[256];
    K_INT16 a;

    for(a=0;a<256;a++) {
	c[a].r=palette[a*3]*27/16;
	c[a].g=palette[a*3+1]*27/16;
	c[a].b=palette[a*3+2]*27/16;
    }

    if (ingame)
	c[255].r=c[255].g=c[255].b=0;

    SDL_SetPalette(screen, SDL_LOGPAL|SDL_PHYSPAL, c, 0, 256);
    if (back!=NULL)
	SDL_SetPalette(back, SDL_LOGPAL|SDL_PHYSPAL, c, 0, 256);
}

/* Normal palette... */

void softwareSetTransferPalette() {

}

/* Change some of the palette... */

void softwareUpdateOverlayPalette(K_UINT16 start,K_UINT16 amount,
				  unsigned char *cols) {
    K_INT16 i;
    SDL_Color c[256];

    for(i=0;i<amount;i++) {
	c[i+start].r=cols[i*3]*4;
	c[i+start].g=cols[i*3+1]*4;
	c[i+start].b=cols[i*3+2]*4;
    }

    SDL_SetPalette(screen, SDL_LOGPAL|SDL_PHYSPAL, c+start, start, amount);
    if (back!=NULL)
	SDL_SetPalette(back, SDL_LOGPAL|SDL_PHYSPAL, c+start, start, amount);
}

void softwareDrawVolumeBar(int vol,int type,float level) {
    int yoffset=15-30*type+110;
    int xoffset=96;
    int xmax=128;
    int xsize=vol>>1;
    int ysize=20;
    int col;
    SDL_Rect r;
    r.x=(xoffset+(virtualscreenwidth-360)/2)*screenwidth/virtualscreenwidth;
    r.y=(yoffset+(virtualscreenheight-240)/2)*screenheight/virtualscreenheight;
    r.w=xmax*screenwidth/virtualscreenwidth;
    r.h=ysize*screenheight/virtualscreenheight;
    if (level>0.5) level=0.5;
    
    SDL_FillRect(current, &r, 0);

    col=type?SDL_MapRGB(current->format, 0, 0, 255):
	SDL_MapRGB(current->format, 255, 0, 0);

    r.w=xsize*screenwidth/virtualscreenwidth;

    SDL_FillRect(current, &r, col);
}

void softwareUploadImage(int i, unsigned char *walsegg, unsigned char create) {
    
}

void softwareTransitionTexture(int left, int mid, int right) {

}

void softwareUpdateImage(int i) {

}

void softwareUploadPartialOverlay(int x,int y,int w,int h) {
    if (menuing) return;
    ShowPartialOverlay(x-1,y-1,w+2,h+2,0);
}

void softwareUploadOverlay(void) {

}

void softwareShowPartialOverlay(int x,int y,int w,int h,int statusbar) {
    int i,j;

    int xoff,yoff1,yoff2;
    
    int xo,yo;
    float xs=(((float)screenwidth)/((float)virtualscreenwidth)); 
    float ys=(((float)screenheight)/((float)virtualscreenheight));
    int xsi,ysi;
    int startx,endx;
    int starty,endy;
    int ox,oy,stox,stoy;
    unsigned char *orgl, *dest, *org;

    if (statusbar==0) {
	y-=visiblescreenyoffset;
	if (x+w>360) w=360-x;
	if (y+h>240) h=240-y;
	if (x<0) {w+=x; x=0;}
	if (y<0) {h+=y; y=0;}
	if ((w<=0)||(h<=0)) return;
	y+=visiblescreenyoffset;
    }  

    xoff=(virtualscreenwidth-360)/2;
    yoff1=(virtualscreenheight-240)/2;
    yoff2=(-statusbaryoffset+virtualscreenheight-statusbaryvisible);

    if (statusbar==1) {
	xo=xoff;
	yo=yoff2;
    } else if (statusbar==2) {
	xo=xoff+(x-340);
	yo=yoff2;
	x=340; y=statusbaryoffset;
    } else {
	xo=xoff;
	yo=yoff1;
    }

    /* Software scaler... */

    startx=(xo+x)*xs;
    if (startx<0) {
	x=-xo;
	w+=startx/xs;
	startx=0;
    }
    starty=(yo+(y-visiblescreenyoffset))*ys;
    if (starty<0) {
	y=-yo+visiblescreenyoffset;
	h+=starty/xs;
	starty=0;
    }
    endx=(xo+(x+w))*xs;
    endy=(yo+(y-visiblescreenyoffset+h))*ys;

    if (endy>screenheight)
	endy=screenheight;

    if (endx>screenwidth)
	endx=screenwidth;

    xsi=65536.0/xs;
    ysi=65536.0/ys;

    stox=(startx/xs-xo)*65536.0;
    stoy=(starty/ys-yo+visiblescreenyoffset)*65536.0;

    while(stox<0) {
	stox+=xsi;
	startx++;
    }

    while(stoy<0) {
	stoy+=ysi;
	starty++;
    }

    SDL_LockSurface(current);
    oy=stoy;
    for(j=starty;((oy>>16)<screenbufferheight)&&(j<endy);j++) {
	orgl=screenbuffer+((screenbufferwidth*(oy>>16)));
	dest=((unsigned char *)(current->pixels))+startx+(j*current->pitch);
	ox=stox;
	if (ingame)
	    for(i=startx;((ox>>16)<screenbufferwidth)&&(i<endx);
		i++) {
		org=orgl+(ox>>16);
		if ((*org)!=255)
		    *dest=*org;
		
		dest++;
		ox+=xsi;
	    }
	else
	    for(i=startx;((ox>>16)<screenbufferwidth)&&(i<endx);
		i++) {
		org=orgl+(ox>>16);
		*dest=*org;
		
		dest++;
		ox+=xsi;
	    }
	oy+=ysi;
    }

    SDL_UnlockSurface(current);

    if (statusbar==1) {
	for(i=0;i<(virtualscreenwidth-319)/2;i+=20) {
	    ShowPartialOverlay(340+i,statusbaryoffset,20,statusbaryvisible,2);
	    ShowPartialOverlay(0-i,statusbaryoffset,20,statusbaryvisible,2);
	}
    }
}

void softwareScreenCapture(void) {
    char filename[15];
    
    strcpy(filename,"capturxx.bmp");
    filename[6] = (capturecount/10)+48;
    filename[7] = (capturecount%10)+48;
    capturecount++;
    if (capturecount==100)
	capturecount=0; /* Just to keep the file names sane. */
    if (SDL_SaveBMP(screen, filename))
	ksay(8);
    else
	ksay(7);
}

void softwareDrawFront(void) {
    current=screen;
}

void softwareDrawBack(void) {
//    current=back;
    current=screen;
}

void softwareFlush(void) {
    if (back!=NULL)
	SDL_BlitSurface(screen, NULL, back, NULL);
    if (SDL_Flip(screen))
	fprintf(stderr, "Buffer flip failed!\n");
    if (back!=NULL)
	SDL_BlitSurface(back, NULL, screen, NULL);
}

void softwareFade(K_INT16 brightness) {
    SDL_Color c[256];
    K_INT16 a;
    int redfactor, greenfactor, bluefactor;
    int t;
    fadelevel=brightness;

    if (brightness == 63) {
	redfactor=greenfactor=bluefactor=64;
    }
    else if (brightness < 64) {
	redfactor=greenfactor=bluefactor=brightness;
    }
    else if (brightness < 128) {
	redfactor=greenfactor=bluefactor=(brightness-64);
    }
    else {
	redfactor=(brightness-65);
	greenfactor=bluefactor=64;
    }

    for(a=0;a<256;a++) {
	t=(palette[a*3]*redfactor)>>4;
	c[a].r=t>255?255:t;
	t=(palette[a*3+1]*greenfactor)>>4;
	c[a].g=t>255?255:t;
	t=(palette[a*3+2]*bluefactor)>>4;
	c[a].b=t>255?255:t;
    }

    if (ingame)
	c[255].r=c[255].g=c[255].b=0;

    SDL_SetPalette(screen, SDL_LOGPAL|SDL_PHYSPAL, c, 0, 256);
    if (back!=NULL)
	SDL_SetPalette(back, SDL_LOGPAL|SDL_PHYSPAL, c, 0, 256);
}

/* Cast a ray from (posxs,posys) in direction angle, and see what it hits
   and passes through. */

void castraysoft(K_UINT16 posxs,K_UINT16 posys, double angle, double cangle,
		 int scrx) {
    double hitpointx,hitpointy,hitdistance,hitheightd;
    double maxheightd = 32000;

    K_INT16 /*walx,waly,*/waln;
    char wals;

    double tan1,tan2;

    double y1,x2;
    K_INT16 x1,y2,x1i,y2i;
    int y1i,x2i;

    K_INT16 xdir,ydir;
    double xinc,yinc;

    int status;

    char xdet,detr;

    K_INT16 j,k;

    angle=angcan(angle);

    tan1=tan(0.5*M_PI-angle);
    tan2=tan(angle);

    x1=posxs>>10;
    y1=(posxs&1023)/1024.0;
    yinc=tan2;
    xdir=1;
    if ((angle>=M_PI*0.5)&&(angle<M_PI*1.5)) {
	xdir = -1;
	yinc = -yinc;
	x1 += 1; /* Note: if xdir==-1, use x1-1 for wall checks. */
	y1 = 1.0 - y1;
    }
    if (tan2<0) {
	tan2=-tan2;
    }

    y1*=tan2;
	
    if (!(angle>=M_PI))
	y1=-y1;
    y1+=posys/1024.0;

    y2=posys>>10;
    x2=(posys&1023)/1024.0;
    xinc=tan1;
    ydir=1;
    if (angle>=M_PI) {
	ydir = -1;
	xinc = -xinc;
	y2 += 1; /* Note: if ydir==-1, use y2-1 for wall checks. */
	x2 = 1.0 - x2;
    }
    if (tan1<0) {
	tan1=-tan1;
    }
    
    x2*=tan1;
    
    if (!((angle>=M_PI*0.5)&&(angle<M_PI*1.5)))
	x2=-x2;
    x2+=posxs/1024.0;

    x1+=xdir;
    y1+=yinc;
    x2+=xinc;
    y2+=ydir;

    x1i=x1-(xdir<0);
    x2i=x2;
    y1i=y1;
    y2i=y2-(ydir<0);
    
    status=0;

    if (fabs(tan2)<EPSILON) {
	y1i=y1;
	x1i=x1-(xdir<0);
	if ((y1i>=0)&&(y1i<64))
	    while((status!=1)&&(x1i>=0)&&(x1i<64)) {
		tempbuf[(x1i<<6)+y1i]=1;
		status=bmpkind[board[x1i][y1i]&1023];
		if (status!=1) {
		    x1i+=xdir;
		}
	    }
	x1=x1i+(xdir<0);
	if (status!=1) return;
    } else if (fabs(tan1)<EPSILON) {
	x2i=x2;
	y2i=y2-(ydir<0);
	if ((x2i>=0)&&(x2i<64))
	    while((status!=256)&&(y2i>=0)&&(y2i<64)) {
		tempbuf[(x2i<<6)+y2i]=1;
		status=bmpkind[board[x2i][y2i]&1023]<<8;
		if (status!=256) {
		    y2i+=ydir;
		}
	    }
	y2=y2i+(ydir<0);
	if (status!=256) return;
    } else {
	xdet=(tan2<tan1);
	y1i=y1; x2i=x2;

	while((status!=1)&&(status!=256)) {
/*
  printf("Player at (%f,%f):(%d,%lf)(%lf,%d), reald %lf,%lf\n",
  posxs/1024.0,posys/1024.0,x1,y1,x2,y2,
  ((double)(x1-(posxs/1024.0)))/(double)(y1-(posys/1024.0)),
  ((double)(x2-(posxs/1024.0)))/(double)(y2-(posys/1024.0)));
  printf("expd %lf,%lf\n",
  ((double)(xdir))/(double)yinc,
  ((double)xinc)/(double)(ydir));
*/
	    x1i=x1-(xdir<0); y2i=y2-(ydir<0);
	    if (xdet)
		detr=((x1-x2)<0)^(xdir<0);
	    else
		detr=((y1-y2)<0)^(ydir<0);
	    if (detr) {
		if ((x1i<0)||(x1i>=64)||(y1i<0)||(y1i>=64)) break;
		tempbuf[(x1i<<6)+y1i]=1;
		status=bmpkind[board[x1i][y1i]&1023];
		if (status!=1) {
		    x1+=xdir;
		    y1+=yinc;
		    y1i=y1;
		}
	    } else {
		if ((x2i<0)||(x2i>=64)||(y2i<0)||(y2i>=64)) break;
		tempbuf[(x2i<<6)+y2i]=1;
		status=bmpkind[board[x2i][y2i]&1023]<<8;
		if (status!=256) {
		    y2+=ydir;
		    x2+=xinc;
		    x2i=x2;
		}
	    }
	}
	if ((status!=1)&&(status!=256)) return;
    }

    if ((status&0xff)==1) {
	j = ((int)(board[x1-(xdir<0)][y1i]-1)&1023);	
	if ((angle>=M_PI*0.5)&&(angle<M_PI*1.5))
	    k=board[x1i+1][y1i];
	else
	    k=board[x1i-1][y1i];
	if ((k&8192)==0) {
	    k &= 1023;
	    if (lab3dversion) {
		if (((k >= 152) && (k <= 157)) || ((k >= 168) && (k <= 173)))
		    j = 188;
	    } else {
		if ((k >= door1) && (k <= door1+5)) j = doorside1-1;
		if ((k >= door2) && (k <= door2+5)) j = doorside2-1;
		if ((k >= door3) && (k <= door3+7)) j = doorside3-1;
		if ((k >= door4) && (k <= door4+6)) j = doorside4-1;
		if ((k >= door5) && (k <= door5+7)) j = doorside5-1;
	    }
	}
	wals=(xdir<0); /* 0=west, 1=east. */
/*	walx=x1-(xdir<0);
	waly=y1i;*/
	waln=j;
	hitpointx=x1;
	hitpointy=y1;
	x1+=xdir;
	y1+=yinc;
    } else if ((status&0xff00)==256) {
	j = ((int)(board[x2i][y2-(ydir<0)]-1)&1023);
	if (angle<M_PI) 
	    k=board[x2i][y2i-1];
	else
	    k=board[x2i][y2i+1];
	if ((k&8192) > 0)
	{
	    k &= 1023;
	    if (lab3dversion) {
		if (((k >= 152) && (k <= 157)) || ((k >= 168) && (k <= 173)))
		    j = 188;
	    } else {
		if ((k >= door1) && (k <= door1+5)) j = doorside1-1;
		if ((k >= door2) && (k <= door2+5)) j = doorside2-1;
		if ((k >= door3) && (k <= door3+7)) j = doorside3-1;
		if ((k >= door4) && (k <= door4+6)) j = doorside4-1;
		if ((k >= door5) && (k <= door5+7)) j = doorside5-1;
	    }
	}
	wals=2+(ydir<0);
/*	walx=x2i;
	waly=y2-(ydir<0);*/
	waln=j;
	hitpointx=x2;
	hitpointy=y2;
	x2+=xinc;
	y2+=ydir;
    } else return;

    if (lab3dversion==0)
	if (waterstat>0)
	    if ((waln&1023)==fountain-1)
		waln+=(animate2+1);

    if ((status&255)==1) waln|=16384;

    //	fprintf(stderr,"Ray hit at %lf,%lf\n",hitpointx,hitpointy);

    hitwall[scrx]=waln&1023;

    /* hitdistance is distance from player (posxs/1024.0, posys/1024.0) to
       (hitpointx, hitpointy) projected onto the player's view direction.*/
    hitdistance=(hitpointx-(posxs/1024.0))*cos(cangle)+
	(hitpointy-(posys/1024.0))*sin(cangle);

    if (hitdistance<EPSILON)
	hitheightd = maxheightd;
    else
	hitheightd = (2.0/3.0)/asph*screenheight/hitdistance;

    if (hitheightd > maxheightd)
	hitheightd = maxheightd;
    hitheight[scrx]=hitheightd;

    if (hitheight[scrx]<1)
	hitheight[scrx]=1;
    hitu[scrx]=((hitpointx-floor(hitpointx))+
		    (hitpointy-floor(hitpointy)))*64;
    if ((wals&1)^((wals&2)>>1))
	hitu[scrx]=63-hitu[scrx];
}

void softwarePicrot(K_UINT16 posxs, K_UINT16 posys, 
		    K_INT16 poszs, K_INT16 angs) {
    int i,j,sy,ey,oy,sty;
    double ca,a,vw;
    unsigned char *p;
    int s,hh,hih;
    SDL_Rect r;
    unsigned char *orgl;
    K_INT16 temp, k;

    memset(tempbuf, 0, 4096);
    hh=screenheight/2;
    r.x=0;
    r.w=current->w;
    r.h=r.y=hh;

    if (lab3dversion)
	SDL_FillRect(current, &r, 0x85);
    else
	SDL_FillRect(current, &r, 0x85);
    r.y=0;
    SDL_FillRect(current, &r, 0xe3);

    ca=angs/1024.0*M_PI;
    vw=aspw/screenwidth*2;

    SDL_LockSurface(current);
    s=current->pitch;
    for(i=0;i<screenwidth;i++) {
	a=atan((i-screenwidth/2)*vw);
	castraysoft(posxs, posys, ca+a, ca, i);
	orgl=walseg[hitwall[i]]+(hitu[i]<<6);
	hih=hitheight[i];
	if (hih<1) continue;
	sy=(hh-(hih*poszs/64));
	ey=sy+hih;
	sty=4194304/hih;
	hih=4194304/sty; /* Eliminate sawtoothing by dropping y axis precision
			    to match step size. */
	oy=-(((((hh<<16)-(hih*(poszs<<10)))-(sy<<16))*sty)>>16);
	if (sy<0) {
	    oy-=sty*sy;
	    sy=0;
	}
	while(oy<0) {
	    oy+=sty;
	    sy++;
	}
	if (ey>=screenheight) ey=screenheight-1;
	p=((unsigned char *)current->pixels)+(s*sy)+i;
	for(j=sy;(j<ey)&&(oy<4194304);j++) {		
	    *p=*(orgl+((oy>>16)));
	    p+=s;
	    oy+=sty;
	}		
    }
    SDL_UnlockSurface(current);
	
    updatebullets(posxs, posys, poszs);
    checkforvisiblestuff(posxs, posys, poszs, angs);

    /* Draw all the partially transparent stuff in order of distance... */

    totalsortcnt = sortcnt;
    for(i=0;i<totalsortcnt;i++)
    {
	temp = 0;
	for(j=0;j<sortcnt;j++)
	    if (sorti[j] < sorti[temp])
		temp = j;
	k = sortbnum[temp];
	if (bmpkind[k] == 2)
	{
	    if (lab3dversion)
		flatsprite(sortx[temp],sorty[temp],0,angs,k);
	    else {
		if (k == warp)
		    flatsprite(sortx[temp],sorty[temp],
			       (K_INT16)((totalclock<<2)&2047),angs,warp);
		else if (k == bul8fly)
		    flatsprite(sortx[temp],sorty[temp],
			       (K_INT16)((totalclock<<3)&2047),angs,
			       bul8fly+animate2);
		else if (k == bul10fly)
		    flatsprite(sortx[temp],sorty[temp],
			       (K_INT16)((totalclock<<3)&2047),angs,
			       bul10fly+animate2);
		else 
		    flatsprite(sortx[temp],sorty[temp],0,angs,k);
		if (k == fan)
		    flatsprite(sortx[temp],sorty[temp],
			       (K_INT16)((totalclock<<2)&2047),
			       angs,fan+1);
	    }
	}
	if (bmpkind[k] == 4)
	{
	    if (!lab3dversion && (k == slotto))
		if (slottime > 0)
		    k++;
	    doordraw(sortx[temp],sorty[temp],k,posxs,posys);
	}
	sortcnt--;
	sortx[temp]=sortx[sortcnt];
	sorty[temp]=sorty[sortcnt];
	sorti[temp] = sorti[sortcnt];
	sortbnum[temp] = sortbnum[sortcnt];
    }
    ShowStatusBar();
}

/* Draw a sprite at (x,y) in the labyrinth, twisted ang round its Z axis
   (warps/fans only!), oriented to be facing the player who is looking in
   direction playerang. Sprite has texture number walnume-1. */

void softwareFlatsprite(K_UINT16 x, K_UINT16 y,K_INT16 ang,K_INT16 playerang,
			K_INT16 walnume) {
    int sx,sy;
    K_INT16 siz, ysiz;
    float yscale, xscale;
    float xsx, xsy, ysx, ysy;
    int xsxi, xsyi, ysxi, ysyi;
    int oxi,oyi,oxsi,oysi;
    int loxsi,loysi;
    int i,j;
    int startx,endx;
    int starty,endy;
    float ox=0.0,oy=0.0;
    unsigned char *orgl, *dest, *org;
    int sd;
    int ph;

    if (((posx|1023) == (x|1023)) && ((posy|1023) == (y|1023)))
	return;

    siz = (int)(((((long)x-(long)posx)>>2)*sintable[(playerang+512)&2047]+
		 (((long)y-(long)posy)>>2)*sintable[playerang])>>16);
    if (siz==0) return;
    
    ysiz = (int)(((((long)x-(long)posx)>>2)*sintable[playerang]-
		  (((long)y-(long)posy)>>2)*sintable[(playerang+512)&2047])>>16);
    sx=(K_INT16)(((double)screenwidth/2.0)-
		 ((((double)screenwidth/2.0)*(double)ysiz)/(double)siz/aspw));
    if (siz==0) return;
    siz = (int)(((screenheight*163840L))/240/((long)siz));
    yscale=siz/256.0;
    xscale=(yscale*screenwidth/4*3)/screenheight;
    sy=screenheight/2-(siz*(posz-32)/asph/256);
    if (siz<=0) return;

    /* Software scaler... */

    startx=(((float)(sx))-xscale*46.7/aspw);
    if (startx<0) {
	startx=0;
    }
    starty=(((float)(sy))-yscale*46.7/aspw);
    if (starty<0) {
	starty=0;
    }
    if (ang==0) {
	endx=(((float)(sx))+xscale*32.0/aspw);
	endy=(((float)(sy))+yscale*32.0/asph);
    } else {
	endx=(((float)(sx))+xscale*46.7/aspw);
	endy=(((float)(sy))+yscale*46.7/asph);
    }
    if (endx>=screenwidth) {
	endx=screenwidth-1;
    }
    if (endy>=screenheight) {
	endy=screenheight-1;
    }

    ph=yscale/asph*64.0;

    xsx=-sintable[(8192-(ang+512))%2048]/65536.0/xscale*aspw;
    xsy=sintable[(2048-ang)%2048]/65536.0/yscale*asph;
    ysx=-sintable[(2048-ang)%2048]/65536.0/xscale*aspw;
    ysy=-sintable[(8192-(ang+512))%2048]/65536.0/yscale*asph;
    xsxi=-sintable[(8192-(ang+512))%2048]/xscale*aspw;
    xsyi=sintable[(2048-ang)%2048]/yscale*asph;
    ysxi=-sintable[(2048-ang)%2048]/xscale*aspw;
    ysyi=-sintable[(8192-(ang+512))%2048]/yscale*asph;

    SDL_LockSurface(current);
    orgl=walseg[walnume-1];

    ox=(startx-(sx))*xsx+(starty-(sy))*xsy+32.0;
    oy=(startx-(sx))*ysx+(starty-(sy))*ysy+32.0;
    loxsi=ox*65536.0;
    loysi=oy*65536.0;
    sd=current->pitch;
    if (ang==0)
	for(i=startx;i<endx;i++) {
	    if (ph>=hitheight[i]) {
		oxsi=loxsi;
		oysi=loysi;
		dest=((unsigned char *)(current->pixels))+(starty*sd)+i;
		oxi=oxsi>>16;
		if ((oxi<64)&&(oxi>=0))
		    for(j=starty;(j<endy);j++) {
			oyi=oysi>>16;
			if ((oyi<64)&&(oyi>=0)) {
			    org=orgl+oyi+(oxi<<6);
			    if ((*org)!=255)
				*dest=*org;
			}
		
			dest+=sd;
			oysi+=ysyi;
		    }
	    }
	    loxsi+=xsxi;
	}
    else
	for(i=startx;i<endx;i++) {
	    if (ph>=hitheight[i]) {
		oxsi=loxsi;
		oysi=loysi;
		dest=((unsigned char *)(current->pixels))+(starty*sd)+i;
		for(j=starty;(j<endy);j++) {
		    oxi=oxsi>>16;
		    oyi=oysi>>16;
		    if ((oxi<64)&&(oxi>=0)&&(oyi<64)&&(oyi>=0)) {
			org=orgl+oyi+(oxi<<6);
			if ((*org)!=255)
			    *dest=*org;
		    }
		
		    dest+=sd;
		    oxsi+=xsyi;
		    oysi+=ysyi;
		}
	    }
	    loxsi+=xsxi;
	    loysi+=ysxi;
	}
    
    SDL_UnlockSurface(current);
}

/* Draw wall number walnume-1, centred at (x,y), magnified siz>>8 times
   (i.e. at a size of siz>>2*siz>>2 pixels), rotated clockwise (2048 is
   full rotation). Colour 255 is transparent. Clip to y=[0, dside[. 
   Z-Buffering done using height[] (which contains height of object in each
   column and can be considered a 1D inverse Z-buffer (alternatively add
   a Z parameter?). */

void softwarePictur(K_INT16 x,K_INT16 y,K_INT16 siz,
		    K_INT16 ang,K_INT16 walnume) {
    float scale=siz/256.0;
    float xs=((float)screenwidth)/((float)virtualscreenwidth); 
    float ys=((float)screenheight)/((float)virtualscreenheight);
    float xsx, xsy, ysx, ysy;
    int xsxi, xsyi, ysxi, ysyi;
    int startx,endx;
    int starty,endy;
    float ox=0.0,oy=0.0;
    unsigned char *orgl, *dest, *org;
    int i,j;
    int oxi,oyi,oxsi,oysi;
    int loxsi,loysi;
    int xoff=(virtualscreenwidth-360)/2;
    int yoff1=(virtualscreenheight-240)/2;
    
    y+=spriteyoffset;

    /* Software scaler... */

    if (ang!=0) {
	startx=(((float)(x+xoff))-scale*46.7)*xs;
	starty=(((float)(y+yoff1))-scale*46.7)*ys;
	endx=(((float)(x+xoff))+scale*46.7)*xs;
	endy=(((float)(y+yoff1))+scale*46.7)*ys;
    } else {
	startx=(((float)(x+xoff))-scale*32.0)*xs;
	starty=(((float)(y+yoff1))-scale*32.0)*ys;
	endx=(((float)(x+xoff))+scale*32.0)*xs;
	endy=(((float)(y+yoff1))+scale*32.0)*ys;	
    }
    if (startx<0) {
	startx=0;
    }
    if (starty<0) {
	starty=0;
    }
    if (endx>=screenwidth) {
	endx=screenwidth-1;
    }
    if (endy>=screenheight) {
	endy=screenheight-1;
    }

    xsx=-sintable[(8192-(ang+512))%2048]/65536.0/scale/xs;
    xsy=sintable[(2048-ang)%2048]/65536.0/scale/ys;
    ysx=-sintable[(2048-ang)%2048]/65536.0/scale/xs;
    ysy=-sintable[(8192-(ang+512))%2048]/65536.0/scale/ys;    
    xsxi=-sintable[(8192-(ang+512))%2048]/scale/xs;
    xsyi=sintable[(2048-ang)%2048]/scale/ys;
    ysxi=-sintable[(2048-ang)%2048]/scale/xs;
    ysyi=-sintable[(8192-(ang+512))%2048]/scale/ys;    

    SDL_LockSurface(current);
    orgl=walseg[walnume-1];

    ox=(startx-(x+xoff)*xs)*xsx+(starty-(y+yoff1)*ys)*xsy+32.0;
    oy=(startx-(x+xoff)*xs)*ysx+(starty-(y+yoff1)*ys)*ysy+32.0;
    loxsi=ox*65536.0;
    loysi=oy*65536.0;

    if (ang==0) {
	for(j=starty;j<endy;j++) {
	    oxsi=loxsi;
	    oysi=loysi;
	    dest=((unsigned char *)(current->pixels))+startx+(j*current->pitch);
	    oyi=oysi>>16;
	    if ((oyi<64)&&(oyi>=0))
		for(i=startx;(i<endx);i++) {
		    oxi=oxsi>>16;
		    if ((oxi<64)&&(oxi>=0)) {
			org=orgl+oyi+(oxi<<6);
			if ((*org)!=255)
			    *dest=*org;
		    }
		    
		    dest++;
		    oxsi+=xsxi;
		}
	    loysi+=ysyi;
	}
    } else
	for(j=starty;j<endy;j++) {
	    oxsi=loxsi;
	    oysi=loysi;
	    dest=((unsigned char *)(current->pixels))+startx+(j*current->pitch);
	    for(i=startx;(i<endx);i++) {
		oxi=oxsi>>16;
		oyi=oysi>>16;
		if ((oxi<64)&&(oxi>=0)&&(oyi<64)&&(oyi>=0)) {
		    org=orgl+oyi+(oxi<<6);
		    if ((*org)!=255)
			*dest=*org;
		}
	    
		dest++;
		oxsi+=xsxi;
		oysi+=ysxi;
	    }
	    loxsi+=xsyi;
	    loysi+=ysyi;
	}

    SDL_UnlockSurface(current);
}

void softwareFilter(unsigned char filter) {

}

void softwareTextureDepth(int depth) {

}

void softwareInitFailsafe(void) {
    SDL_Surface *icon;

    SDL_ShowCursor(0);

    fprintf(stderr,"Activating video (software/SDL failsafe)...\n");

    screenwidth=360; screenheight=240;

    icon=SDL_LoadBMP("ken.bmp");
    if (icon==NULL) {
	fprintf(stderr,"Warning: ken.bmp (icon file) not found.\n");
    }
    SDL_WM_SetIcon(icon,NULL);
    if ((screen=SDL_SetVideoMode(screenwidth, screenheight, 8, 
				 SDL_HWPALETTE))==
	NULL) {
	fprintf(stderr,"Video mode set failed.\n");
	SDL_Quit();
	exit(-1);
    }

    SDL_SetGamma(1.0,1.0,1.0); /* Zap gamma correction. */

    screenwidth=screen->w;
    screenheight=screen->h;

    virtualscreenwidth=360;
    virtualscreenheight=240;

    screenbufferwidth=374;
    screenbufferheight=746;

    screenbuffer=malloc(screenbufferwidth*screenbufferheight);    
    SDL_WM_SetCaption("Ken's Labyrinth", "Ken's Labyrinth");
    back=NULL;
/*    back=SDL_CreateRGBSurface(SDL_HWSURFACE, screenwidth, screenheight, 8, 
      0, 0, 0, 0);*/
}

/* Draw wall number walnume-1 at board position (x,y) (multiply by
   1024 to get a value compatible with player co-ordinates), as seen from
   (posxs, posys, poszs) looking in direction angs (0-4095).
   board[x][y]&8192 indicates the direction in which the door points
   (extends over x (0) or over y (1). */

void softwareDoordraw(K_UINT16 x,K_UINT16 y,K_INT16 walnume,K_UINT16 posxs,
		      K_UINT16 posys) {
    K_INT32 x1, y1, x2, y2, dx, dy, hp, tp;
    K_INT16 sx1, sx2;
    K_INT16 lx, rx;
    K_INT16 siz, ysiz;
    int i;
    unsigned char *orgl, *dest, *org, *p;
    double st, ca, vw, a, sxt;
    int oy, sy, ey, s, j, sty;

    x1=((K_INT32)x);
    y1=((K_INT32)y);

    /* Calculate endpoint world coordinates. */

    if ((board[x>>10][y>>10]&8192)) {
	x2=x1;
	y1+=512;
	y2=y1-1024;
    } else {
	y2=y1;
	x1-=512;
	x2=x1;
	x2+=1024;	
    }

    if (
	(lab3dversion&&(!(((walnume >= 152) && (walnume <= 157)) ||
			  ((walnume >= 168) && (walnume <= 173)) ||
			  (walnume == 180)))) ||
	((!lab3dversion)&&((!(((walnume >= door1) && (walnume <= door1+5)) ||
			      ((walnume >= door2) && (walnume <= door2+5)) ||
			      ((walnume >= door5) && (walnume <= door5+7)) ||
			      (walnume == 180)))))) {
	if ((board[x>>10][y>>10]&8192)) {
	    if (posxs<x) {
		y1-=1024;
		y2+=1024;
	    }
	} else {
	    if (posys<y) {
		x1+=1024;
		x2-=1024;
	    }
	}
    }
  
    /* Calculate start and end point directions. */

    siz = (int)(((((long)x1-(long)posxs)>>2)*sintable[(ang+512)&2047]+
		 (((long)y1-(long)posys)>>2)*sintable[ang])>>16);
    ysiz = (int)(((((long)x1-(long)posxs)>>2)*sintable[ang]-
		  (((long)y1-(long)posys)>>2)*sintable[(ang+512)&2047])>>16);
    if (siz>0) {    
	sxt=(((double)screenwidth/2.0)-
	     ((((double)screenwidth/2.0)*
	       (double)ysiz)/(double)siz/aspw));
	if ((sxt<32766)&&sxt>=-32766)
	    sx1=(K_INT16)sxt;
	else if (sxt<0)
	    sx1=-32767;
	else
	    sx1=32767;
    } else sx1=(ysiz>0)?0:(screenwidth-1);

//    fprintf(stderr, "a: %d %d %d\n", siz, ysiz, sx1);

    siz = (int)(((((long)x2-(long)posxs)>>2)*sintable[(ang+512)&2047]+
		 (((long)y2-(long)posys)>>2)*sintable[ang])>>16);
    ysiz = (int)(((((long)x2-(long)posxs)>>2)*sintable[ang]-
		  (((long)y2-(long)posys)>>2)*sintable[(ang+512)&2047])>>16);
    if (siz>0) {
	sxt=(((double)screenwidth/2.0)-
	     ((((double)screenwidth/2.0)*
	       (double)ysiz)/(double)siz/aspw));
	if ((sxt<32766)&&sxt>=-32766)
	    sx2=(K_INT16)sxt;
	else if (sxt<0)
	    sx2=-32767;
	else
	    sx2=32767;
    } else sx2=(ysiz>0)?0:(screenwidth-1);
    
//    fprintf(stderr, "b: %d %d %d\n", siz, ysiz, sx2);

    if (sx1<sx2) {
	lx=sx1;
	rx=sx2;
    } else {
	lx=sx2;
	rx=sx1;
    }

    if (lx<0) lx=0;
    if (rx>=screenwidth) rx=screenwidth-1;

    SDL_LockSurface(current);
    dest=((unsigned char *)(current->pixels))+lx;
    s=current->pitch;
    ca=ang/1024.0*M_PI;
    vw=aspw/screenwidth*2;

    s=current->pitch;
    for(i=lx;i<=rx;i++,dest++) {
	a=(ca+atan((i-screenwidth/2)*vw))*1024.0/M_PI;
	dx=sintable[((int)a+512)&2047];
	dy=sintable[((int)a)&2047];

	if (x1==x2) {
	    if (dx==0) continue;
	    st=((double)(x1-posxs))/(double)dx;
	    hp=st*dy+posys;
	    tp=((double)hp-y1)/(y2-y1)*64;
	    siz = (int)(((((long)x1-(long)posx))*sintable[(ang+512)&2047]+
			 (((long)hp-(long)posy))*sintable[ang])>>16);
	} else {
	    if (dy==0) continue;
	    st=((double)(y1-posys))/(double)dy;	    
	    hp=st*dx+posxs;
	    tp=((double)hp-x1)/(x2-x1)*64;
	    siz = (int)(((((long)hp-(long)posx))*sintable[(ang+512)&2047]+
			 (((long)y1-(long)posy))*sintable[ang])>>16);
	}
	
	if (tp<0) continue;
	if (tp>63) continue;
	if (siz<=0) continue;

	siz = (int)(((screenheight*163840L/asph))/240/((long)siz));
	if (siz<=0) continue;
	if (siz<hitheight[i]) continue;
	if (posz==32)
	    siz=siz&~1;
	orgl=walseg[walnume-1]+(tp<<6);
	sy=(screenheight/2-siz*posz/64);
	sty=4194304/siz;
	oy=(((((screenheight<<15)-siz*(posz<<10)))-(sy<<16))*sty)>>16;
	ey=sy+siz;
	if (sy<0) {
	    oy-=sty*sy;
	    sy=0;
	}
	while(oy<0) {
	    oy+=sty;
	    sy++;
	}
	p=dest+(s*sy);
	if (ey>=screenheight) ey=screenheight-1;
	for(j=sy;(j<ey)&&(oy<4194304);j++) {		
	    org=orgl+((oy>>16));
	    if ((*org)!=255)
		*p=*org;
	    p+=s;
	    oy+=sty;
	}
    }
    SDL_UnlockSurface(current);
}
