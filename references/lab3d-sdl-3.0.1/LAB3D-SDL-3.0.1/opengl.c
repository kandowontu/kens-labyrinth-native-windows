#define _USE_MATH_DEFINES
#include <math.h>
#ifdef _MSC_VER
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#ifndef GL_BGR
#ifdef GL_BGR_EXT
/* MSVC compatibility hack (some versions have out-of-date OpenGL headers). */
#define GL_BGR GL_BGR_EXT
#else
/* MinGW compatibility hack (why is GL 1.2 considered an extension?). */
#include <GL/glext.h>
#endif
#endif
#include "lab3d.h"

float fogcol[4]={0.5,0.5,0.5,1.0};

static K_INT16 wallfound[64][64][4];
static char wallside[16384];
static K_INT16 wallx[16384],wally[16384];
static K_UINT16 walnum[16384];

/* Palette for OpenGL transfer... */

static GLfloat Red[256],Blue[256],Green[256];
static GLfloat Alpha[256];
static GLuint texName[numwalls+1];
static GLuint gameoversprite;
static GLuint splitTexName[numsplits][2];
static K_INT16 splitTexNum[numsplits];

static GLuint screenbuffertexture; /* One big texture. */
static GLuint screenbuffertextures[72]; /* 72 small textures. */
static int largescreentexture;

/* Fade level... */
static GLfloat redfactor,bluefactor,greenfactor;

/* Filtering modes. */
static GLint fullfilter,partialfilter;

/* Colour depth. */
static GLint colourformat;

/* Precalculated BMP header; RGB, 24-bit. */

unsigned char BMPHeader[54]={0x42,0x4d,0,0,0,0,0,0,
			     0,0,0x36,0,0,0,0x28,0,
			     0,0,0,0,0,0,0,0,
			     0,0,1,0,0x18,0,0,0,
			     0,0,0,0,0,0,0,0,
			     0,0,0,0,0,0,0,0,
			     0,0,0,0,0,0};

/* Write a DWORD (32-bit int) to BMP header. */
			     
void bmpheaderwrite(int offset, K_UINT32 value) {
    int a;

    for(a=0;a<4;a++)
	BMPHeader[offset+a]=(value>>(a<<3))&255;
}

void openGLInit() {
    SDL_Surface *screen;
    SDL_Surface *icon;
    int realr,realg,realb,realz,reald;

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE,0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE,0);
    
    SDL_ShowCursor(0);

    fprintf(stderr,"Activating video (OpenGL)...\n");

    icon=SDL_LoadBMP("ken.bmp");
    if (icon==NULL) {
	fprintf(stderr,"Warning: ken.bmp (icon file) not found.\n");
    }
    SDL_WM_SetIcon(icon,NULL);
    if ((screen=SDL_SetVideoMode(screenwidth, screenheight, 32, 
				 fullscreen?
				 (SDL_OPENGL|SDL_FULLSCREEN):SDL_OPENGL))==
	NULL) {
	fprintf(stderr,"True colour failed; taking whatever is available.\n");
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,16);
	if ((screen=SDL_SetVideoMode(screenwidth, screenheight, 0, 
				     fullscreen?
				     (SDL_OPENGL|SDL_FULLSCREEN):SDL_OPENGL))==
	    NULL) {
	    fprintf(stderr,"Video mode set failed.\n");
	    SDL_Quit();
	    exit(-1);
	}
    }	

    SDL_GL_GetAttribute(SDL_GL_RED_SIZE,&realr);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE,&realg);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE,&realb);
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE,&realz);
    SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER,&reald);

    fprintf(stderr,"GL Vendor: %s\n",glGetString(GL_VENDOR));
    fprintf(stderr,"GL Renderer: %s\n",glGetString(GL_RENDERER));
    fprintf(stderr,"GL Version: %s\n",glGetString(GL_VERSION));
    fprintf(stderr,"GL Extensions: %s\n",glGetString(GL_EXTENSIONS));

    fprintf(stderr,"GLU Version: %s\n",gluGetString(GLU_VERSION));
    fprintf(stderr,"GLU Extensions: %s\n",gluGetString(GLU_EXTENSIONS));

    if (reald==0) {
	fprintf(stderr,"Double buffer not available.\n");
	SDL_Quit();
	exit(-1);
    }

    if (realz<16) {
	fprintf(stderr,"Warning: Depth buffer resolution too low; expect\n");
	fprintf(stderr,"graphical glitches.\n");
	SDL_Quit();
	exit(-1);
    }

    SDL_SetGamma(gammalevel,gammalevel,gammalevel);

    if (realz<24) {
	walltol=256; neardist=128;
    } else {
	walltol=32; neardist=16;
    }

    fprintf(stderr,
	    "Opened GL at %d/%d/%d (R/G/B) bits, %d bit depth buffer.\n",
	    realr,realg,realb,realz);

    /* SDL on Windows does funny things if the specified window size or
       fullscreen resolution is invalid. SDL on X fakes it seamlessly. The
       documentation doesn't mention anything about this, so I assume that this
       aspect of SDL is undefined. */

    if ((screenwidth!=screen->w)||(screenheight!=screen->h)) {
	fprintf(stderr,"Warning: screen resolution is actually %dx%d.\n",
		screen->w,screen->h);
	if ((screen->w<screenwidth)||(screen->h<screenheight)) {
	    fprintf(stderr,"Too small to pad; using full screen.\n");
	    screenwidth=screen->w;
	    screenheight=screen->h;	    
	}
	else {
	    glViewport((screen->w-screenwidth)>>1,(screen->h-screenheight)>>1,
		       screenwidth,screenheight);
	    fprintf(stderr,"Using a viewport within the screen.\n");
	}
    }

    /* Actually, this mode seems to be both more compatible and faster, so
       I think I'll leave it like this. Change this to 1 at your own risk. */

    largescreentexture=0;
							      
    if (largescreentexture) {
	/* One large 512x512 texture. */

	screenbufferwidth=screenbufferheight=512;
    } else {
	/* 6*11 matrix of 64x64 tiles with 1 pixel wide borders on shared
	   edges. */

	screenbufferwidth=374;
	screenbufferheight=746;
    }

    screenbuffer=malloc(screenbufferwidth*screenbufferheight);    
    SDL_WM_SetCaption("Ken's Labyrinth", "Ken's Labyrinth");

    fprintf(stderr,"Allocating screen buffer texture...\n");

    if (largescreentexture) {
	glGenTextures(1,&screenbuffertexture);
    } else {
	glGenTextures(72,screenbuffertextures);
    }

}

void openGLSwapBuffers() {
    if (debugmode)
      fprintf(stderr,"TIMING: Swapping buffers\n");
    SDL_GL_SwapBuffers();
    if (debugmode) {
	/* Make undefined buffer data annoyingly purple. */
      glClearColor(1,0,1,0);
      glClear( GL_COLOR_BUFFER_BIT);
    }
}

void openGLClearScreen() {
    glClearColor(0,0,0,0);
    glClear( GL_COLOR_BUFFER_BIT);
}

/* Check OpenGL status and complain if necessary. */
void checkGLStatus()
{
    GLenum errCode;
    const GLubyte *errString;

    if ((errCode=glGetError())!=GL_NO_ERROR) {
	errString=gluErrorString(errCode);
	fprintf(stderr,"OpenGL Error: %s\n",errString);
    }
}

static K_INT32 wallsfound;

static K_INT32 rayscast;


static double hitpointx,hitpointy;

static K_INT16 mapfound;
static K_INT16 gameoverfound;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define EPSILON 0.0000001

/* (x2-x1)^2+(y2-y1)^2. */

double distance2(double x1,double y1,double x2,double y2) {
    double dx=x2-x1;
    double dy=y2-y1;

    return dx*dx+dy*dy;
}

/* Cast a ray from (posxs,posys) in direction angle, and see what it hits
   and passes through. */

K_INT16 castray(K_UINT16 posxs,K_UINT16 posys, double angle) {
    K_INT16 walx,waly,waln;
    char wals;

    double tan1,tan2;

    double y1,x2;
    K_INT16 x1,y2,x1i,y2i;
    int y1i,x2i;

    K_INT16 xdir,ydir;
    double xinc,yinc;

    int status;

    char xdet,detr;

    char cont;

    K_INT16 j,k;

    K_INT32 cx1,cy1;

    rayscast++;

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
    
    cont=1;
    while(cont) {
	status=0;
	cont=0;

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
	    if (status!=1) return -1;
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
	    if (status!=256) return -1;
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
	    if ((status!=1)&&(status!=256)) return -1;
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
	    walx=x1-(xdir<0);
	    waly=y1i;
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
	    walx=x2i;
	    waly=y2-(ydir<0);
	    waln=j;
	    hitpointx=x2;
	    hitpointy=y2;
	    x2+=xinc;
	    y2+=ydir;
	} else return -1;

	if (lab3dversion==0)
	    if (waterstat>0)
		if ((waln&1023)==fountain-1)
		    waln+=(animate2+1);

	if ((status&255)==1) waln|=16384;

	//	fprintf(stderr,"Ray hit at %lf,%lf\n",hitpointx,hitpointy);

	/* Keep going if ray hit a wall very close to us. */

	cx1=((K_INT32)walx)<<10;
	cy1=((K_INT32)waly)<<10;

	cont=0;

	switch(wals) {
	    case 0: /* West */
		if (((cx1-posxs)<walltol)&&
		    (abs(posys-(cy1+512))<(512+walltol)))
		    cont=1;
		break;
	    case 1: /* East */
		if (((posxs-(cx1+1024))<walltol)&&
		    (abs(posys-(cy1+512))<(512+walltol)))
		    cont=1;
		break;
	    case 2: /* North */
		if (((cy1-posys)<walltol)&&
		    (abs(posxs-(cx1+512))<(512+walltol)))
		    cont=1;
		break;
	    case 3: /* South */
		if (((posys-(cy1+1024))<walltol)&&
		    (abs(posxs-(cx1+512))<(512+walltol)))
		    cont=1;
		break;
	}

	if (wallfound[walx][waly][(int)wals]!=-1) {
	    if (!cont)
		return wallfound[walx][waly][(int)wals];
	    else
		continue;
	}
	wallfound[walx][waly][(int)wals]=wallsfound;

	wallx[wallsfound]=walx;
	wally[wallsfound]=waly;
	wallside[wallsfound]=wals;
	walnum[wallsfound]=waln;

	if ((waln&1023)==map-1) mapfound=1;
	if ((waln&1023)==gameover-1) gameoverfound=1;

	wallsfound++;
/*	if (cont)
	printf("Continuing...\n");*/
    }
    return wallsfound;
}

/* Binary division ray casting routine. */

void recurseray(K_UINT16 posxs,K_UINT16 posys,double angle,double la,double ra,
		double leftx,double lefty,double rightx,double righty) {
    
    if (angcan(ra-la)<EPSILON) return;

    if (castray(posxs,posys,angle)<0) {
        fprintf(stderr,"Warning: ray to nothing.\n");
	return;
    }
    if ((angcan(ra-la)>=M_PI/2-EPSILON)||
	(distance2(hitpointx,hitpointy,leftx,lefty)>(1.0-EPSILON)))
	recurseray(posxs,posys,(la+angle)/2.0,la,angle,
		   leftx,lefty,hitpointx,hitpointy);
    if ((angcan(ra-la)>=M_PI/2-EPSILON)||
	(distance2(hitpointx,hitpointy,rightx,righty)>(1.0-EPSILON)))
	recurseray(posxs,posys,(ra+angle)/2.0,angle, ra,
		   hitpointx,hitpointy,rightx,righty);
}

/* Darkened palette for end of game sequences... */

void openGLSetDarkenedPalette(void) {
    K_INT16 a;

    for(a=0;a<256;a++) {
	Red[a]=palette[a*3]*27/4096.0;
	Green[a]=palette[a*3+1]*27/4096.0;
	Blue[a]=palette[a*3+2]*27/4096.0;
	Alpha[a]=1.0;
    }

    if (ingame)
	Red[255]=Green[255]=Blue[255]=Alpha[255]=0.0;

    glPixelMapfv(GL_PIXEL_MAP_I_TO_R,256,Red);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G,256,Green);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B,256,Blue);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_A,256,Alpha);    
}

/* Normal palette... */

void openGLSetTransferPalette(void) {
    K_INT16 a;

    for(a=0;a<256;a++) {
	Red[a]=palette[a*3]/64.0;
	Green[a]=palette[a*3+1]/64.0;
	Blue[a]=palette[a*3+2]/64.0;
	Alpha[a]=1.0;
    }

    if (ingame)
	Red[255]=Green[255]=Blue[255]=Alpha[255]=0.0;

    glPixelMapfv(GL_PIXEL_MAP_I_TO_R,256,Red);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G,256,Green);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B,256,Blue);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_A,256,Alpha);    
}

/* Change some of the palette... */

void openGLUpdateOverlayPalette(K_UINT16 start,K_UINT16 amount,
				unsigned char *cols) {
    K_INT16 i;

    for(i=0;i<amount;i++) {
	Red[i+start]=cols[i*3]/64.0;
	Green[i+start]=cols[i*3+1]/64.0;
	Blue[i+start]=cols[i*3+2]/64.0;
	Alpha[i+start]=1.0;
    }

    if (ingame)
	Red[255]=Green[255]=Blue[255]=Alpha[255]=0.0;

    glPixelMapfv(GL_PIXEL_MAP_I_TO_R,256,Red);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G,256,Green);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B,256,Blue);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_A,256,Alpha); 
}

void openGLDrawVolumeBar(int vol,int type,float level) {
    if (level>0.5) level=0.5;
    glEnable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 360.0, -15+30*type, 225+30*type);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	
    glBegin(GL_QUADS);
    glColor4f(0,0,0,level);
    glVertex2s(96,110);
    glVertex2s(96,130);
    glColor4f(0.25,0.25,0.25,level);
    glVertex2s(224,130);
    glVertex2s(224,110);
    if (type)
	glColor4f(0,0,255,level);
    else
	glColor4f(255,0,0,level);
    glVertex2s(96,110);
    glVertex2s(96,130);
    glVertex2s(96+(vol>>1),130);
    glVertex2s(96+(vol>>1),110);
    glEnd();
    glDisable(GL_BLEND);
    checkGLStatus();
}

void openGLUploadImage(int i, unsigned char *walsegg, unsigned char create) {
    unsigned char *RGBATexture=malloc(64*64*4);

    if (create)
	glGenTextures(1,&texName[i]);
    
    glBindTexture(GL_TEXTURE_2D,texName[i]);
    checkGLStatus();

    if (bmpkind[i+1]<2) { 
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,partialfilter);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,
			fullfilter);
    } else {
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,partialfilter);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,
			fullfilter);	    
    }
    checkGLStatus();
    
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
    checkGLStatus();
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    checkGLStatus();
    
    TextureConvert(walsegg, RGBATexture, bmpkind[i+1]);
    if (debugmode)
	fprintf(stderr,"Trying to upload texture.\n");

    gluBuild2DMipmaps(GL_TEXTURE_2D,colourformat,64,64,GL_RGBA,
		      GL_UNSIGNED_BYTE,
		      RGBATexture);

    if (debugmode)
	fprintf(stderr,"Upload texture complete.\n");
    if (i==gameover-1) {
	/* Keep two copies of this; one for walls, the other for spinning
	   overlay text. */
	glGenTextures(1,&gameoversprite);
	
	glBindTexture(GL_TEXTURE_2D,gameoversprite);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,partialfilter);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,
			fullfilter);
	TextureConvert(walsegg, RGBATexture, 4);

	gluBuild2DMipmaps(GL_TEXTURE_2D,colourformat,64,64,GL_RGBA,
			  GL_UNSIGNED_BYTE,
			  RGBATexture);
    }
	
    checkGLStatus();

    free(RGBATexture);
}

#define COPYLINE \
    for(y=0;y<64;y++) { \
	*(t++)=spritepalette[(*f)*3]<<2; \
	*(t++)=spritepalette[(*f)*3+1]<<2; \
	*(t++)=spritepalette[(*f)*3+2]<<2; \
	*(t++)=255; \
	f++; \
    }

/* Create a smoother transition texture. */

void openGLTransitionTexture(int left,int texture,int right) {
    unsigned char texdata[66*64*4];

    unsigned char *f=walseg[left]+(63*64),*t=texdata;

    static int texnum=0;

    int x,y;

    COPYLINE;

    f=walseg[texture];

    for(x=0;x<64;x++) 
	COPYLINE;

    f=walseg[right];

    COPYLINE;

    for(x=0;x<2;x++) {

	glGenTextures(1,&splitTexName[texnum][x]);
	
	glBindTexture(GL_TEXTURE_2D,splitTexName[texnum][x]);
	checkGLStatus();

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,partialfilter);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,
			fullfilter);	    
	checkGLStatus();

	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
	checkGLStatus();
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	checkGLStatus();	

	/* Add code here to upload two textures from texdata, one with cols
	   0-63, other 2-65. */

	glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH,0);

	gluBuild2DMipmaps(GL_TEXTURE_2D,colourformat,64,64,GL_RGBA,
			  GL_UNSIGNED_BYTE,
			  texdata+x*512);
	
	checkGLStatus();
    }
    splitTexNum[texnum++]=texture;

}

/* Update map/game over texture... */

void openGLUpdateImage(int i) {
    unsigned char *RGBATexture=malloc(64*64*4);

    glBindTexture(GL_TEXTURE_2D,texName[i-1]);
    checkGLStatus();

    TextureConvert(walseg[i-1], RGBATexture, bmpkind[i]);

    gluBuild2DMipmaps(GL_TEXTURE_2D,colourformat,64,64,GL_RGBA,
		      GL_UNSIGNED_BYTE,
		      RGBATexture);

    checkGLStatus();
    free(RGBATexture);
}

/* Upload rectangular part of overlay from memory to specified texture. */

void UploadPartialOverlayToTexture(int x,int y,int dx,int dy,int w,int h,
				   GLuint tex,int create) {
    glBindTexture(GL_TEXTURE_2D,tex);
    checkGLStatus();
    
    glPixelStorei(GL_UNPACK_ROW_LENGTH,screenbufferwidth);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,x);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,y);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    checkGLStatus();

    if (create) {
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,partialfilter);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,partialfilter);
    }

    glPixelTransferi(GL_MAP_COLOR,GL_TRUE);

    if (debugmode)
	fprintf(stderr,"Partial overlay upload (%d %d %d %d)... ",
		w,h,dx,dy);

    if (create) {
	if (debugmode)
	    fprintf(stderr,"(create) ");
	glTexImage2D(GL_TEXTURE_2D,0,colourformat,w,
		     h,0,GL_COLOR_INDEX,
		     GL_UNSIGNED_BYTE,
		     screenbuffer);
    } else {
	glTexSubImage2D(GL_TEXTURE_2D,0,dx,dy,w,h,
			GL_COLOR_INDEX,
			GL_UNSIGNED_BYTE,
			screenbuffer);
    }
    checkGLStatus();
    if (debugmode)
	fprintf(stderr,"done.\n");
    glPixelTransferi(GL_MAP_COLOR,GL_FALSE);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
}

/* Upload rectangular part of overlay from memory to overlay texture... */

void openGLUploadPartialOverlay(int x,int y,int w,int h) {
    int left,right,top,bottom,i,j;
    int lr,rr,tr,br;

    if (menuing) return;

    if (largescreentexture) {
	/* On my nVidia Riva TNT, uploading 1 pixel high subimages is very slow
	   (driver bug?), so I upload an extra row. Very odd. Probably a driver
	   issue (nVidia driver version 1.0-1541 on Linux 2.4.4-4GB). 

	   This only seems to affect the large textures. Very odd. */

	UploadPartialOverlayToTexture(x,y,x,y,w,(h>1)?h:2,
				      screenbuffertexture,0);
    } else {
	left=(x-2)/62;
	if (left<0) left=0;
	right=(x+w-1)/62;
	if (right>5) right=5;
	top=(y-2)/62;
	if (top<0) top=0;
	bottom=(y+h-1)/62;
	if (bottom>11) bottom=11;

	for(i=top;i<=bottom;i++)
	    for(j=left;j<=right;j++) {
		lr=x-62*j;
		rr=lr+w-1;
		tr=y-62*i;
		br=tr+h-1;
		
		if (rr<0) continue;
		if (lr>63) continue;
		if (br<0) continue;
		if (tr>63) continue;

		if (lr<0) lr=0;
		if (rr>63) rr=63;
		if (tr<0) tr=0;
		if (br>63) br=63;

		UploadPartialOverlayToTexture(lr+62*j,tr+62*i,lr,tr,rr-lr+1,
					      br-tr+1,
					      screenbuffertextures[i*6+j],0);
	    }
    }
    ShowPartialOverlay(x-1,y-1,w+2,h+2,0);
}

/* Upload entire overlay from memory to texture (creates textures)... */

void openGLUploadOverlay(void) {
    int i,j;
    static int c=1;

    settransferpalette();
    if (largescreentexture)
	UploadPartialOverlayToTexture(0,0,0,0,screenbufferwidth,
				      screenbufferheight,
				      screenbuffertexture,c);
    else {
	for(i=0;i<12;i++)
	    for(j=0;j<6;j++)
		UploadPartialOverlayToTexture(62*j,62*i,0,0,64,64,
					      screenbuffertextures[i*6+j],c);
    }
    c=0;
}

/* Display rectangular part of overlay... */

void openGLShowPartialOverlay(int x,int y,int w,int h,int statusbar) {

    float tx1,tx2,ty1,ty2;
    int i,j,tr,br,lr,rr,left,right,top,bottom;

    float vl,vt1,vt2;

    if (statusbar==0) {
	y-=visiblescreenyoffset;
	if (x+w>360) w=360-x;
	if (y+h>240) h=240-y;
	if (x<0) {w+=x; x=0;}
	if (y<0) {h+=y; y=0;}
	if ((w<=0)||(h<=0)) return;
	y+=visiblescreenyoffset;
    }  

    if (mixing) 
	glEnable(GL_BLEND);
    else {
	glAlphaFunc(GL_GEQUAL,0.99);
	glEnable(GL_ALPHA_TEST);
    }
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    vl=floor(-((float)virtualscreenwidth-360.0)/2.0);
    vt1=floor(240.0+(virtualscreenheight-240.0)/2.0);
    vt2=floor(statusbaryoffset+statusbaryvisible+statusbaryoffset-y);

    if (statusbar==1)
	gluOrtho2D(vl,
		   vl+virtualscreenwidth,
		   vt2,
		   vt2-virtualscreenheight);
    else if (statusbar==2) {
	gluOrtho2D(vl+340.0-x,
		   vl+virtualscreenwidth+340.0-x,
		   vt2,
		   vt2-virtualscreenheight);
	x=340; y=statusbaryoffset;
    }
    else
	gluOrtho2D(vl,
		   vl+virtualscreenwidth,
		   vt1,
		   vt1-virtualscreenheight);

//    gluOrtho2D(0.0, 360.0, 0.0, 240.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
	
    if (largescreentexture) {
	tx1=((float)x)/(float)(screenbufferwidth);
	tx2=((float)(x+w))/(float)screenbufferwidth;
	
	ty1=((float)y)/(float)(screenbufferheight);
	ty2=((float)(y+h))/(float)screenbufferheight;

	y-=visiblescreenyoffset;
	
	glBindTexture(GL_TEXTURE_2D,screenbuffertexture);
	glBegin(GL_QUADS);
	glColor3f(redfactor,greenfactor,bluefactor);
	glTexCoord2f(tx1,ty2);
	glVertex2s(x,y+h);
	glTexCoord2f(tx2,ty2);
	glVertex2s(x+w,y+h);
	glTexCoord2f(tx2,ty1);
	glVertex2s(x+w,y);
	glTexCoord2f(tx1,ty1);
	glVertex2s(x,y);
	glEnd();
    } else {
	left=(x-1)/62;
	if (left<0) left=0;
	right=(x+w-2)/62;
	if (right>5) right=5;
	top=(y-1)/62;
	if (top<0) top=0;
	bottom=(y+h-2)/62;
	if (bottom>11) bottom=11;

//	printf("Drawing %d %d %d %d\n",x,y,w,h);

	for(i=top;i<=bottom;i++)
	    for(j=left;j<=right;j++) {
		lr=x-62*j;
		rr=lr+w-1;
		tr=y-62*i;
		br=tr+h-1;
		
		if (rr<(j>0)) continue;
		if (lr>(63-(j<5))) continue;
		if (br<(i>0)) continue;
		if (tr>(63-(i<11))) continue;

		if (lr<(j>0)) lr=(j>0);
		if (rr>(63-(j<5))) rr=63-(j<5);
		if (tr<(i>0)) tr=(i>0);
		if (br>(63-(i<11))) br=63-(i<11);

		tx1=((float)lr)/64.0;
		tx2=((float)(rr+1))/64.0;
		
		ty1=((float)tr)/64.0;
		ty2=((float)(br+1))/64.0;

		if (debugmode) {
		    fprintf(stderr,"Partial overlay display... ");
		    fprintf(stderr,"%d %d %d %d %d %d... ",i,j,lr,tr,rr,br);
		}

		glBindTexture(GL_TEXTURE_2D,screenbuffertextures[i*6+j]);
		glBegin(GL_QUADS);
		glColor3f(redfactor,greenfactor,bluefactor);
		glTexCoord2f(tx1,ty2);
		glVertex2s(lr+62*j,br+1+62*i-visiblescreenyoffset);
		glTexCoord2f(tx2,ty2);
		glVertex2s(rr+1+62*j,br+1+62*i-visiblescreenyoffset);
		glTexCoord2f(tx2,ty1);
		glVertex2s(rr+1+62*j,tr+62*i-visiblescreenyoffset);
		glTexCoord2f(tx1,ty1);
		glVertex2s(lr+62*j,tr+62*i-visiblescreenyoffset);
		glEnd();

		if (debugmode)
		    fprintf(stderr,"done.\n");

	    }
    }
    if (mixing)
	glDisable(GL_BLEND);
    else
	glDisable(GL_ALPHA_TEST);
    checkGLStatus();

    if (statusbar==1) {
	for(i=0;i<(virtualscreenwidth-319)/2;i+=20) {
	    ShowPartialOverlay(340+i,statusbaryoffset,20,statusbaryvisible,2);
	    ShowPartialOverlay(0-i,statusbaryoffset,20,statusbaryvisible,2);
	}
    }

}

/* Save screen to capturxx.bmp, where xx counts up from 00. */

void openGLScreencapture()
{
    char filename[15];
    
    unsigned char *screen=malloc(3*screenwidth*screenheight);

    char success=0;
    int file;

    K_UINT32 size=54+3*screenwidth*screenheight;

    strcpy(filename,"capturxx.bmp");
    filename[6] = (capturecount/10)+48;
    filename[7] = (capturecount%10)+48;
    capturecount++;
    if (capturecount==100)
	capturecount=0; /* Just to keep the file names sane. */

    bmpheaderwrite(2,size);

    bmpheaderwrite(0x12,screenwidth);
    bmpheaderwrite(0x16,screenheight);
	
    if (screen!=NULL) {
	glReadPixels(0,0,screenwidth,screenheight,GL_BGR,GL_UNSIGNED_BYTE,
		     screen);
	unlink(filename);
	file=open(filename,O_CREAT|O_WRONLY|O_BINARY,
		  S_IREAD|S_IWRITE|S_IRGRP|S_IROTH);
	if (file!=-1) {
	    if (write(file,BMPHeader,54)==54) {
		if (write(file,screen,size-54)==size-54) {
		    if (close(file)!=-1)
			success=1;
		}
	    }
	}
	free(screen);
    }

    if (success)
	ksay(7);
    else
	ksay(8);
}

void openGLDrawFront(void) {
    glDrawBuffer(GL_FRONT);
}

void openGLDrawBack(void) {
    glDrawBuffer(GL_BACK);
}

void openGLFlush(void) {
    if (debugmode)
      fprintf(stderr,"TIMING: Flushing buffers\n");
    glFlush();
}

/* Fade... */

void openGLFade(K_INT16 brightness)
{
    fadelevel=brightness;

    if (brightness == 63) {
	redfactor=greenfactor=bluefactor=1.0;
    }
    else if (brightness < 64) {
	redfactor=greenfactor=bluefactor=brightness/64.0;
    }
    else if (brightness < 128) {
	redfactor=greenfactor=bluefactor=(brightness-64)/64.0;
    }
    else {
	redfactor=(brightness-65)/64.0;
	greenfactor=bluefactor=1.0;

	/* Factors can't exceed 1.0, so we scale them all (makes the hurting
	   a bit darker, but a lot easier/faster). */

	greenfactor/=redfactor;
	bluefactor/=redfactor;
	redfactor=1.0;

    }
}

/* Draw an ingame view, as seen from (posxs,posys,poszs) in direction
   angs. */

void openGLPicrot(K_UINT16 posxs, K_UINT16 posys, K_INT16 poszs, K_INT16 angs)
{
    unsigned char shadecoffs;
    K_INT16 i, j, k;
    K_INT16 yy, temp;
    K_INT32 x1, y1, x2, y2;

    double hpx1,hpy1;

    GLdouble xmin,xmax,ymin,ymax;

    double angl, angr;
    double vangw;
    
    wallsfound=0;
    mapfound=0;
    gameoverfound=0;

    memset(tempbuf, 0, 4096);

    memset(wallfound,255,32768);

    rayscast=0;

    vangw=atan(aspw);

    angl=angs/1024.0*M_PI-vangw;
    angr=angs/1024.0*M_PI+vangw;

    if (castray(posxs,posys,angr)<0)
	fprintf(stderr,"Warning: ray to nothing.\n");
    hpx1=hitpointx;
    hpy1=hitpointy;

    if (castray(posxs,posys,angl)<0)
	fprintf(stderr,"Warning: ray to nothing.\n");

    if ((angcan(angr-angl)>=M_PI/2-EPSILON)||
	(distance2(hitpointx,hitpointy,hpx1,hpy1)>(1.0-EPSILON)))
	recurseray(posxs,posys,angs/1024.0*M_PI,angl,
		   angr,hitpointx,hitpointy,hpx1,hpy1);
	
    //    fprintf(stderr,"Rays cast: %d\n",rayscast);
    
    if (vidmode == 0) {
	yy = 9000;
    }
    else {
	yy = 10800;
    }

    /* These two textures change all the time, but we don't want to waste time
       uploading invisible changes... */

    if (lab3dversion==0) {
	if (mapfound)
	    gfxUpdateImage(map);
	if (gameoverfound)
	    gfxUpdateImage(gameover);
    }

    /* I suppose it could be faster on some systems to do tricks with the
       viewport rather than just draw everything that goes under the status
       bar. Perhaps... later. */

    glDisable(GL_LIGHTING);
    /* Draw floor and roof (save time by clearing to one of them, and drawing
       only one rectangle)... */

    if (lab3dversion)
	glClearColor( palette[0x85*3]/64.0*redfactor, 
		      palette[0x85*3+1]/64.0*greenfactor, 
		      palette[0x85*3+2]/64.0*bluefactor, 0 );
    else
	glClearColor( palette[0x84*3]/64.0*redfactor, 
		      palette[0x84*3+1]/64.0*greenfactor, 
		      palette[0x84*3+2]/64.0*bluefactor, 0 );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, (GLfloat)360, 0.0, (GLfloat)240);
    
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity( );

    glDisable(GL_DEPTH_TEST);
    glDepthMask(0);


    glBegin(GL_QUADS);

    glColor3f(palette[0xe3*3]/64.0*redfactor,
	      palette[0xe3*3+1]/64.0*greenfactor,
	      palette[0xe3*3+2]/64.0*bluefactor);
    glVertex3i(0,240,0);
    glVertex3i(0,240-yy/90,0);
    glVertex3i(360,240-yy/90,0);
    glVertex3i(360,240,0);      
    glEnd();

    checkGLStatus();

    /* Switch to labyrinth view transformations. */

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
  
    xmax = neardist;
    xmin = -xmax;
 
    ymin = xmin * 0.75;
    ymax = -ymin;
 
    xmax *= aspw; xmin *= aspw;
    ymax *= asph; ymin *= asph;
    
    glFrustum(xmin, xmax, ymin, ymax, neardist, 98304.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(posxs, posys, poszs*16.0, 
	      posxs+sintable[(angs+512)&2047], posys+sintable[angs], 
	      poszs*16.0,
	      0.0,0.0,-1.0);	

    glEnable(GL_DEPTH_TEST);
    glDepthMask(1);
    /* Draw solid walls... */

//    printf("Walls found: %d\n",wallsfound);

    for(i=0;i<wallsfound;i++)
    {
	shadecoffs=(walnum[i]>>13)&2;

	if ((walnum[i]&1023) == map-1)
	    shadecoffs = 0;

	j=walnum[i]&1023;

	if (wallx[i]<0) continue;

	x1=((K_INT32)wallx[i])<<10;
	y1=((K_INT32)wally[i])<<10;

	switch(wallside[i]) {
	    case 0: /* West */
		x2=x1;
		y2=y1+1024;
		break;
	    case 1: /* East */
		x1+=1024;
		x2=x1;
		y2=y1;
		y1+=1024;
		break;
	    case 2: /* North */
		y2=y1;
		x2=x1;
		x1+=1024;
		break;
	    case 3: /* South */
		y1+=1024;
		y2=y1;
		x2=x1+1024;
		break;
	    default:
		/* Can't happen. I hope. */
		x2=x1;
		y2=y1;
		break;
	}
	
	glEnable(GL_TEXTURE_2D);
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	if (lab3dversion)
	    k=numsplits;
	else
	    for(k=0;k<numsplits;k++)
		if (splitTexNum[k]==j) break;

	if (k<numsplits) {
	    glBindTexture(GL_TEXTURE_2D,splitTexName[k][0]); 
	    glBegin(GL_QUADS);
	    if (shadecoffs) {
		glColor3f(redfactor,greenfactor,bluefactor);
	    }
	    else {
		glColor3f(0.9*redfactor,0.9*greenfactor,0.9*bluefactor);
	    }

	    glTexCoord2f(0.0,1.0/64.0);
	    glVertex3i(x1,y1,0);
	    glTexCoord2f(1.0,1.0/64.0);
	    glVertex3i(x1,y1,1024);
	    glTexCoord2f(1.0,33.0/64.0);
	    glVertex3i((x1+x2)>>1,(y1+y2)>>1,1024);
	    glTexCoord2f(0.0,33.0/64.0);
	    glVertex3i((x1+x2)>>1,(y1+y2)>>1,0);      
	    glEnd();

	    glBindTexture(GL_TEXTURE_2D,splitTexName[k][1]); 
	    glBegin(GL_QUADS);
	    glTexCoord2f(0.0,31.0/64.0);
	    glVertex3i((x1+x2)>>1,(y1+y2)>>1,0);
	    glTexCoord2f(1.0,31.0/64.0);
	    glVertex3i((x1+x2)>>1,(y1+y2)>>1,1024);
	    glTexCoord2f(1.0,63.0/64.0);
	    glVertex3i(x2,y2,1024);
	    glTexCoord2f(0.0,63.0/64.0);
	    glVertex3i(x2,y2,0);      
	    glEnd();

	} else {
	    if (j == invisible-1)
		glEnable(GL_BLEND);
	    glBindTexture(GL_TEXTURE_2D,texName[j]); 
	    glBegin(GL_QUADS);
	    if (j == invisible-1)
		glColor4f(1.0,1.0,1.0,0.0); /* Must draw invisible walls
					       to make stuff behind invisible;
					       see board 15. */
	    else {
		if (shadecoffs) {
		    glColor3f(redfactor,greenfactor,bluefactor);
		}
		else {
		    glColor3f(0.9*redfactor,0.9*greenfactor,0.9*bluefactor);
		}
	    }
	    glTexCoord2f(0.0,0.0);
	    glVertex3i(x1,y1,0);
	    glTexCoord2f(1.0,0.0);
	    glVertex3i(x1,y1,1024);
	    glTexCoord2f(1.0,1.0);
	    glVertex3i(x2,y2,1024);
	    glTexCoord2f(0.0,1.0);
	    glVertex3i(x2,y2,0);      
	    glEnd();
	    if (j == invisible-1)
		glDisable(GL_BLEND);
	}
	checkGLStatus();
    }

    updatebullets(posxs, posys, poszs);

    glDepthMask(0);

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
    glDepthMask(1);
    glDisable(GL_LIGHTING);
    ShowStatusBar();
}

/* Draw a sprite at (x,y) in the labyrinth, twisted ang round its Z axis
   (warps/fans only!), oriented to be facing the player who is looking in
   direction playerang. Sprite has texture number walnume-1. */

void openGLFlatsprite(K_UINT16 x, K_UINT16 y,K_INT16 ang,K_INT16 playerang,
		      K_INT16 walnume) {

    K_INT32 x1,y1,x2,y2;
    K_INT32 xoff,yoff; 

    yoff=sintable[(playerang+512)&2047]>>7;
    xoff=sintable[(playerang+1024)&2047]>>7;

    x1=x-xoff;
    x2=x+xoff;
    y1=y-yoff;
    y2=y+yoff;
    
    glEnable(GL_DEPTH_TEST);

    glDisable(GL_LIGHTING);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();
    glTranslatef((x1+x2)/2.0,(y1+y2)/2.0,512);
    glRotatef(ang/2048.0*360.0,sintable[(playerang+512)&2047],
	      sintable[playerang],0.0);

    glTranslatef(-(x1+x2)/2.0,-(y1+y2)/2.0,-512);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D,texName[walnume-1]); 
    glBegin(GL_QUADS);
    glColor3f(redfactor,greenfactor,bluefactor);
    glTexCoord2f(0.0,0.0);
    glVertex3i(x1,y1,0);
    glTexCoord2f(1.0,0.0);
    glVertex3i(x1,y1,1024);
    glTexCoord2f(1.0,1.0);
    glVertex3i(x2,y2,1024);
    glTexCoord2f(0.0,1.0);
    glVertex3i(x2,y2,0);      
    glEnd();
    checkGLStatus();
    glPopMatrix();

    glDisable(GL_BLEND);

}

/* Draw wall number walnume-1, centred at (x,y), magnified siz>>8 times
   (i.e. at a size of siz>>2*siz>>2 pixels), rotated clockwise (2048 is
   full rotation). Colour 255 is transparent. Clip to y=[0, dside[. 
   Z-Buffering done using height[] (which contains height of object in each
   column and can be considered a 1D inverse Z-buffer (alternatively add
   a Z parameter?). */

void openGLPictur(K_INT16 x,K_INT16 y,K_INT16 siz,K_INT16 ang,K_INT16 walnume)
{
    y+=spriteyoffset;

    glDisable(GL_DEPTH_TEST);

    glDisable(GL_LIGHTING);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-(virtualscreenwidth-360)/2,
	       360+(virtualscreenwidth-360)/2,
	       -(virtualscreenheight-240)/2,
	       240+(virtualscreenheight-240)/2);
//    gluOrtho2D(0.0, (GLfloat)360, 0.0, (GLfloat)240);
    
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity( );
    glTranslatef(x,240.0-y,0.0);
    glScalef(siz/256.0,siz/256.0,siz/256.0);
    glRotatef(((GLfloat)ang)/2048.0*360.0,0.0,0.0,1.0);
    glTranslatef(-32.0,-32.0,0.0);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    if (walnume==gameover)
	glBindTexture(GL_TEXTURE_2D,gameoversprite); /* Horrible kludge. */
    else
	glBindTexture(GL_TEXTURE_2D,texName[walnume-1]);
    glBegin(GL_QUADS);
    glColor3f(redfactor,greenfactor,bluefactor);
    glTexCoord2f(1.0,0.0);
    glVertex3f(0.0,0.0,0);
    glTexCoord2f(1.0,1.0);
    glVertex3f(64.0,0.0,0);
    glTexCoord2f(0.0,1.0);
    glVertex3f(64.0,64.0,0);
    glTexCoord2f(0.0,0.0);
    glVertex3f(0.0,64.0,0);      
    glEnd();
    checkGLStatus();

    glDisable(GL_BLEND);
}

/* Draw wall number walnume-1 at board position (x,y) (multiply by
   1024 to get a value compatible with player co-ordinates), as seen from
   (posxs, posys, poszs) looking in direction angs (0-4095).
   board[x][y]&8192 indicates the direction in which the door points
   (extends over x (0) or over y (1). */
    
void openGLDoordraw(K_UINT16 x,K_UINT16 y,K_INT16 walnume,K_UINT16 posxs,
		    K_UINT16 posys)
{
    K_INT32 x1, y1, x2, y2;

    x1=((K_INT32)x);
    y1=((K_INT32)y);

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
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D,texName[walnume-1]); 
    glBegin(GL_QUADS);
    glColor3f(redfactor,greenfactor,bluefactor);
    glTexCoord2f(0.0,0.0);
    glVertex3i(x1,y1,0);
    glTexCoord2f(1.0,0.0);
    glVertex3i(x1,y1,1024);
    glTexCoord2f(1.0,1.0);
    glVertex3i(x2,y2,1024);
    glTexCoord2f(0.0,1.0);
    glVertex3i(x2,y2,0);      
    glEnd();
    glDisable(GL_BLEND);
    checkGLStatus();
	
}

void openGLFilter(unsigned char filter) {
    if (filter) {
	fullfilter=GL_LINEAR_MIPMAP_LINEAR;
	partialfilter=GL_LINEAR;
    } else {
	fullfilter=partialfilter=GL_NEAREST;
    }
}

void openGLTextureDepth(int depth) {
   switch(depth) {
       case 1:
	   colourformat=GL_RGBA8;
	   break;
       case 2:
	   colourformat=GL_RGBA4;
	   break;
       default:
	   colourformat=GL_RGBA;
   }
}

void openGLInitFailsafe(void) {
    SDL_Surface *screen, *icon;

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_RED_SIZE,0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_GREEN_SIZE,0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_BLUE_SIZE,0);
    SDL_GL_SetAttribute(SDL_GL_ACCUM_ALPHA_SIZE,0);
    SDL_ShowCursor(0);

    fprintf(stderr,"Activating video (OpenGL failsafe)...\n");

    screenwidth=360; screenheight=240;

    icon=SDL_LoadBMP("ken.bmp");
    if (icon==NULL) {
	fprintf(stderr,"Warning: ken.bmp (icon file) not found.\n");
    }
    SDL_WM_SetIcon(icon,NULL);
    if ((screen=SDL_SetVideoMode(screenwidth, screenheight, 32, 
				 SDL_OPENGL))==
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

    if (largescreentexture) {
	/* One large 512x512 texture. */

	screenbufferwidth=screenbufferheight=512;
    } else {
	/* 6*11 matrix of 64x64 tiles with 1 pixel wide borders on shared
	   edges. */

	screenbufferwidth=374;
	screenbufferheight=746;
    }

    screenbuffer=malloc(screenbufferwidth*screenbufferheight);    
    SDL_WM_SetCaption("Ken's Labyrinth", "Ken's Labyrinth");
    if (largescreentexture) {
	glGenTextures(1,&screenbuffertexture);
    } else {
	glGenTextures(72,screenbuffertextures);
    }

}

