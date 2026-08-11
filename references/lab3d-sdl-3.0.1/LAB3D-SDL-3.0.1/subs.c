#include "lab3d.h"
#include "adlibemu.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "SDL_endian.h"

/* Various constants that really should be stored in the data files... */

#define giflen1 7649
#define giflen2 8258

/* Stack size in LZW. Probably overkill. */

#define LZW_STACK_SIZE 4096

/* Last time tick handler was called... */
static Uint32 lastTick=0;

/* Fractional part of milliseconds. */
static Uint32 tickFrac=0;

/* Sequencer buffer. Completely overkill, unless Ken wants to play hundreds of
   notes at once. */

#ifdef USE_OSS
SEQ_DEFINEBUF (2048);
#endif

static char BADNAME[13]="MESTINXBADLY";

#ifdef USE_OSS
/* Linux sequencer write routine... */

void seqbuf_dump ()
{
    if (_seqbufptr)
	if (write (sequencerdevice, _seqbuf, _seqbufptr) == -1)
	{
	    fprintf (stderr,"write /dev/sequencer failed.\n");
	    SDL_Quit();
	    exit (-1);
	}
    _seqbufptr = 0;
}
#endif

/* Endian-converting reads. */

#if SDL_BYTEORDER != SDL_LIL_ENDIAN  
#define SWAPBLOCK16 for(a=0;a<(cnt>>1);a++) *(b+a)=SDL_Swap16(*(b+a));
#define SWAPBLOCK32 for(a=0;a<(cnt>>2);a++) *(b+a)=SDL_Swap32(*(b+a));

ssize_t readLE16(int fd, void *buf, size_t cnt) {
    ssize_t r=read(fd,buf,cnt);
    K_UINT16 *b=buf;
    int a;
    
    SWAPBLOCK16;
    return r;
} 
ssize_t readLE32(int fd, void *buf, size_t cnt) {
    ssize_t r=read(fd,buf,cnt);
    K_UINT32 *b=buf;
    int a;
        
    SWAPBLOCK32;
    return r;
} 

ssize_t writeLE16(int fd, void *buf, size_t cnt) {
    ssize_t r;
    K_UINT16 *b=buf;
    int a;
    
    SWAPBLOCK16;
    r=write(fd,buf,cnt);
    SWAPBLOCK16;
    
    return r;
} 
ssize_t writeLE32(int fd, void *buf, size_t cnt) {
    ssize_t r;
    K_UINT32 *b=buf;
    int a;
        
    SWAPBLOCK32;
    r=write(fd,buf,cnt);
    SWAPBLOCK32;
    return r;
} 
#endif

K_INT32 readlong(unsigned char *t) {
    return((*t)|((*(t+1))<<8)|((*(t+2))<<16)|((*(t+3))<<24));
}

void writelong(unsigned char *t,K_INT32 v) {
    *t=v&255;
    *(t+1)=(v>>8)&255;
    *(t+2)=(v>>16)&255;
    *(t+3)=(v>>24)&255;
}

K_UINT16 readshort(unsigned char *t) {
    return((*t)|((*(t+1))<<8));
}

void writeshort(unsigned char *t,K_UINT16 v) {
    *t=v&255;
    *(t+1)=(v>>8)&255;
}

/* Load a board. Uses LZW compression. Get a lawyer. */

void loadboard()
{
    unsigned char bitcnt, numbits;
    K_INT16 i, j, k, fil, bytecnt1, bytecnt2;
    K_INT16 currstr, strtot, compleng, dat, goalstr;
    K_INT32 templong;
    K_INT16 prepdie=0;

    K_UINT16 stack[LZW_STACK_SIZE];
    K_UINT16 stackp=0;

#if SDL_BYTEORDER != SDL_LIL_ENDIAN  
    int a; K_UINT16 *b; size_t cnt;
#endif

    if (lab3dversion) {
	if (((fil = open("boards.dat",O_RDONLY|O_BINARY,0)) != -1)||
	    ((fil = open("BOARDS.DAT",O_RDONLY|O_BINARY,0)) != -1)) {
	    lseek(fil,((long)boardnum)<<13,SEEK_SET);
	    read(fil,&board[0],8192);
	    close(fil);
	    numwarps=0;
	    justwarped=0;
	} else {
	    fprintf(stderr,"Can't find boards.dat.\n");
	    SDL_Quit();
	    exit(1);	    
	}
    } else {
	if (((fil = open("boards.kzp",O_RDONLY|O_BINARY,0)) != -1)||
	    ((fil = open("BOARDS.KZP",O_RDONLY|O_BINARY,0)) != -1))
	{
	    prepdie = 0;
	    numwarps = 0;
	    justwarped = 0;
	    readLE16(fil,&boleng[0],30*4);
	    templong = (long)(30*4);
	    for(i=0;i<(boardnum<<1);i++)
		templong += ((long)(boleng[i]+2));
	    lseek(fil,templong,SEEK_SET);
	    
	    for(i=1;i<=256;i++) {
		lzwbuf[i]=i&255;
		lzwbuf2[i]=i;
	    }
	    lzwbuf2[0]=0;
	    lzwbuf[0]=0;
	    
	    for(i=0;i<2;i++)
	    {
		compleng = boleng[(boardnum<<1)+i];
		readLE16(fil,&strtot,2);
		read(fil,&tempbuf[0],compleng);
		
		if (strtot > 0)
		{
		    tempbuf[compleng] = 0;
		    tempbuf[compleng+1] = 0;
		    tempbuf[compleng+2] = 0;
		    bytecnt2 = 0;
		    bytecnt1 = 0;
		    bitcnt = 0;
		    currstr = 256;
		    goalstr = 512;
		    numbits = 9;
		    do
		    {		    
			dat=(((tempbuf[bytecnt2])|
			      ((K_UINT16)(tempbuf[bytecnt2+1]))<<8)>>bitcnt)&
			    ((1<<numbits)-1);    
			
/*		    dat=((*((K_UINT16 *)(tempbuf+bytecnt2)))
		    >>bitcnt)&
		    ((1<<numbits)-1);*/
			if (bitcnt+numbits>16) {
			    dat+=(((K_UINT16)tempbuf[bytecnt2+2])&
				  ((1<<((bitcnt+numbits)&15))-1))<<
				(16-bitcnt);
			}
			
			bitcnt+=numbits;
			bytecnt2+=bitcnt>>3;
			bitcnt&=7;
			
			lzwbuf2[currstr]=dat;
			
			while(dat>=256) {
			    stack[stackp++]=lzwbuf[dat];
			    
			    dat=lzwbuf2[dat];
			}
			
			lzwbuf[currstr-1]=dat;
			lzwbuf[currstr]=dat;
			
			dat=lzwbuf2[dat];
			stack[stackp++]=dat;
			
			while(stackp>0) {
			    stackp--;
			    if (bytecnt1<4096)
				((unsigned char *)(board[32*i]))[bytecnt1++]=
				    stack[stackp];
			}
			
			currstr++;
			if (currstr == goalstr)
			{
			    numbits++;
			    goalstr = (goalstr<<1);
			}
		    }
		    while (currstr <= strtot);
		}
		else {
		    memcpy(board[32*i], tempbuf, 4096);
		}
	    }
	    close(fil);
	} else {
	    fprintf(stderr,"Can't find boards.kzp.\n");
	    SDL_Quit();
	    exit(1);
	}
	
    }
#if SDL_BYTEORDER != SDL_LIL_ENDIAN  
    b=(K_UINT16 *)board;
    cnt=8192;
    SWAPBLOCK16;
#endif

    /* Generate map texture... */
    
    for(i=0;i<4096;i++)
	walseg[map-1][i]=board[0][i]&255;
    
    if (lab3dversion) {
	spritepalette[0]=63;
	spritepalette[1]=63;
	spritepalette[2]=63;
    }

    i=bmpkind[map];    
    bmpkind[map]=2;
    gfxUploadImage(map-1, walseg[map-1], 0);
    bmpkind[map]=i;

    /* Place warps and monsters... */
    
    mnum = 0;
    if (lab3dversion)
	for(i=0;i<63;i++)
	    for(j=0;j<63;j++)
	    {
		mboard[i][j] = 0;
		k = board[i][j]&1023;
		if ((k == 123) && (numwarps < 16))
		{
		    xwarp[(int)numwarps] = (char)i;
		    ywarp[(int)numwarps] = (char)j;
		    numwarps++;
		}
		if ((k == 66) || (k == 38) || (k == 54) || (k == 68) || (k == 94) || (k == 98) || (k == 109) || (k == 160) || (k == 165) || (k == 187))
		{
		    mposx[mnum] = (i<<10)+512;
		    mposy[mnum] = (j<<10)+512;
		    mgolx[mnum] = mposx[mnum];
		    mgoly[mnum] = mposy[mnum];
		    moldx[mnum] = mposx[mnum];
		    moldy[mnum] = mposy[mnum];
		    mstat[mnum] = board[i][j]&255;
		    mshock[mnum] = 0;
		    if (k == 66)
			mshot[mnum] = 1;
		    if (k == 68)
			mshot[mnum] = 255;
		    if (k == 38)
			mshot[mnum] = 2;
		    if (k == 54)
			mshot[mnum] = 5;
		    if (k == 94)
		    {
			mshot[mnum] = 10;
			prepdie |= 2;
		    }
		    if (k == 98)
			mshot[mnum] = 255;
		    if (k == 109)
		    {
			mshot[mnum] = 100;
			prepdie |= 1;
		    }
		    if (k == 160)
			mshot[mnum] = 1;
		    if (k == 165)
			mshot[mnum] = 1;
		    if (k == 187)
			mshot[mnum] = 1;
		    mnum++;
		    mboard[i][j] = k;
		    board[i][j] = 1024;
		}
		if ((board[i][j]&4096) > 0)
		{
		    posx = (i<<10)+512;
		    posy = (j<<10)+512;
		    ang = ((board[i][j]&3)<<9);
		    startx = posx;
		    starty = posy;
		    startang = ang;
		    board[i][j] = 62+1024;
		}
	    }
    else
	for(i=0;i<63;i++)
	    for(j=0;j<63;j++)
	    {
		board[i][j] &= 0xbfff;
		k = board[i][j]&1023;
		if ((k == warp) && (numwarps < 16))
		{
		    xwarp[(int)numwarps] = (char)i;
		    ywarp[(int)numwarps] = (char)j;
		    numwarps++;
		}
		if ((k == monken) || (k == mongre) || (k == monand) ||
		    (k == monbal) || (k == monali) || (k == monhol) ||
		    (k == monzor) || (k == monbat) || (k == mongho) ||
		    (k == monske) || (k == monke2) || (k == monan2) ||
		    (k == monan3) || (k == monwit) || (k == monbee) ||
		    (k == monspi) || (k == mongr2) || (k == monear) ||
		    (k == monrob) || (k == monro2) || (k == mondog) ||
		    (k == monmum) || (k == hive))
		{
		    mposx[mnum] = (i<<10)+512;
		    mposy[mnum] = (j<<10)+512;
		    mgolx[mnum] = mposx[mnum];
		    mgoly[mnum] = mposy[mnum];
		    moldx[mnum] = mposx[mnum];
		    moldy[mnum] = mposy[mnum];
		    mstat[mnum] = k;
		    mshock[mnum] = 0;
		    if (k == monken) mshot[mnum] = 1;
		    if (k == monbal) mshot[mnum] = 255;
		    if (k == mongre) mshot[mnum] = 2;
		    if (k == mongr2) mshot[mnum] = 3;
		    if (k == monrob) mshot[mnum] = 5;
		    if (k == monwit) mshot[mnum] = 1;
		    if (k == monand) mshot[mnum] = 5;
		    if (k == monali) mshot[mnum] = 10, prepdie |= 2;
		    if (k == monhol) mshot[mnum] = 255;
		    if (k == monzor) mshot[mnum] = 47, prepdie |= 1;
		    if (k == monbat) mshot[mnum] = 1;
		    if (k == monear) mshot[mnum] = 1;
		    if (k == monbee) mshot[mnum] = 1;
		    if (k == monspi) mshot[mnum] = 1;
		    if (k == mongho) mshot[mnum] = 1;
		    if (k == monske) mshot[mnum] = 1;
		    if (k == monmum) mshot[mnum] = 2;
		    if (k == monke2) mshot[mnum] = 47, prepdie |= 1;
		    if (k == monan2) mshot[mnum] = 47, prepdie |= 1;
		    if (k == monan3) mshot[mnum] = 1;
		    if (k == hive) mshot[mnum] = 15;
		    if (k == monro2) mshot[mnum] = 2;
		    if (k == mondog) mshot[mnum] = 1;
		    mnum++;
		    board[i][j] = 16384+1024;
		}
		if ((board[i][j]&4096) > 0)
		{
		    posx = (i<<10)+512;
		    posy = (j<<10)+512;
		    yourhereoldpos = ((posx>>10)<<6)+(posy>>10);
		    youarehere();
		    ang = ((board[i][j]&3)<<9);
		    startx = posx;
		    starty = posy;
		    startang = ang;
		    board[i][j] = stairtop+1024;
		}
	    }
    if ((prepdie&1) > 0)
	ksay(21);
    else if ((prepdie&2) > 0)
	ksay(20);
    posz = 32;
    angvel = 0;
    vel = 0;
    mxvel = 0;
    myvel = 0;
    svel = 0;
    hvel = 0;
    for(i=0;i<32;i++)
	bulstat[i] = 0;
    lastbulshoot = 0;
    bulnum = 0;
    keys[0] = 0;
    keys[1] = 0;
    death = 4095;
}

/* Load tables and settings. The settings are ignored. */

void loadtables()
{
    K_INT16 fil;

    if (((fil = open("tables.dat",O_RDONLY|O_BINARY,0)) != -1)||
	((fil = open("TABLES.DAT",O_RDONLY|O_BINARY,0)) != -1))
    {
	readLE32(fil,&sintable[0],8192);
	readLE32(fil,&tantable[0],4096);
	readLE16(fil,&radarang[0],720);
/*	read(fil,&option[0],numoptions);
	read(fil,&keydefs[0],numkeys);
	readLE16(fil,&joyx1,2);
	readLE16(fil,&joyy1,2);
	readLE16(fil,&joyx2,2);
	readLE16(fil,&joyy2,2);
	readLE16(fil,&joyx3,2);
	readLE16(fil,&joyy3,2);
	readLE16(fil,&ksayfreq,2);*/
	close(fil);

	/* Override joystick values with SDL limits... */

	joyx1=joyy1=-32768;
	joyx2=joyy2=0;
	joyx3=joyy3=32767;

    } else {
	fprintf(stderr,"Can't find tables.dat.\n");
	SDL_Quit();
	exit(1);
    }
}

/* ksay sound filenum. pan = 0 for left, 128 centre, 256 right. */

K_INT16 ksaypan(K_UINT16 filenum,K_UINT16 pan) {
    K_INT16 numfiles;
    K_UINT16 leng;
    K_INT32 sndfiloffs;
    K_INT32 blocksize=(musicsource==2)?SOUNDBLOCKSIZE44KHZ:SOUNDBLOCKSIZE11KHZ;

    if (!soundpan) pan=128;

    SDL_LockMutex(soundmutex);
    if ((speechstatus == 0) || (mute == 1)) {
	SDL_UnlockMutex(soundmutex);
	return(-1);
    }
    numfiles=readshort(SoundFile);
    if (filenum >= numfiles) {
	SDL_UnlockMutex(soundmutex);
	return(-1);
    }

    sndfiloffs=readlong(SoundFile+(2+filenum*6));
    leng=readshort(SoundFile+(6+filenum*6));
	    
    DumpSound(SoundFile+sndfiloffs, leng,((FeedPoint+blocksize)&65535),pan);
    SDL_UnlockMutex(soundmutex);
    return 0;
}

/* ksay sound filenum from position x,y in game. */

K_INT16 ksaystereo(K_UINT16 filenum,K_UINT16 x,K_UINT16 y) {
    K_INT32 k,m,dir,pan,templong;

    if ((x==posx)&&(y==posy)) return(ksay(filenum));

    k = 512;
    if (x != posx)
    {
	templong = (((((K_INT32)y-(K_INT32)posy)<<12)/
		     ((K_INT32)x-(K_INT32)posx))<<4);
	if (templong < 0)
	    k = 768;
	else
	    k = 256;
	for (m=128;m>0;m>>=1)
	{
	    if (tantable[k] < templong)
		k += m;
	    else
		k -= m;
	}
    }
    if (y > posy)
	k += 1024;

    dir=(k+2048-ang)&2047;
    dir^=2047;

    if (dir>=1536)
	pan=(dir-1536)>>2; /* Forward left */
    else if (dir>=512)
	pan=(1536-dir)>>2; /* Back */
    else pan=128+(dir>>2); /* Forward right */

    return ksaypan(filenum,pan);
}

/* Play a digital sound... */

K_INT16 ksay(K_UINT16 filenum)
{
    return(ksaypan(filenum,128));
}

/* Wipe digital sound buffer... */

void reset_dsp()
{
    memset(SoundBuffer,0,65536*2);
    FeedPoint=0;
}

#ifndef min
int min(long a,long b) {
    return((a<b)?a:b);
}
#endif

static long minicnt = 0;
void preparesound(void *dasnd, long numbytestoprocess)
{
    long i, prepcnt;

    prepcnt = numbytestoprocess;
    while (prepcnt > 0)
    {
	i = min(prepcnt,(minicnt/speed+4)&~3);
	adlibgetsample(dasnd,i);
	dasnd = (void *)(((long)dasnd)+i);
	prepcnt -= i;

	minicnt -= speed*i;
	while (minicnt < 0)
	{
	    minicnt += 44100*channels*2;
	    ksmhandler();
	}
    }
}

/* SDL audio callback. Feed a chunk from sound buffer. */

void AudioCallback(void *userdata, Uint8 *stream, int len) {
    int rl;
    int i;
    int t=0;
    int j1,j2;

    if (soundtimer) {
	soundtimerbytes+=len;
	SDL_LockMutex(timermutex);
	while(soundtimerbytes>=soundbytespertick) {
	    soundtimerbytes-=soundbytespertick;
	    clockspeed++;
	}
	SDL_UnlockMutex(timermutex);
    }

    SDL_LockMutex(soundmutex);

    if (musicsource==2) len>>=2;

    len>>=1;
    rl=len;

    if (FeedPoint+rl>=65536) rl=65536-FeedPoint;

    if (musicsource==2) {
	/* mute=2: stop music, but don't mute. */
	if ((mute!=1)&&musicstatus)
	    preparesound (stream, rl*2*4);
	else
	    memset(stream,0,rl*2*4);
	j1=FeedPoint;
	j2=(FeedPoint+channels)&65535;

	if (mute!=1) {
	    for(i=0;i<(len<<2);i++) {
		if (channels==1)
		    switch(i&3) {
			case 0:
			    t=SoundBuffer[j1]+((Sint16 *)stream)[i];
			    break;
			case 1:
			    t=3*(SoundBuffer[j1]>>2)+(SoundBuffer[j2]>>2)+
				((Sint16 *)stream)[i];
			    break;
			case 2:
			    t=(SoundBuffer[j1]>>1)+(SoundBuffer[j2]>>1)+
				((Sint16 *)stream)[i];
			    break;
			case 3:
			    t=(SoundBuffer[j1]>>2)+3*(SoundBuffer[j2]>>2)+
				((Sint16 *)stream)[i];
			    j1++;
			    j2++;
			    j2&=65535;
			    break;
		    }
		else
		    switch(i&7) {
			case 0:
			case 1:
			    t=SoundBuffer[j1+(i&1)]+((Sint16 *)stream)[i];
			    break;
			case 2:
			case 3:
			    t=3*(SoundBuffer[j1+(i&1)]>>2)+
				(SoundBuffer[j2+(i&1)]>>2)+
				((Sint16 *)stream)[i];
			    break;
			case 4:
			case 5:
			    t=(SoundBuffer[j1+(i&1)]>>1)+
				(SoundBuffer[j2+(i&1)]>>1)+
				((Sint16 *)stream)[i];
			    break;
			case 6:
			case 7:
			    t=(SoundBuffer[j1+(i&1)]>>2)+
				3*(SoundBuffer[j2+(i&1)]>>2)+
				((Sint16 *)stream)[i];
			    if ((i&7)==7) {
				j1+=2;
				j2+=2;
				j2&=65535;
			    }
			    break;
		    }
		if (t<-32768) t=-32768;
		if (t>32767) t=32767;
		((Sint16 *)stream)[i]=t;
	    }
	}
    }
    else
	memcpy(stream, SoundBuffer+FeedPoint, rl*2);
    memset(SoundBuffer+FeedPoint,0,rl*2);

    FeedPoint+=rl;
    FeedPoint&=65535;

    SDL_UnlockMutex(soundmutex);

    if (rl<len) {
	if (musicsource!=2)
	    AudioCallback(userdata, stream+(rl<<1), (len-rl)<<1);
	else
	    AudioCallback(userdata, stream+(rl<<3), (len-rl)<<3);
    }
}

/* Copy sound to sound buffer. */

void DumpSound(unsigned char *sound,K_UINT16 leng,K_UINT32 playpoint,int pan) {
    K_INT32 a,t;

    K_UINT32 e1,pl;

    if (channels==1) {
	e1=playpoint+leng;
	if (e1>=65536) e1=65536;
	pl=e1-playpoint;

	for(a=playpoint;a<e1;a++) {
	    t=SoundBuffer[a]+(sound[a-playpoint]-128)*soundvolume;
	    if (t<-32768) t=-32768;
	    if (t>32767) t=32767;
	    SoundBuffer[a]=t;
	}
    } else {
	e1=playpoint+(leng<<1);
	if (e1>=65536) e1=65536;
	pl=(e1-playpoint)>>1;

	for(a=playpoint;a<e1;a++) {
	    t=((a-playpoint)&1)?pan:(256-pan);
	    t=SoundBuffer[a]+
		(((sound[(a-playpoint)>>1]-128)*soundvolume*t)>>7);
	    if (t<-32768) t=-32768;
	    if (t>32767) t=32767;
	    SoundBuffer[a]=t;
	}
    }

    if (pl<leng) DumpSound(sound+pl, leng-pl, 0,pan);
}

/* Check if object is visible... */

void checkobj(K_UINT16 x,K_UINT16 y,K_UINT16 posxs,K_UINT16 posys,
	      K_INT16 angs,K_INT16 num)
{
    K_INT16 angle, siz, ysiz;

    if (((posxs|1023) != (x|1023)) || ((posys|1023) != (y|1023)))
    {
	siz = (int)(((((long)x-(long)posxs)>>2)*sintable[(angs+512)&2047]+(((long)y-(long)posys)>>2)*sintable[angs])>>16);
	if (siz != 0)
	{
	    ysiz = (int)(((((long)x-(long)posxs)>>2)*sintable[angs]-(((long)y-(long)posys)>>2)*sintable[(angs+512)&2047])>>16);
	    angle = (K_INT16)(180.0-((180.0*(double)ysiz)/(double)siz/aspw));
	    siz = (int)(163840L/((long)siz));
	    sortx[sortcnt] = x;
	    sorty[sortcnt] = y;
	    //	    sorti[sortcnt] = siz;
	    sorti[sortcnt]=163840L/sqrt((((long)x-(long)posxs)*
					 ((long)x-(long)posxs))+
					(((long)y-(long)posys)*
					 ((long)y-(long)posys)))*4;
	    sortbnum[sortcnt] = (num&1023);
	    if (((angle+(siz>>3)) >= -2) && ((angle-(siz>>3)) <= 362) && ((siz&0xc000) == 0))
		sortcnt++;
	}
    }
}

/* Set status bar position... */

void linecompare(K_UINT16 lin)
{

    statusbaryvisible=240-((lin+1)>>1);

}

/* Draw energy meter... */

void drawlife()
{
    K_INT16 lifespot, olifespot;

    if (statusbar == 479)
	return;
    if (life < 0) life = 0;
    if (life > 4095) life = 4095;
    lifespot = (life>>6);
    olifespot = (oldlife>>6);
    if (lifespot < olifespot)
	statusbardraw(lifespot,57,olifespot-lifespot+1,7,lifespot+128,4+statusbaryoffset,statusbarinfo);
    else if (lifespot > olifespot)
	statusbardraw(olifespot,50,lifespot-olifespot+1,7,olifespot+128,4+statusbaryoffset,statusbarinfo);
    oldlife = life;
}

/* Load textures. Use LZW without a license. */

void loadwalls()
{
    unsigned char bitcnt, numbits;
    K_INT16 i, j, fil, bytecnt1, bytecnt2;
#ifdef COLOURTEST
    K_INT16 k;
#endif
    K_INT16 currstr, strtot, compleng, dat, goalstr;
    unsigned char *walsegg;

    K_UINT16 stack[LZW_STACK_SIZE];
    K_UINT16 stackp=0;

    if (((fil = open("walls.kzp",O_RDONLY|O_BINARY,0)) != -1)||
	((fil = open("WALLS.KZP",O_RDONLY|O_BINARY,0)) != -1))
    {
	bmpkind[0] = 0;
	wallheader[0] = 8;
	read(fil,&wallheader[1],rnumwalls);
	if (lab3dversion==0)
	    readLE16(fil,&tileng[0],numwalls*2);
	tioffs[0] = (long)(numwalls+numwalls+numwalls);
	for(i=1;i<=rnumwalls;i++)
	{
	    tioffs[i] = tioffs[i-1]+((long)(tileng[i-1]+2));
	    bmpkind[i] = 1+(wallheader[i]&7);
	    if (bmpkind[i] == 3)
		bmpkind[i] = 4;
	}

	bmpkind[sharewaremessage]=4;

	for(i=1;i<=256;i++) {
	    lzwbuf[i]=i&255;
	    lzwbuf2[i]=i;		    
	}
	lzwbuf2[0]=0;
	lzwbuf[0]=0;

	for(i=0;i<rnumwalls;i++)
	{
	    readLE16(fil,&strtot,2);
	    if (lab3dversion)
		readLE16(fil, &compleng, 2);
	    else
		compleng = tileng[i];
	    read(fil,&tempbuf[0],compleng);

	    walsegg=walseg[i];

	    if (strtot > 0)
	    {
		tempbuf[compleng] = 0;
		tempbuf[compleng+1] = 0;
		tempbuf[compleng+2] = 0;
		bytecnt2 = 0;
		bytecnt1 = 0;
		bitcnt = 0;
		currstr = 256;
		goalstr = 512;
		numbits = 9;
		do
		{
		    dat=(((tempbuf[bytecnt2])|
			  ((K_UINT16)(tempbuf[bytecnt2+1]))<<8)>>bitcnt)&
			((1<<numbits)-1);    
/*		    dat=((*((K_UINT16 *)(tempbuf+bytecnt2)))
		    >>bitcnt)&
		    ((1<<numbits)-1);*/
		    if (bitcnt+numbits>16) {
			dat+=(((K_UINT16)tempbuf[bytecnt2+2])&
			      ((1<<((bitcnt+numbits)&15))-1))<<
			    (16-bitcnt);
		    }
				    
		    bitcnt+=numbits;
		    bytecnt2+=bitcnt>>3;
		    bitcnt&=7;
				    
		    lzwbuf2[currstr]=dat;

		    while(dat>=256) {
			stack[stackp++]=lzwbuf[dat];

			dat=lzwbuf2[dat];
		    }
				    
		    lzwbuf[currstr-1]=dat;
		    lzwbuf[currstr]=dat;
				    
		    dat=lzwbuf2[dat];
		    stack[stackp++]=dat;
				    
		    while(stackp>0) {
			stackp--;
			if (bytecnt1<4096)
			    walsegg[bytecnt1++]=stack[stackp];
		    }
		    currstr++;
		    if (currstr == goalstr)
		    {
			numbits++;
			goalstr = (goalstr<<1);
		    }
		}
		while (currstr <= strtot);
	    }
	    else
		memcpy(walsegg, tempbuf, 4096);
	    if (bmpkind[i+1] >= 2)
	    {
		j=0;
		while((j<4096)&&(walsegg[j]!=255))
		    j++;
		j&=0xfc0;
		lborder[i+1]=j;
		j=4095;
		while((j>=0)&&(walsegg[j]!=255))
		    j--;
		j&=0xfc0;
		j+=64;
		rborder[i+1]=j;
	    }
	    else
	    {
		lborder[i+1] = 0;
		rborder[i+1] = 4096;
	    }
	    if ((i < 127) && ((i&1)==0)) {
		if (debugmode)
		    fprintf(stderr,"Trying to draw screen buffer.\n");
		if (!paletterenderer)
		    gfxDrawBack();
		else
		    gfxDrawFront();
		fade(64+(i>>1));
		if ((!paletterenderer)||(i==0))
		    SetVisibleScreenOffset(0);
		else
		    gfxDrawFront();
		if (debugmode)
		    fprintf(stderr,"Screen buffer draw OK.\n");
	    } else {
		gfxDrawFront();
		
		if (i==128) {
		    fade(63);
		}
	    }

	    j=(160-(rnumwalls>>2)+i);

	    if (lab3dversion) {
		if (i < (rnumwalls>>1)) {
		    screenbuffer[screenbufferwidth*219+j]=255;		
		}
		else {
		    j-=rnumwalls>>1;
		    screenbuffer[screenbufferwidth*219+j]=0;
		}
		gfxUploadPartialOverlay(j,219,1,1);
	    } else {	    
		if (i < (rnumwalls>>1)) {
		    screenbuffer[screenbufferwidth*199+j]=255;		
		}
		else {
		    j-=rnumwalls>>1;
		    screenbuffer[screenbufferwidth*199+j]=63;
		}
		if (debugmode)
		    fprintf(stderr,"Trying to update screen buffer.\n");
		gfxUploadPartialOverlay(j,199,1,1);
		if (debugmode)
		    fprintf(stderr,"Screen buffer update OK.\n");
	    }

	    /* Use double buffer when fading, single buffer when not.
	       Yes, I know I'm too clever for my own good. */

	    if ((!paletterenderer)&&((i<127) && ((i&1)==0)))
		gfxSwapBuffers();
	    else
		gfxFlush();

	    /* Replace door1 with colour test image. */

#ifdef COLOURTEST	
	    if (i==door1-1) {
		for(j=0;j<64;j++)
		    for(k=0;k<64;k++)
			walsegg[j*64+k]=((j>>2)<<4)|(k>>2);
	    }
#endif

	    gfxUploadImage(i, walsegg, 1);

/*	    printf("Wall number %d:\n",i);

for(k=0;k<64;k++) {
for(j=0;j<64;j++) {
printf("%2x",walsegg[k+j*64]);
}
printf("\n");
}
printf("\n");*/
	}
	close(fil);
    } else {
	fprintf(stderr,"Can't find walls.kzp.\n");
	SDL_Quit();
	exit(1);
    }
//      Examine textures...
/*
    for(i=0;i<rnumwalls;i++) {
        gfxClearScreen();
	printf("Texture number %d.\n",i);
	spridraw(180,50,512,i+1);
//	gfxSwapBuffers();
	pressakey();
    }
*/

    if (lab3dversion) {
	visiblescreenyoffset=0;
	strcpy(textbuf,
	       "\"LAB3D/SDL\" conversion");
	textprint(30,222,0);

	strcpy(textbuf,
	       ".");
	textprint(260,226,0);
	textprint(264,226,0);
	strcpy(textbuf,
	       "Copyright (c) 2002-2013 Jan Lonnberg");
	textprint(30,232,0);
    }

    if (lab3dversion==0) {
	/* Set up transition textures between walls properly. */

	gfxTransitionTexture(0,1,2);
	gfxTransitionTexture(2,3,0);
	gfxTransitionTexture(6,7,8);
	gfxTransitionTexture(8,9,6);
	
	/* Set up end of game rainbow. */
	
	gfxTransitionTexture(424,425,426);
	gfxTransitionTexture(47,424,425);
	gfxTransitionTexture(425,426,47);
    }
    gfxDrawBack();
}

/* Get average of neighbouring pixels... */

int AverageColour(unsigned char *p, int x, int y,int colour) {
    int a,b,c=0,n=0;

    for(a=-1;a<=1;a++) {
	if (x+a<0) continue;
	if (x+a>63) continue;
	for(b=-1;b<=1;b++) {
	    if (y+b<0) continue;
	    if (y+b>63) continue;
	    if (p[(x+a)*64+(y+b)]!=255) {
		c+=spritepalette[p[(x+a)*64+(y+b)]*3+colour]<<2;
		n++;
	    }
	}
    }
    if (n>0) return c/n; else return 0;
}

/* Convert a texture from 8-bit indexed to 32-bit RGBA. */

void TextureConvert(unsigned char *from, unsigned char *to, K_INT16 type) {

    unsigned char *f=from,*t=to;

    int x,y;

    for(x=0;x<64;x++) 
	for(y=0;y<64;y++) {	    
	    if ((type>1)&&(*(f))==255) {
		*(t++)=AverageColour(from,x,y,0);
		*(t++)=AverageColour(from,x,y,1);
		*(t++)=AverageColour(from,x,y,2);
		*(t++)=0;		
	    } else {
		*(t++)=spritepalette[(*f)*3]<<2;
		*(t++)=spritepalette[(*f)*3+1]<<2;
		*(t++)=spritepalette[(*f)*3+2]<<2;
		*(t++)=255;
	    }
	    f++;
	}
}

/* Load a saved game... */

K_INT16 loadgame(K_INT16 gamenum)
{
    char filename[20];
    K_INT16 i;
    int fil;

    filename[0] = 'S', filename[1] = 'A', filename[2] = 'V';
    filename[3] = 'G', filename[4] = 'A', filename[5] = 'M';
    filename[6] = 'E', filename[7] = gamenum+48;
    filename[8] = '.', filename[9] = 'D', filename[10] = 'A';
    filename[11] = 'T', filename[12] = 0;
    if((fil=open(filename,O_RDONLY|O_BINARY,0))==-1) {
	filename[0] = 's', filename[1] = 'a', filename[2] = 'v';
	filename[3] = 'g', filename[4] = 'a', filename[5] = 'm';
	filename[6] = 'e', filename[7] = gamenum+48;
	filename[8] = '.', filename[9] = 'd', filename[10] = 'a';
	filename[11] = 't', filename[12] = 0;	

	if((fil=open(filename,O_RDONLY|O_BINARY,0))==-1)
	    return -1;
    }
    musicoff();
    read(fil,&hiscorenam[0],16);
    read(fil,&hiscorenamstat,1);
    readLE16(fil,&boardnum,2);

    if (boardnum >= numboards)
	quitgame = 2;
    filename[0] = 'L', filename[1] = 'A', filename[2] = 'B';
    filename[3] = 'S', filename[4] = 'N', filename[5] = 'G';
    filename[6] = (boardnum/10)+48, filename[7] = (boardnum%10)+48;
    filename[8] = 0;
    mute = 1;
    loadmusic(filename);
    musicon();

    readLE32(fil,&scorecount,4);
    readLE32(fil,&scoreclock,4);

    readLE16(fil,&board[0][0],8192);
    readLE16(fil,&skilevel,2);
    readLE16(fil,&life,2);
    readLE16(fil,&death,2);
    readLE16(fil,&lifevests,2);
    readLE16(fil,&lightnings,2);
    readLE16(fil,&firepowers[0],6);
    readLE16(fil,&bulchoose,2);
    readLE16(fil,&keys[0],4);
    readLE16(fil,&coins,2);
    readLE16(fil,&compass,2);
    readLE16(fil,&cheated,2);
    readLE16(fil,&animate2,2);
    readLE16(fil,&animate3,2);
    readLE16(fil,&animate4,2);
    readLE16(fil,&oscillate3,2);
    readLE16(fil,&oscillate5,2);
    readLE16(fil,&animate6,2);
    readLE16(fil,&animate7,2);
    readLE16(fil,&animate8,2);
    readLE16(fil,&animate10,2);
    readLE16(fil,&animate11,2);
    readLE16(fil,&animate15,2);
    readLE16(fil,&statusbar,2);
    readLE16(fil,&statusbargoal,2);
    readLE16(fil,&posx,2);
    readLE16(fil,&posy,2);
    readLE16(fil,&posz,2);
    readLE16(fil,&ang,2);
    readLE16(fil,&startx,2);
    readLE16(fil,&starty,2);
    readLE16(fil,&startang,2);
    readLE16(fil,&angvel,2);
    readLE16(fil,&vel,2);
    readLE16(fil,&mxvel,2);
    readLE16(fil,&myvel,2);
    readLE16(fil,&svel,2);
    readLE16(fil,&hvel,2);
    readLE16(fil,&oldposx,2);
    readLE16(fil,&oldposy,2);
    readLE16(fil,&bulnum,2);
    readLE16(fil,&bulang[0],bulnum<<1);
    readLE16(fil,&bulkind[0],bulnum<<1);
    readLE16(fil,&bulx[0],bulnum<<1);
    readLE16(fil,&buly[0],bulnum<<1);
    readLE32(fil,&bulstat[0],bulnum<<2);
    readLE32(fil,&lastbulshoot,4);
    readLE16(fil,&mnum,2);
    readLE16(fil,&mposx[0],mnum<<1);
    readLE16(fil,&mposy[0],mnum<<1);
    readLE16(fil,&mgolx[0],mnum<<1);
    readLE16(fil,&mgoly[0],mnum<<1);
    readLE16(fil,&moldx[0],mnum<<1);
    readLE16(fil,&moldy[0],mnum<<1);
    readLE16(fil,&mstat[0],mnum<<1);
    readLE16(fil,&mshock[0],mnum<<1);
    read(fil,&mshot[0],mnum);
    readLE16(fil,&doorx,2);
    readLE16(fil,&doory,2);
    readLE16(fil,&doorstat,2);
    read(fil,&numwarps,1);
    read(fil,&justwarped,1);
    read(fil,&xwarp[0],numwarps);
    read(fil,&ywarp[0],numwarps);
    readLE32(fil,&totalclock,4);
    ototclock = totalclock;
    readLE32(fil,&purpletime,4);
    readLE32(fil,&greentime,4);
    readLE32(fil,&capetime[0],8);
    readLE32(fil,&musicstatus,4);
    readLE16(fil,(K_INT16 *)(&clockspeed),2); /* volatile warning here. */
    readLE32(fil,&count,4);
    readLE32(fil,&countstop,4);
    readLE16(fil,&nownote,2);
    readLE16(fil,&i,2);
    readLE32(fil,&chanage,18<<2);
    read(fil,&chanfreq,18);
    readLE16(fil,&midiinst,2);
    readLE16(fil,&mute,2);
    read(fil,&namrememberstat,1);
    readLE16(fil,&fadewarpval,2);
    readLE16(fil,&fadehurtval,2);
    readLE16(fil,&slottime,2);
    readLE16(fil,&slotpos[0],6);
    readLE16(fil,&owecoins,2);
    readLE16(fil,&owecoinwait,2);
    close(fil);
    if (((i == -1) || (i > 2)) && (musicsource >= 0))
    {
	musicoff();
	loadmusic(filename);
	musicon();
    }
    totalclock -= 2;
    copyslots(slotto);
    totalclock++;
    copyslots(slotto+1);
    totalclock++;

    yourhereoldpos = ((posx>>10)<<6)+(posy>>10);

    for(i=0;i<4096;i++)
	walseg[map-1][i]=board[0][i]&255;
    walseg[map-1][yourhereoldpos]=255;

    if ((vidmode == 0) && (statusbargoal > 400))
    {
	statusbar -= 80;
	statusbargoal -= 80;
    }
    if ((vidmode == 1) && (statusbargoal < 400))
    {
	statusbar += 80;
	statusbargoal += 80;
    }
    if (vidmode == 0)
    {
	scrsize = 18000-2880;
	if (statusbar == 399)
	    scrsize += 2880;
    }
    else
    {
	scrsize = 21600-2880;
	if (statusbar == 479)
	{
	    scrsize += 2880;
	}
    }
    linecompare(statusbar);
    statusbaralldraw();
    if (compass>0) showcompass(ang);
    fade(63);
    return 0;
}

/* Save game... */

K_INT16 savegame(K_INT16 gamenum)
{
    char filename[20];
    int i, fil;

    /* If we have a lower case copy of this save game, destroy it. */

    filename[0] = 's', filename[1] = 'a', filename[2] = 'v';
    filename[3] = 'g', filename[4] = 'a', filename[5] = 'm';
    filename[6] = 'e', filename[7] = gamenum+48;
    filename[8] = '.', filename[9] = 'd', filename[10] = 'a';
    filename[11] = 't', filename[12] = 0;	

    unlink(filename);

    filename[0] = 'S', filename[1] = 'A', filename[2] = 'V';
    filename[3] = 'G', filename[4] = 'A', filename[5] = 'M';
    filename[6] = 'E', filename[7] = gamenum+48;
    filename[8] = '.', filename[9] = 'D', filename[10] = 'A';
    filename[11] = 'T', filename[12] = 0;
    if((fil=open(filename,O_CREAT|O_WRONLY|O_BINARY,
		 S_IWRITE|S_IREAD|S_IRGRP|S_IROTH))==-1) {
	return(-1);
    }
    write(fil,&hiscorenam[0],16);
    write(fil,&hiscorenamstat,1);
    writeLE16(fil,&boardnum,2);
    writeLE32(fil,&scorecount,4);
    writeLE32(fil,&scoreclock,4);

    for(i=0;i<16;i++)
	gamehead[gamenum][i] = hiscorenam[i];
    gamehead[gamenum][16] = hiscorenamstat;

    writeshort(&(gamehead[gamenum][17]),boardnum);
    writelong(&(gamehead[gamenum][19]),scorecount);
    writelong(&(gamehead[gamenum][23]),scoreclock);

    gamexist[gamenum] = 1;
    writeLE16(fil,&board[0][0],8192);
    writeLE16(fil,&skilevel,2);
    writeLE16(fil,&life,2);
    writeLE16(fil,&death,2);
    writeLE16(fil,&lifevests,2);
    writeLE16(fil,&lightnings,2);
    writeLE16(fil,&firepowers[0],6);
    writeLE16(fil,&bulchoose,2);
    writeLE16(fil,&keys[0],4);
    writeLE16(fil,&coins,2);
    writeLE16(fil,&compass,2);
    writeLE16(fil,&cheated,2);
    writeLE16(fil,&animate2,2);
    writeLE16(fil,&animate3,2);
    writeLE16(fil,&animate4,2);
    writeLE16(fil,&oscillate3,2);
    writeLE16(fil,&oscillate5,2);
    writeLE16(fil,&animate6,2);
    writeLE16(fil,&animate7,2);
    writeLE16(fil,&animate8,2);
    writeLE16(fil,&animate10,2);
    writeLE16(fil,&animate11,2);
    writeLE16(fil,&animate15,2);
    writeLE16(fil,&statusbar,2);
    writeLE16(fil,&statusbargoal,2);
    writeLE16(fil,&posx,2);
    writeLE16(fil,&posy,2);
    writeLE16(fil,&posz,2);
    writeLE16(fil,&ang,2);
    writeLE16(fil,&startx,2);
    writeLE16(fil,&starty,2);
    writeLE16(fil,&startang,2);
    writeLE16(fil,&angvel,2);
    writeLE16(fil,&vel,2);
    writeLE16(fil,&mxvel,2);
    writeLE16(fil,&myvel,2);
    writeLE16(fil,&svel,2);
    writeLE16(fil,&hvel,2);
    writeLE16(fil,&oldposx,2);
    writeLE16(fil,&oldposy,2);
    writeLE16(fil,&bulnum,2);
    writeLE16(fil,&bulang[0],bulnum<<1);
    writeLE16(fil,&bulkind[0],bulnum<<1);
    writeLE16(fil,&bulx[0],bulnum<<1);
    writeLE16(fil,&buly[0],bulnum<<1);
    writeLE32(fil,&bulstat[0],bulnum<<2);
    writeLE32(fil,&lastbulshoot,4);
    writeLE16(fil,&mnum,2);
    writeLE16(fil,&mposx[0],mnum<<1);
    writeLE16(fil,&mposy[0],mnum<<1);
    writeLE16(fil,&mgolx[0],mnum<<1);
    writeLE16(fil,&mgoly[0],mnum<<1);
    writeLE16(fil,&moldx[0],mnum<<1);
    writeLE16(fil,&moldy[0],mnum<<1);
    writeLE16(fil,&mstat[0],mnum<<1);
    writeLE16(fil,&mshock[0],mnum<<1);
    write(fil,&mshot[0],mnum);
    writeLE16(fil,&doorx,2);
    writeLE16(fil,&doory,2);
    writeLE16(fil,&doorstat,2);
    write(fil,&numwarps,1);
    write(fil,&justwarped,1);
    write(fil,&xwarp[0],numwarps);
    write(fil,&ywarp[0],numwarps);
    writeLE32(fil,&totalclock,4);
    writeLE32(fil,&purpletime,4);
    writeLE32(fil,&greentime,4);
    writeLE32(fil,&capetime[0],8);
    writeLE32(fil,&musicstatus,4);
    writeLE16(fil,(K_INT16 *)(&clockspeed),2);
    writeLE32(fil,&count,4);
    writeLE32(fil,&countstop,4);
    writeLE16(fil,&nownote,2);
    writeLE16(fil,&musicsource,2);
    writeLE32(fil,&chanage,18<<2);
    write(fil,&chanfreq,18);
    writeLE16(fil,&midiinst,2);
    writeLE16(fil,&mute,2);
    write(fil,&namrememberstat,1);
    writeLE16(fil,&fadewarpval,2);
    writeLE16(fil,&fadehurtval,2);
    writeLE16(fil,&slottime,2);
    writeLE16(fil,&slotpos[0],6);
    writeLE16(fil,&owecoins,2);
    writeLE16(fil,&owecoinwait,2);
    close(fil);
    ksay(16);
    return 0;
}

/* Common variables between introduction() and drawintroduction().
   Messy, but works. */

static K_INT32 dalasti, pickskiltime;
static K_INT16 animater2,sharemessplc, newgamepisode;
static K_UINT16 introplc;

/* Draw introduction frame... */

void drawintroduction() {
    K_INT32 dai=totalclock>>2;
    K_INT16 m;

    if ((dai >= 0) && (dai < 300))
    {
	introplc = 5+times90[dai>>2];
	if (introplc > 3785)
	    introplc = 3785;

	SetVisibleScreenOffset(introplc);

	if (dai < 240)
	{
	    pictur(180+(240-((int)dai)),164,64*4,((((unsigned)dai)<<8)/15)&2047,copyright);
	    pictur(180-(240-((int)dai)),164,64*4,(((((unsigned)dai)<<8)/15)&2047)^2047,copyright);
	}
	else
	{
	    if (vidmode == 0)
		m = 20;
	    else
		m = 0;
	    statusbardraw(0,44,63,20,128+m,112,sodapics);
	    spridraw(180-32,164-32,64*4,copyright);
	}
    }
    if ((dai >= 300) && (dai < 364))
    {
	SetVisibleScreenOffset(introplc);

	if (vidmode == 0)
	    m = 20;
	else
	    m = 0;
	statusbardraw(0,44,63,20,128+m,112,sodapics);
	m = (364-dai);
	spridraw(180-(m>>1),164-(m>>1),m<<2,copyright);
    }
    if ((dai >= 364) && (dai < 424))
    {
	SetVisibleScreenOffset(introplc);

	wipeoverlay(0,174,361,20);
	if (vidmode == 0)
	    m = 20;
	else
	    m = 0;
	statusbardraw(0,36,63,8,128+m,112,sodapics);
	strcpy(&textbuf[0],"Select Episode");
	textprint(124,112,(char)96);

	if (vidmode == 0)
	    m = 20;
	else
	    m = 0;
/*	statusbardraw(0,44,63,20,128+m,112,sodapics);*/
	m = (dai-360);
	spridraw(180-96-(m>>1),152-(m>>1),m<<2,episodesign1off);
	spridraw(180-(m>>1),152-(m>>1),m<<2,episodesign2off);
	spridraw(180+96-(m>>1),152-(m>>1),m<<2,episodesign3off);
    }
    if (dai >= 424)
    {
	if (sharemessplc >= 0)
	{
	    if (vidmode == 0)
		m = 20;
	    else
		m = 0;
	    /*
	    k = 0;
	    if (numboards == 10)
		k = 32;
	      statusbardraw(sharemessplc,k,64-sharemessplc,32,m,32,sharewaremessage);
	      statusbardraw(0,k,64-sharemessplc,32,256+m+sharemessplc,32,sharewaremessage);*/
	    sharemessplc--;
	}
	if (pickskiltime < 0)
	{
	    SetVisibleScreenOffset(introplc);
	
/*	    if (dalasti < 424)
	    {*/
	    wipeoverlay(0,174,361,20);
	    if (vidmode == 0)
		m = 20;
	    else
		m = 0;
	    statusbardraw(0,36,63,8,128+m,112,sodapics);
	    strcpy(&textbuf[0],"Select Episode");
	    textprint(124,112,(char)96);
/*	    }*/
	    animater2 = (dai&16);
	    if ((newgamepisode != 1) || (animater2 == 0))
		spridraw(180-128,120,64*4,episodesign1off);
	    else
		spridraw(180-128,120,64*4,episodesign1on);
	    if ((newgamepisode != 2) || (animater2 == 0))
		spridraw(180-32,120,64*4,episodesign2off);
	    else
		spridraw(180-32,120,64*4,episodesign2on);
	    if ((newgamepisode != 3) || (animater2 == 0))
		spridraw(180+64,120,64*4,episodesign3off);
	    else
		spridraw(180+64,120,64*4,episodesign3on);
	}
	else if (dai >= pickskiltime+128)
	{
	    SetVisibleScreenOffset(introplc);

/*	    if (dalasti < pickskiltime+128)
	    {*/
	    wipeoverlay(0,174,361,8);
	    strcpy(&textbuf[0],"Select Difficulty");
	    textprint(112,112,(char)96);
/*	    }*/
	    animater2 = (dai&16);
	    if ((skilevel != 0) || (animater2 == 0))
		spridraw(180-48-32,120,64*4,skilevel1);
	    else {
		spridraw(180-48-32,120,64*4,skilevel1);
		spridraw(180-48-32,120,64*4,skilblank);
	    }
	    if ((skilevel != 1) || (animater2 == 0))
		spridraw(180+48-32,120,64*4,skilevel2);
	    else {
		spridraw(180+48-32,120,64*4,skilevel2);
		spridraw(180+48-32,120,64*4,skilblank);
	    }
	}
    }
    if ((pickskiltime >= 0) && (dai < pickskiltime+64))
    {
	SetVisibleScreenOffset(introplc);

	wipeoverlay(0,174,361,20);
	if (vidmode == 0)
	    m = 20;
	else
	    m = 0;
	statusbardraw(0,36,63,8,128+m,112,sodapics);
	strcpy(&textbuf[0],"Select Episode");
	textprint(124,112,(char)96);

	m = ((pickskiltime+63)-dai);
	spridraw(180-96-(m>>1),152-(m>>1),m<<2,episodesign1off);
	spridraw(180-(m>>1),152-(m>>1),m<<2,episodesign2off);
	spridraw(180+96-(m>>1),152-(m>>1),m<<2,episodesign3off);
    }
    if ((pickskiltime >= 0) && (dai >= pickskiltime+64) && (dai < pickskiltime+128))
    {
	SetVisibleScreenOffset(introplc);

	wipeoverlay(0,174,361,8);
	strcpy(&textbuf[0],"Select Difficulty");
	textprint(112,112,(char)96);

	m = (dai-(pickskiltime+64));
	spridraw(180-48-(m>>1),152-(m>>1),m<<2,skilevel1);
	spridraw(180+48-(m>>1),152-(m>>1),m<<2,skilevel2);
    }
}

/* Introduction... */

void introduction(K_INT16 songnum)
{
    K_INT16 j;
    K_INT16 leaveintro;
    K_INT32 dai=0;

    ingame=0;
    dside = 200;
    halfheight = 100;
    scrsize = 18000;
    dalasti = 0;

    spriteyoffset=20;

    gfxClearScreen();

    if (songnum == 0)
	loadmusic("INTRO");
    else
	loadmusic("INTRO2");
    clockspeed = 0;
    ototclock = -1;
    totalclock = 1;
    introplc = 5;

    musicon();
    if (saidwelcome == 0)
    {
	ksay(18);
	saidwelcome = 1;
    }
/*    for(i=0;i<360;i++)
      height[i] = 0;*/
    boardnum = 0;
    keystatus[57] = 0;
    keystatus[28] = 0;
    keystatus[1] = 0;
    newkeystatus[SDLK_ESCAPE]=0;
    newkeystatus[SDLK_RETURN]=0;
    newkeystatus[SDLK_SPACE]=0;
    sharemessplc = 64;
    newgamepisode = 1;
    animater2 = 0;
    totalclock = 0;
    leaveintro = 0;
    pickskiltime = -32768;

    while (leaveintro == 0)
    {
	PollInputs();
	bstatus = 0;
	if (moustat == 0)
	{
	    bstatus=readmouse(NULL, NULL);
	}
	if (joystat == 0) {
	    bstatus|=readjoystick(NULL,NULL);
	}
	j = (((int)labs((totalclock%120)-60))>>3);

	SDL_LockMutex(timermutex);
	while(clockspeed<4) {
	    SDL_UnlockMutex(timermutex);
	    SDL_Delay(10);
	    SDL_LockMutex(timermutex);
	}
	totalclock += clockspeed;
	clockspeed = 0;
	SDL_UnlockMutex(timermutex);

	dalasti = dai;
	dai = (totalclock>>2);
	if ((keystatus[0xc9]|keystatus[0xc8]|keystatus[0xcb]) != 0)
	{
	    if (pickskiltime < 0)
	    {
		newgamepisode--;
		if (newgamepisode == 0)
		    newgamepisode = 3;
	    }
	    else
		skilevel = 1 - skilevel;
	    ksay(27);
	    keystatus[0xc9] = 0;
	    keystatus[0xc8] = 0;
	    keystatus[0xcb] = 0;
	    newkeystatus[SDLK_UP]=0;
	    newkeystatus[SDLK_PAGEUP]=0;
	    newkeystatus[SDLK_LEFT]=0;
	}
	if ((keystatus[0xd1]|keystatus[0xd0]|keystatus[0xcd]) != 0)
	{
	    if (pickskiltime < 0)
	    {
		newgamepisode++;
		if (newgamepisode == 4)
		    newgamepisode = 1;
	    }
	    else
		skilevel = 1 - skilevel;
	    ksay(27);
	    keystatus[0xd1] = 0;
	    keystatus[0xd0] = 0;
	    keystatus[0xcd] = 0;
	    newkeystatus[SDLK_DOWN]=0;
	    newkeystatus[SDLK_PAGEDOWN]=0;
	    newkeystatus[SDLK_RIGHT]=0;
	}
	if (newkeystatus[newkeydefs[17]] > 0)
	{
	    lastunlock = 1;
	    lastshoot = 1;
	    lastbarchange = 1;
	    sortcnt = -1;
	    saidwelcome = introplc;
	    spriteyoffset=0;
	    j = mainmenu();
	    spriteyoffset=20;
	    saidwelcome = 1;
	    sortcnt = 0;
	    if ((j == 0) && (newgameplace >= 0))
	    {
		if (newgameplace == 0) boardnum = 0, newgamepisode = 1;
		if (newgameplace == 1) boardnum = 10, newgamepisode = 2;
		if (newgameplace == 2) boardnum = 20, newgamepisode = 3;
		leaveintro = 1;
	    }
	    if ((j == 1) && (loadsavegameplace >= 0))
	    {
		musicoff();
		setgamevideomode();
		if (vidmode == 0)
		{
		    dside = 200;
		    halfheight = 100;
		    scrsize = 18000-2880;
		}
		else
		{
		    dside = 240;
		    halfheight = 120;
		    scrsize = 21600-2880;
		}
		spriteyoffset=0;
		visiblescreenyoffset=0;
		ingame=1;
		settransferpalette();
		loadgame(loadsavegameplace);
		wipeoverlay(0,0,361,statusbaryoffset);
		fade(63);		
		return;
	    }
	    if (j == 8)
	    {
		newgamepisode = 1;
		leaveintro = 1;
		quitgame = 1;
	    }
	    newkeystatus[newkeydefs[17]] = 0;
	    keystatus[1] = 0;
	    keystatus[57] = 0;
	    keystatus[28] = 0;
	    newkeystatus[SDLK_ESCAPE]=0;
	    newkeystatus[SDLK_RETURN]=0;
	    newkeystatus[SDLK_SPACE]=0;
	    SDL_LockMutex(timermutex);
	    clockspeed = 0;
	    SDL_UnlockMutex(timermutex);
	}
	if ((keystatus[1] != 0) || (keystatus[57] != 0) || (keystatus[28] != 0) || (bstatus != 0))
	{
	    if (dai < 420)
		leaveintro = 1;
	    else if (((newgamepisode == 2) && (numboards < 20)) || ((newgamepisode == 3) && (numboards < 30)))
	    {
		ksay(12);
		SDL_LockMutex(timermutex);
		clockspeed = 0;
		SDL_UnlockMutex(timermutex);
	    }
	    else
	    {
		if (pickskiltime == -32768)
		    pickskiltime = dai;
		else
		    leaveintro = 1;
	    }
	    keystatus[1] = 0;
	    keystatus[57] = 0;
	    keystatus[28] = 0;
	    newkeystatus[SDLK_ESCAPE]=0;
	    newkeystatus[SDLK_RETURN]=0;
	    newkeystatus[SDLK_SPACE]=0;
	}
	if ((dai >= 0) && (dai < 128))
	    fade(64+((int)(dai>>1)));
	if ((dai >= 128) && (dalasti < 128))
	    fade(63);
	drawintroduction();
	gfxSwapBuffers( );
    }

    spriteyoffset=0;

    if (newgamepisode == 1) boardnum = 0;
    if (newgamepisode == 2) boardnum = 10;
    if (newgamepisode == 3) boardnum = 20;
    if ((newgamepisode == 2) && (numboards < 20))
	newgamepisode = 1, boardnum = 0;
    if ((newgamepisode == 3) && (numboards < 30))
	newgamepisode = 1, boardnum = 0;
    musicoff();
    setgamevideomode();
    fade(0);
    keystatus[57] = 0;
    keystatus[28] = 0;
    keystatus[1] = 0;
    newkeystatus[SDLK_ESCAPE]=0;
    newkeystatus[SDLK_RETURN]=0;
    newkeystatus[SDLK_SPACE]=0;
    if (vidmode == 0)
    {
	dside = 200;
	halfheight = 100;
	scrsize = 18000-2880;
    }
    else
    {
	dside = 240;
	halfheight = 120;
	scrsize = 21600-2880;
    }

    gfxClearScreen();

    loadboard();
    owecoins = 0;
    sortcnt = 0;
    oldlife = 0;
    life = 4095;
    death = 4095;
    lifevests = 1;
    switch (boardnum)
    {
	case 0: firepowers[0] = 0; firepowers[1] = 0;
	    firepowers[2] = 0; lightnings = 0;
	    break;
	case 10: firepowers[0] = 3; firepowers[1] = 2;
	    firepowers[2] = 0; lightnings = 1;
	    break;
	case 20: firepowers[0] = 4; firepowers[1] = 3;
	    firepowers[2] = 2; lightnings = 2;
	    break;
    }
    coins = 0;
    bulchoose = 0;
    animate2 = 0;
    animate3 = 0;
    animate4 = 0;
    oscillate3 = 0;
    oscillate5 = 0;
    animate6 = 0;
    animate7 = 0;
    animate8 = 0;
    animate10 = 0;
    animate11 = 0;
    animate15 = 0;
    ototclock = -1;
    totalclock = 1;
    purpletime = 0;
    greentime = 0;
    capetime[0] = 0;
    capetime[1] = 0;
    compass = 0;
    if (boardnum >= 10) compass = 1;
    cheated = 0;
    doorstat = 0;
    statusbar = 335;
    if (vidmode == 1)
	statusbar += 80;
    statusbargoal = statusbar;
    linecompare(statusbar);
    ingame=1;
    settransferpalette();
    namrememberstat = hiscorenamstat;
    hiscorenamstat = 0;
    hiscorenam[0] = 0;
    SDL_LockMutex(timermutex);
    clockspeed = 0;
    SDL_UnlockMutex(timermutex);
    scoreclock = 0;
    scorecount = 0;
    fadewarpval = 63;
    fadehurtval = 0;
    visiblescreenyoffset=0;
    wipeoverlay(0,0,361,statusbaryoffset);
    gfxDrawBack();
    statusbaralldraw();
}

/* Load KSM file... */

K_INT16 loadmusic(char *filename)
{
    char buffer[256], instbuf[11];
    int infile;
    K_INT16 i, j, k, numfiles;
    K_INT32 filoffs;

    FILE *file;

    if (musicsource == -1)
	return(-1);
    if (firstime == 1)
    {
	if (musicsource == 1)
	{
	    /* Open KSM->MIDI instrument translation table... */

	    file=fopen("ksmmidi.txt","rt");
	    if (file==NULL) {
		fprintf(stderr,"ksmmidi.txt not found; music disabled.\n");
		musicsource=-1;
		return -1;
	    }
	    for(i=0;i<256;i++)
		fscanf(file,"%d",&gminst[i]);
	    fclose(file);
	}
	if (musicsource == 2)
	{
	    if(((infile=open("insts.dat",O_RDONLY|O_BINARY,0))==-1)&&
	       ((infile=open("INSTS.DAT",O_RDONLY|O_BINARY,0))==-1))
		return(-1);
	    for(i=0;i<256;i++)
	    {
		read(infile,&buffer,33);
		for(j=0;j<11;j++)
		    inst[i][j] = buffer[j+20];
	    }
	    close(infile);
	    numchans = 9;

	    outdata((char)0,(char)0x1,(char)32);  //clear test stuff
	    outdata((char)0,(char)0x4,(char)0);   //reset
	    outdata((char)0,(char)0x8,(char)0);   //2-operator synthesis

	    firstime = 0;
	}
    }
    if (((infile=open("songs.kzp",O_RDONLY|O_BINARY,0))==-1)&&
	((infile=open("SONGS.KZP",O_RDONLY|O_BINARY,0))==-1))
	return(-1);
    readLE16(infile,&numfiles,2);
    i = 0;
    j = 1;
    while ((j == 1) && (i < numfiles))
    {
	read(infile,&buffer[0],12);
	j = 0;
	k = 0;
	while ((filename[k] != 0) && (k < 8))
	{
	    if (buffer[k] != filename[k])
		j = 1;
	    k++;
	}
	i++;
    }
    if (j == 1)
    {

	close(infile);
	return(-1);
    }

    filoffs=readlong(buffer+8);

    lseek(infile,filoffs,SEEK_SET);
    read(infile,&trinst[0],16);
    read(infile,&trquant[0],16);
    read(infile,&trchan[0],16);
    read(infile,&trprio[0],16);
    read(infile,&trvol[0],16);
    readLE16(infile,&numnotes,2);
    readLE32(infile,note,numnotes*4);
    close(infile);
    numchans = 9-trchan[11]*3;
    if (musicsource == 1)
	setmidiinsts();
    if (musicsource == 2)
    {
	if (trchan[11] == 0)
	{
	    drumstat = 0;
	    outdata((char)0,(unsigned char)0xbd,(unsigned char)drumstat);
	}
	if (trchan[11] == 1)
	{
	    for(i=0;i<11;i++)
		instbuf[i] = inst[trinst[11]][i];
	    instbuf[1] = ((instbuf[1]&192)|((trvol[11])^63));
	    setinst(0,6,instbuf[0],instbuf[1],instbuf[2],instbuf[3],instbuf[4],instbuf[5],instbuf[6],instbuf[7],instbuf[8],instbuf[9],instbuf[10]);
	    for(i=0;i<5;i++)
		instbuf[i] = inst[trinst[12]][i];
	    for(i=5;i<11;i++)
		instbuf[i] = inst[trinst[15]][i];
	    instbuf[1] = ((instbuf[1]&192)|((trvol[12])^63));
	    instbuf[6] = ((instbuf[6]&192)|((trvol[15])^63));
	    setinst(0,7,instbuf[0],instbuf[1],instbuf[2],instbuf[3],instbuf[4],instbuf[5],instbuf[6],instbuf[7],instbuf[8],instbuf[9],instbuf[10]);
	    for(i=0;i<5;i++)
		instbuf[i] = inst[trinst[14]][i];
	    for(i=5;i<11;i++)
		instbuf[i] = inst[trinst[13]][i];
	    instbuf[1] = ((instbuf[1]&192)|((trvol[14])^63));
	    instbuf[6] = ((instbuf[6]&192)|((trvol[13])^63));
	    setinst(0,8,instbuf[0],instbuf[1],instbuf[2],instbuf[3],instbuf[4],instbuf[5],instbuf[6],instbuf[7],instbuf[8],instbuf[9],instbuf[10]);
	    outdata((char)0,(unsigned char)0xa6,(unsigned char)(600&255));
	    outdata((char)0,(unsigned char)0xb6,(unsigned char)((600>>8)&223));
	    outdata((char)0,(unsigned char)0xa7,(unsigned char)(400&255));
	    outdata((char)0,(unsigned char)0xb7,(unsigned char)((400>>8)&223));
	    outdata((char)0,(unsigned char)0xa8,(unsigned char)(5510&255));
	    outdata((char)0,(unsigned char)0xb8,(unsigned char)((5510>>8)&223));
	    drumstat = 32;
	    outdata((char)0,(unsigned char)0xbd,(unsigned char)drumstat);
	}
    }
    return 0;
}

void outdata(unsigned char synth,unsigned char index,unsigned char data)
{
    adlib0(index,data);
}

/* Start music (and start timer!). */

void musicon()
{
    K_INT16 i, j, k, ksaystat;
    unsigned char instbuf[11];
    K_UINT32 templong;

    if (musicsource >= 0)
    {
	for(i=0;i<numchans;i++)
	{
	    chantrack[i] = 0;
	    chanage[i] = 0;
	}
	j = 0;
	for(i=0;i<16;i++)
	    if ((trchan[i] > 0) && (j < numchans))
	    {
		k = trchan[i];
		while ((j < numchans) && (k > 0))
		{
		    chantrack[j] = i;
		    k--;
		    j++;
		}
	    }
	if (musicsource==1)
	    setmidiinsts();
	for(i=0;i<numchans;i++)
	{
	    if (musicsource == 2)
	    {
		for(j=0;j<11;j++)
		    instbuf[j] = inst[trinst[chantrack[i]]][j];
		instbuf[1] = ((instbuf[1]&192)|(63-trvol[chantrack[i]]));
		setinst(0,i,instbuf[0],instbuf[1],instbuf[2],instbuf[3],instbuf[4],instbuf[5],instbuf[6],instbuf[7],instbuf[8],instbuf[9],instbuf[10]);
	    }
	    chanfreq[i] = 0;
	}
    }
    templong=note[0];
    count = (templong>>12)-1;
    countstop = (templong>>12)-1;
    nownote = 0;
    SDL_LockMutex(soundmutex);
    musicstatus = 1;
    ksaystat = 0;
    if (musicpan) randoinsts();

    if (ksaystat == 0)
    {
	lastTick=SDL_GetTicks();
	if (timer==NULL) 
	    timer=SDL_AddTimer(4,tickhandler, NULL);/* 250 Hz, should be 240.*/
    }
    SDL_UnlockMutex(soundmutex);
}

/* Stop music (and timer!). */

void musicoff()
{
    K_INT16 i, ksaystat;

    ksaystat = 0;
    if (ksaystat == 0)
    {
	if (timer!=NULL) {
	    SDL_RemoveTimer(timer);
	    timer=NULL;
	}
    }
    if (musicsource == 1) {
#ifdef WIN32
	midiOutReset(sequencerdevice);
#endif
#ifdef USE_OSS
	ioctl(sequencerdevice, SNDCTL_SEQ_PANIC);
	ioctl(sequencerdevice, SNDCTL_SEQ_RESET);
#endif
	setmidiinsts();
    }
    if (musicsource == 2)
	for(i=0;i<numchans;i++)
	{
	    outdata((char)0,(unsigned char)(0xa0+i),(char)0);
	    outdata((char)0,(unsigned char)(0xb0+i),(char)0);
	}
    SDL_LockMutex(soundmutex);
    musicstatus = 0;
    SDL_UnlockMutex(soundmutex);
}

/* SDL tick handler, complete with routine to even out timer ticks at funny
   intervals... */

void updateclock(void) {
    Uint32 now;

    now=SDL_GetTicks();
    
    while(((lastTick+(4+(tickFrac==0)))<=now)||(lastTick>now)) {
	if (!soundtimer) clockspeed++;
	if (musicsource!=2) ksmhandler();
	lastTick+=4+(tickFrac==0);
	tickFrac++;
	if (tickFrac==6) tickFrac=0;
	}
}

Uint32 tickhandler(Uint32 interval, void *param) {
    SDL_LockMutex(timermutex);
    updateclock();
    SDL_UnlockMutex(timermutex);
    return interval;
}

/* Update clock and music... */

void ksmhandler(void)
{
    K_INT16 i, j, quanter, bufnum, chan, drumnum, freq;
    K_UINT32 temp, templong;

    count++;
    if ((count >= countstop) && (musicsource >= 0))
    {
	bufnum = 0;
	while (count >= countstop)
	{
	    templong=note[nownote];
	    if (musicsource == 0)
		if ((((templong>>8)&15) == 0) && ((templong&64) > 0))
		    databuf[bufnum++] = (unsigned char)(templong&63);
	    if (musicsource > 0)
	    {
		if (((templong&255) >= 1) && ((templong&255) <= 61))
		{
		    i = 0;
		    while (((chanfreq[i] != (templong&63)) || (chantrack[i] != ((templong>>8)&15))) && (i < numchans))
			i++;
		    if (i < numchans)
		    {
			if (musicsource == 1)
			{
			    databuf[bufnum++] = (unsigned char)(0x80)+i;
			    databuf[bufnum++] = (unsigned char)(templong&63)+
				23;
			    databuf[bufnum++] = (unsigned char)0;
			}
			if (musicsource == 2)
			{
			    databuf[bufnum++] = (unsigned char)(0xa0+i);
			    databuf[bufnum++] = (unsigned char)(adlibfreq[templong&63]&255);
			    databuf[bufnum++] = (unsigned char)(0xb0+i);
			    databuf[bufnum++] = (unsigned char)((adlibfreq[templong&63]>>8)&223);
			}
			chanfreq[i] = 0;
			chanage[i] = 0;
		    }
		}
		else if (((templong&255) >= 65) && ((templong&255) <= 125))
		{
		    if (((templong>>8)&15) < 11)
		    {
			temp = 0;
			i = numchans;
			for(j=0;j<numchans;j++)
			    if ((countstop - chanage[j] >= temp) && (chantrack[j] == ((templong>>8)&15)))
			    {
				temp = countstop - chanage[j];
				i = j;
			    }
			if (i < numchans)
			{
			    if (musicsource == 1)
			    {
				if (chanfreq[i]!=0) {
				    databuf[bufnum++] = (unsigned char)
					(0x80+i);
				    databuf[bufnum++] = (unsigned char)
					chanfreq[i]+23;
				    databuf[bufnum++] = (unsigned char)0;
				}
				databuf[bufnum++] = (unsigned char)(0x90+i);
				databuf[bufnum++] = (unsigned char)(templong&63)+23;
				databuf[bufnum++] = (unsigned char)
				    trvol[chantrack[i]]<<1;
			    }
			    if (musicsource == 2)
			    {
				databuf[bufnum++] = (unsigned char)(0xa0+i);
				databuf[bufnum++] = (unsigned char)0;
				databuf[bufnum++] = (unsigned char)(0xb0+i);
				databuf[bufnum++] = (unsigned char)0;
				databuf[bufnum++] = (unsigned char)(0xa0+i);
				databuf[bufnum++] = (unsigned char)(adlibfreq[templong&63]&255);
				databuf[bufnum++] = (unsigned char)(0xb0+i);
				databuf[bufnum++] = (unsigned char)((adlibfreq[templong&63]>>8)|32);
			    }
			    chanfreq[i] = templong&63;
			    chanage[i] = countstop;
			}
		    }
		    else
		    {
			if (musicsource == 1)
			{
			    databuf[bufnum++] = (unsigned char)(0x99);
			    switch((templong>>8)&15)
			    {
				case 11: drumnum = 36; break;
				case 12: drumnum = 38; break;
				case 13: drumnum = 37; break;
				case 14: drumnum = 51; break;
				case 15: drumnum = 42; break;
				default: drumnum = 36; break;
			    }
			    databuf[bufnum++] = (unsigned char)drumnum;
			    databuf[bufnum++] = (unsigned char)64;
			}
			if (musicsource == 2)
			{
			    freq = adlibfreq[templong&63];
			    switch((unsigned char)((templong>>8)&15))
			    {
				case 11: drumnum = 16; chan = 6; freq -= 2048; break;
				case 12: drumnum = 8; chan = 7; freq -= 2048; break;
				case 13: drumnum = 4; chan = 8; break;
				case 14: drumnum = 2; chan = 8; break;
				case 15: drumnum = 1; chan = 7; freq -= 2048; break;
				default: drumnum = 16; chan=6; freq-=2048; break;
			    }
			    databuf[bufnum++] = (unsigned char)(0xa0+chan);
			    databuf[bufnum++] = (unsigned char)(freq&255);
			    databuf[bufnum++] = (unsigned char)(0xb0+chan);
			    databuf[bufnum++] = (unsigned char)((freq>>8)&223);
			    databuf[bufnum++] = (unsigned char)(0xbd);
			    databuf[bufnum++] = (((unsigned char)(drumstat))&((unsigned char)(255-drumnum)));
			    drumstat |= drumnum;
			    databuf[bufnum++] = (unsigned char)(0xbd);
			    databuf[bufnum++] = (unsigned char)(drumstat);
			}
		    }
		}
	    }
	    nownote++;
	    if (nownote >= numnotes)
		nownote = 0;
	    templong=note[nownote];
	    if (nownote == 0)
		count = (templong>>12)-1;
	    quanter = (240/trquant[(templong>>8)&15]);
	    countstop = (((templong>>12)+(quanter>>1)) / quanter) * quanter;
	}
	if (mute == 0)
	{
	    if (musicsource == 0)
	    {
		j = 0;
		for(i=0;i<bufnum;i++)
		    if (databuf[i] > j)
			j = databuf[i];
		if (j == 0)
		{
		    /* Removed: Mute beeper. */
		}
		if (j > 0)
		{
		    /* Removed: Beep at pcfreq[j]. */
		}
	    }
	    if (musicsource == 1) {
#ifdef WIN32
		for(i=0;i<bufnum;i+=3) {
		    midiOutShortMsg(sequencerdevice,databuf[i]|(databuf[i+1]<<8)|(databuf[i+2]<<16));
		}
#endif
#ifdef USE_OSS
		for(i=0;i<bufnum;i++)
		{
		    SEQ_MIDIOUT(nrmidis-1, databuf[i]);
		}
		SEQ_DUMPBUF();
#endif
	    }
	    if (musicsource == 2)
		for(i=0;i<bufnum;i+=2)
		{		    
		    outdata(0,databuf[i],databuf[i+1]);
		}
	}
    }
}

/* Adlib set instrument... */

void setinst(unsigned char synth,K_INT16 chan,unsigned char v0,
	     unsigned char v1,unsigned char v2,unsigned char v3,
	     unsigned char v4,unsigned char v5,unsigned char v6,
	     unsigned char v7,unsigned char v8,unsigned char v9,
	     unsigned char v10)
{
    K_INT16 offs;

    outdata(synth,(unsigned char)(0xa0+chan),(unsigned char)0);
    outdata(synth,(unsigned char)(0xb0+chan),(unsigned char)0);
    outdata(synth,(unsigned char)(0xc0+chan),v10);
/*    if (chan == 0)*/
    offs = 0;
    if (chan == 1)
	offs = 1;
    if (chan == 2)
	offs = 2;
    if (chan == 3)
	offs = 8;
    if (chan == 4)
	offs = 9;
    if (chan == 5)
	offs = 10;
    if (chan == 6)
	offs = 16;
    if (chan == 7)
	offs = 17;
    if (chan == 8)
	offs = 18;
    outdata(synth,(unsigned char)(0x20+offs),v5);
    outdata(synth,(unsigned char)(0x40+offs),v6);
    outdata(synth,(unsigned char)(0x60+offs),v7);
    outdata(synth,(unsigned char)(0x80+offs),v8);
    outdata(synth,(unsigned char)(0xe0+offs),v9);
    offs+=3;
    outdata(synth,(unsigned char)(0x20+offs),v0);
    outdata(synth,(unsigned char)(0x40+offs),v1);
    outdata(synth,(unsigned char)(0x60+offs),v2);
    outdata(synth,(unsigned char)(0x80+offs),v3);
    outdata(synth,(unsigned char)(0xe0+offs),v4);
}

/* MIDI set instruments. */

void setmidiinsts()
{
#ifdef WIN32
    int i;

    midiOutReset(sequencerdevice);
    for(i=0;i<16;i++)
    	midiOutShortMsg(sequencerdevice,0xc0|i|(((i<numchans)?gminst[trinst[chantrack[i]]]:0)<<8));
#endif
#ifdef USE_OSS
    int i;

    _seqbufptr = 0;
    ioctl(sequencerdevice, SNDCTL_SEQ_RESET);
    for (i = 0; i < 16; i++) {
//	SEQ_CONTROL(nrmidis-1,i,0,0);
//	SEQ_CONTROL(nrmidis-1,i,32,0);
        SEQ_MIDIOUT(nrmidis-1, MIDI_PGM_CHANGE + i);
/*	if (i<numchans) {
	printf("Channel %d: KSM inst %d -> GM inst %d.\n", i,
	trinst[chantrack[i]],gminst[trinst[chantrack[i]]]);
	}*/
	SEQ_MIDIOUT(nrmidis-1, (i<numchans)?gminst[trinst[chantrack[i]]]:0);
    }
    SEQ_DUMPBUF();
#endif
}

/* Did we hit a wall? If so, push us out of it... */

void checkhitwall(K_UINT16 oposx,K_UINT16 oposy,K_UINT16 posix,
		  K_UINT16 posiy)
{
    K_INT16 i, j, k, m, xdir, ydir, cntx, cnty, xpos, ypos, xinc, yinc;
    K_UINT16 x1, y1, x2, y2, x3, y3, xspan, yspan;
    K_INT32 templong;

    if (oposx < posix)
    {
	x1 = (oposx>>10);
	xdir = 1;
	xinc = posix-oposx;
    }
    else
    {
	x1 = (posix>>10);
	xdir = -1;
	xinc = oposx-posix;
    }
    if (oposy < posiy)
    {
	y1 = (oposy>>10);
	ydir = 1;
	yinc = posiy-oposy;
    }
    else
    {
	y1 = (posiy>>10);
	ydir = -1;
	yinc = oposy-posiy;
    }
    xspan = abs(((int)(posix>>10))-((int)(oposx>>10)))+1;
    yspan = abs(((int)(posiy>>10))-((int)(oposy>>10)))+1;
    xpos = oposx-(x1<<10);
    ypos = oposy-(y1<<10);
    for(i=0;i<=(xspan<<2);i++)
    {
	x2 = ((i+3)>>2)+x1-1;
	x3 = ((i+5)>>2)+x1-1;
	if (((x2|x3)&0xffc0) == 0)
	    for(j=0;j<=(yspan<<2);j++)
	    {
		y2 = ((j+3)>>2)+y1-1;
		y3 = ((j+5)>>2)+y1-1;
		k = i+(j<<6);
		tempbuf[k] = 0;
		if (((y2|y3)&0xffc0) == 0)
		{
		    m = board[x2][y2];
		    if (((m&3072) != 1024) && ((m&1023) != 0)) tempbuf[k] = 1;
		    m = board[x2][y3];
		    if (((m&3072) != 1024) && ((m&1023) != 0)) tempbuf[k] = 1;
		    m = board[x3][y2];
		    if (((m&3072) != 1024) && ((m&1023) != 0)) tempbuf[k] = 1;
		    m = board[x3][y3];
		    if (((m&3072) != 1024) && ((m&1023) != 0)) tempbuf[k] = 1;
		}
		else
		    tempbuf[k] = 1;
	    }
    }
    cntx = 0, cnty = 0;
    for(i=0;i<256;i++)
    {
	cntx+=xinc;
	j=cntx>>8;
	cntx&=255;
	for(k=0;k<j;k++) {
	    if (tempbuf[((xpos+xdir)>>8)+((ypos&0xff00)>>2)]==0)
		xpos+=xdir;
	}
	    
	cnty+=yinc;
	j=cnty>>8;
	cnty&=255;
	for(k=0;k<j;k++) {
	    if (tempbuf[(xpos>>8)+(((ypos+ydir)&0xff00)>>2)]==0)
		ypos+=ydir;
	}    
    }
    posx = xpos + (((unsigned)x1)<<10);
    posy = ypos + (((unsigned)y1)<<10);
    if (((posx&0xfc00) != (oposx&0xfc00)) || ((posy&0xfc00) != (oposy&0xfc00)))
    {
	if ((board[posx>>10][posy>>10]&1023) == stairs)
	{
	    if ((boardnum >= 10) && (boardnum < 20))
	    {
		j = -1;
		for(i=0;i<mnum;i++)
		    if (mstat[i] == mondog)
		    {
			if (mposx[i] > posx) templong = (long)(mposx[i]-posx);
			else templong = (long)(posx-mposx[i]);
			if (mposy[i] > posy) templong += (long)(mposy[i]-posy);
			else templong += (long)(posy-mposy[i]);
			j = 0;
			if (templong >= 4096)
			    j = 1;
		    }
		if (j != 0)
		{
		    posx = oposx;
		    posy = oposy;
		    mixing=1;
		    strcpy(&textbuf[0],"Where's your dog?");
		    wipeoverlay(112,40,200,10);
		    textprint(112,40+1,(char)96);
		    mixing=0;
		}
	    }
	}
	youarehere();
    }
}

/* Palette for text in endgame sequences... */

unsigned char textpalette[48]={
    3,3,3,
    7,7,7,
    11,11,11,
    15,15,15,
    19,19,19,
    23,23,23,
    27,27,27,
    31,31,31,
    35,35,35,
    39,39,39,
    43,43,43,
    47,47,47,
    51,51,51,
    55,55,55,
    59,59,59,
    63,63,63
};

/* Win episode 1 or 2... */

void wingame(K_INT16 episode)
{
    K_UINT16 startx, starty;
    K_INT16 oldmute;
    K_INT32 revtotalclock, revototclock, templong;

    K_INT32 tempototclock=ototclock,temptotalclock=totalclock;

    oldmute = mute;
    revototclock = -1;
    revtotalclock = 0;
    musicoff();
    if (episode == 1) loadmusic("WINGAME0");
    if (episode == 2) loadmusic("WINGAME1");
    musicon();
    posz = 32;
    if (episode == 1)
    {
	startx = (((unsigned)58)<<10);
	starty = (((unsigned)19)<<10);
    }
    else
    {
	startx = (((unsigned)5)<<10);
	starty = (((unsigned)12)<<10);
    }
    while ((death != 4096) || ((keystatus[1] == 0) && (keystatus[57] == 0) && (keystatus[28] == 0) && (bstatus == 0)))
    {
	PollInputs();
	bstatus = 0;
	if (moustat == 0)
	{
	    bstatus=readmouse(NULL, NULL);
	}
	if (joystat == 0)
	{
	    bstatus|=readjoystick(NULL,NULL);
	}
	if (death != 4096)
	{
	    ang = 512;
	    posx = (startx+512);
	    if (revtotalclock < 3840)
		posy = starty+512+(unsigned)((revtotalclock<<6)/15);
	    else
		posy = starty+16384+512;
	    picrot(posx,posy,posz,ang);
	    gfxSwapBuffers();
	    sortcnt = 0;
	    SDL_LockMutex(soundmutex);
	    SDL_LockMutex(timermutex);
	    if ((musicstatus == 1) && (clockspeed >= 0) && (clockspeed < 3))
		while(clockspeed<3) {
		    SDL_UnlockMutex(timermutex);
		    SDL_UnlockMutex(soundmutex);
		    SDL_Delay(10);
		    SDL_LockMutex(soundmutex);
		    SDL_LockMutex(timermutex);
		}
	    SDL_UnlockMutex(timermutex);
	    SDL_UnlockMutex(soundmutex);
	}
	if (episode == 2)
	{
	    if ((revtotalclock >= 840) && (revototclock < 840))
	    {
		life += 1280;
		if (life > 4095)
		    life = 4095;
		drawlife();
		ksay(7);
	    }
	    if ((revtotalclock >= 1560) && (revototclock < 1560))
	    {
		life += 1280;
		if (life > 4095)
		    life = 4095;
		drawlife();
		ksay(7);
	    }
	}
	if (revtotalclock >= 3840)
	{
	    if (revototclock < 3840)
	    {
		death = 4094;
		if (episode == 1) ksay(6);
		if (episode == 2) ksay(9);
		mute=mute?1:2;
	    }
	    if (death < 4095)
	    {
		if (death > 0)
		{
		    posz+=2;
		    if (posz > 64)
			posz = 64;
		    death -= (((((int)(revtotalclock-revototclock))<<5)&0xff80)+128);
		    if (death <= 0)
		    {
			death = 0;
			posz = 32;
		    }
		    fade(death>>6);
		}
		if ((death == 0) && (revtotalclock >= 4800))
		{
		    templong=note[0];

		    SDL_LockMutex(timermutex);
		    count = (templong>>12)-1;
		    countstop = (templong>>12)-1;
		    nownote = 0;
		    mute = oldmute;
		    SDL_UnlockMutex(timermutex);

		    death = 4096;
		    /*
		    if ((vidmode == 0) && (statusbar < 399))
		    {
			l = times90[((unsigned)statusbar+1)>>1]+5;
		    }
		    if ((vidmode == 1) && (statusbar < 479))
		    {
			l = times90[((unsigned)statusbar+1)>>1];
		    }
		    */
		    ingame=0;
		    wipeoverlay(0,0,361,241);
		    gfxClearScreen();
		    fade(63);
		    setdarkenedpalette();
		    updateoverlaypalette(240,16,textpalette);

		    drawmenu(304,192,menu);

		    if (episode == 1) loadstory(-21);
		    if (episode == 2) loadstory(-19);

		    finalisemenu();

		    ksay(23);
		    gfxSwapBuffers();
		    pressakey();
		    gfxClearScreen();
		    drawmenu(304,192,menu);
		    keystatus[1] = 0;
		    keystatus[57] = 0;
		    keystatus[28] = 0;
		    newkeystatus[SDLK_ESCAPE]=0;
		    newkeystatus[SDLK_RETURN]=0;
		    newkeystatus[SDLK_SPACE]=0;
		    if (episode == 1) loadstory(-20);
		    if (episode == 2) loadstory(-18);
		    finalisemenu();
		    settransferpalette();
		    gfxSwapBuffers();
		    pressakey();
		    ingame=1;
		    settransferpalette();
		}
	    }
	}
	revototclock = revtotalclock;
	SDL_LockMutex(timermutex);
	if (clockspeed==0) {
	    SDL_UnlockMutex(timermutex);
	    SDL_Delay(10); /* To avoid soaking up all CPU. */
	    SDL_LockMutex(timermutex);
	}
	revtotalclock += clockspeed;
	totalclock += clockspeed;
	animate2 = animate2 ^ 1;
	clockspeed = 0;
	SDL_UnlockMutex(timermutex);
    }
    musicoff();
    fade(0);
    gfxClearScreen();

    /* Previously -1 and 1, but this gave the player all the cloaks and potions
       he had in the previous episode for a time corresponding to the time to
       the time until he last ran out of them. Probably for half an hour or so.
       I hope this wasn't a feature. */

    ototclock = tempototclock;
    totalclock = temptotalclock;
    linecompare(statusbar);
    statusbaralldraw();
    fadewarpval = 63;
    fadehurtval = 0;
    keystatus[57] = 0;
    keystatus[28] = 0;
    keystatus[1] = 0;
    newkeystatus[SDLK_ESCAPE]=0;
    newkeystatus[SDLK_RETURN]=0;
    newkeystatus[SDLK_SPACE]=0;
    bstatus = 0;
    lastunlock = 1;
    lastshoot = 1;
    lastbarchange = 1;
    death = 4095;
}

/* Win episode 3... */

void winallgame()
{
    K_INT16 leavewin;
    K_INT32 revtotalclock;

    ingame=0;
//    revototclock = 1;
    revtotalclock = 1;
    linecompare(479);
    musicoff();
    loadmusic("WINGAME2");
    musicon();

    fade(63);
    leavewin = 0;
    bstatus = 0;
    keystatus[1] = 0;
    keystatus[57] = 0;
    keystatus[28] = 0;
    newkeystatus[SDLK_ESCAPE]=0;
    newkeystatus[SDLK_RETURN]=0;
    newkeystatus[SDLK_SPACE]=0;
    while ((leavewin == 0) || ((keystatus[1] == 0) && (keystatus[57] == 0) && (keystatus[28] == 0) && (bstatus == 0)))
    {
	PollInputs();
	bstatus = 0;
	if (moustat == 0) {
	    bstatus=readmouse(NULL,NULL);
	}
	if (joystat == 0)
	{
	    bstatus|=readjoystick(NULL,NULL);
	}
	if (revtotalclock < 3260)
	{
	    /*
	      for(i=lside;i<rside;i++)
	      height[i] = 0;
	    */
	    gfxClearScreen();
	    pictur(180,halfheight,4+(((int)revtotalclock)>>2),((int)((revtotalclock<<2))&2047)^2047,earth);
	    gfxSwapBuffers();
	}
	else
	{
	    gfxClearScreen();
	    wipeoverlay(0,0,361,241);
	    fade(63);
	    setdarkenedpalette();
	    drawmenu(304,192,menu);
	    updateoverlaypalette(240,16,textpalette);
	    loadstory(-17);
	    finalisemenu();
	    ksay(23);
	    gfxSwapBuffers();
	    pressakey();
	    gfxClearScreen();
	    drawmenu(304,192,menu);
	    keystatus[1] = 0;
	    keystatus[57] = 0;
	    keystatus[28] = 0;
	    newkeystatus[SDLK_ESCAPE]=0;
	    newkeystatus[SDLK_RETURN]=0;
	    newkeystatus[SDLK_SPACE]=0;
	    loadstory(-16);
	    finalisemenu();
	    settransferpalette();
	    gfxSwapBuffers();
	    pressakey();
	    leavewin = 1;
	}
//	revototclock = revtotalclock;
	SDL_LockMutex(timermutex);
	if (clockspeed==0) {
	    SDL_UnlockMutex(timermutex);
	    SDL_Delay(10); /* To avoid soaking up all CPU. */
	    SDL_LockMutex(timermutex);
	}
	revtotalclock += clockspeed;
	totalclock += clockspeed;
	clockspeed = 0;
	SDL_UnlockMutex(timermutex);
    }
    musicoff();
    fade(0);
    ototclock = -1;
    totalclock = 1;
    linecompare(statusbar);
    fade(63);
    gfxClearScreen();
    fadewarpval = 63;
    fadehurtval = 0;
    keystatus[57] = 0;
    keystatus[28] = 0;
    keystatus[1] = 0;
    newkeystatus[SDLK_ESCAPE]=0;
    newkeystatus[SDLK_RETURN]=0;
    newkeystatus[SDLK_SPACE]=0;
    bstatus = 0;
    lastunlock = 1;
    lastshoot = 1;
    lastbarchange = 1;
    death = 4095;
}

/* Show the compass. */

void showcompass(K_INT16 compang)
{
    K_INT16 i;

    i = (((compang+64)&2047)>>7);
    statusbardraw((i&2)<<4,((i&1)<<5)+2,29,29,238,1+statusbaryoffset,(i>>2)+compassplc);
}

/* Load a GIF. Violate a patent. 
   0-2 = GIFs in LAB3D.KZP, -1 = lab3d.gif, -2 to -4 = endn.gif. */
K_INT16 kgif(K_INT16 filenum)
{
    unsigned char header[13], imagestat[10], bitcnt, numbits;
    unsigned char globalflag, localflag, chunkind;
    K_INT16 i, j, k, x, y, ydim;
    K_INT16 numcols, numpals, fil, blocklen;
    K_UINT16 gifdatacnt,firstring,currstr,bytecnt,numbitgoal;

    K_UINT16 stack[LZW_STACK_SIZE];
    K_UINT16 stackp=0,dat;

    if (filenum<0) {
	switch(filenum) {
	    case -1:
		if (((fil = open("lab3d.gif",O_RDONLY|O_BINARY,0)) == -1)&&
		    ((fil = open("LAB3D.GIF",O_RDONLY|O_BINARY,0)) == -1))
		    return(-1);
		break;
	    case -2:
		if (((fil = open("end1.gif",O_RDONLY|O_BINARY,0)) == -1)&&
		    ((fil = open("END1.GIF",O_RDONLY|O_BINARY,0)) == -1))
		    return(-1);
		break;
	    case -3:
		if (((fil = open("end2.gif",O_RDONLY|O_BINARY,0)) == -1)&&
		    ((fil = open("END2.GIF",O_RDONLY|O_BINARY,0)) == -1))
		    return(-1);
		break;
	    case -4:
		if (((fil = open("end3.gif",O_RDONLY|O_BINARY,0)) == -1)&&
		    ((fil = open("END3.GIF",O_RDONLY|O_BINARY,0)) == -1))
		    return(-1);
		break;
	    default:
		return(-1);
	}
    } else {
	if (((fil = open("lab3d.kzp",O_RDONLY|O_BINARY,0)) == -1)&&
	    ((fil = open("LAB3D.KZP",O_RDONLY|O_BINARY,0)) == -1))
	    return(-1);
    }

//    rowpos = 0;
    if (filenum == 1)
    {
	lseek(fil,giflen1,SEEK_SET);
//	rowpos = 5;
    }
    if (filenum == 2)
	lseek(fil,giflen1+giflen2,SEEK_SET);

    gifdatacnt = 0;
    read(fil,&tempbuf[0],(unsigned)gifbuflen);
    for(j=0;j<13;j++)
	header[j] = tempbuf[j+gifdatacnt];
    gifdatacnt += 13;
/*    if ((header[0] != 'L') || (header[1] != 'A') || (header[2] != 'B'))
      return(-1);
      if ((header[3] != '3') || (header[4] != 'D') || (header[5] != '!'))
      return(-1);*/
    globalflag = header[10];
    numcols = (1<<((globalflag&7)+1));
    firstring = numcols+2;
//    backg = header[11];
    if (header[12] != 0)
	return(-1);
    memset(screenbuffer,(filenum==2)?255:((filenum==1)?0x50:0),
	   screenbufferwidth*screenbufferheight);

    if ((globalflag&128) > 0)
    {
	numpals = numcols+numcols+numcols;
	for(j=0;j<numpals;j++)
	    palette[j]=tempbuf[j+gifdatacnt]>>2;
	gifdatacnt += numpals;
    }
    chunkind = tempbuf[gifdatacnt], gifdatacnt++;
    while (chunkind == '!')
    {
	gifdatacnt++;
	do
	{
	    chunkind = tempbuf[gifdatacnt], gifdatacnt++;
	    if (chunkind > 0)
		gifdatacnt += chunkind;
	}
	while (chunkind > 0);
	chunkind = tempbuf[gifdatacnt], gifdatacnt++;
    }
    if (chunkind == ',')
    {
	for(j=0;j<9;j++)
	    imagestat[j] = tempbuf[j+gifdatacnt];
	gifdatacnt += 9;
//	xdim = imagestat[4]+(imagestat[5]<<8);
	ydim = imagestat[6]+(imagestat[7]<<8);
	localflag = imagestat[8];
	if ((localflag&128) > 0)
	{
	    numpals = numcols+numcols+numcols;
	    for(j=0;j<numpals;j++)
		palette[j]=tempbuf[j+gifdatacnt];
			
	    gifdatacnt += numpals;
	}
	gifdatacnt++;
//	numlines = 200;


	x = 20, y = (filenum==2)?0:20;
	bitcnt = 0;
	for(i=1;i<=numcols;i++) {
	    lzwbuf[i]=i&255;
	    lzwbuf2[i]=i;		    
	}
	lzwbuf2[0]=0;
	lzwbuf[0]=0;

	currstr = firstring;
	numbits = (globalflag&7)+2;
	numbitgoal = (numcols<<1);
	blocklen = 0;
	blocklen = tempbuf[gifdatacnt], gifdatacnt++;

	for(j=0;j<blocklen;j++) {
	    ((unsigned char *)(lincalc))[j]=tempbuf[j+gifdatacnt];
	}

	gifdatacnt += blocklen;
	bytecnt = 0;

	while (y < ydim+((filenum==2)?0:20))
	{
	    dat=(((((unsigned char *)lincalc)[bytecnt])|
		  ((K_UINT16)(((unsigned char *)lincalc)[bytecnt+1]))<<8)>>bitcnt)&
		((1<<numbits)-1);
/*	    dat=((*((K_UINT16 *)(((unsigned char *)lincalc)+bytecnt)))
	    >>bitcnt)&
	    ((1<<numbits)-1);*/
	    if (bitcnt+numbits>16) {
		dat+=(((K_UINT16)((unsigned char *)lincalc)[bytecnt+2])&
		      ((1<<((bitcnt+numbits)&15))-1))<<
		    (16-bitcnt);
	    }
		    
	    bitcnt+=numbits;
	    bytecnt+=bitcnt>>3;
	    bitcnt&=7;
		    
	    if (bytecnt > blocklen-3)
	    {
		writeshort((unsigned char *)lincalc,
			   readshort(&(((unsigned char *)lincalc)[bytecnt])));

		i = blocklen-bytecnt;

		blocklen=tempbuf[gifdatacnt++];

		if (gifdatacnt+blocklen < gifbuflen)
		{
		    memcpy(((unsigned char *)lincalc)+i,
			   tempbuf+gifdatacnt,
			   (blocklen+1)&~1);
		    gifdatacnt += blocklen;
		}
		else
		{
		    k = gifbuflen-gifdatacnt;
		    memcpy(((unsigned char *)lincalc)+i,
			   tempbuf+gifdatacnt,
			   (k+1)&~1);

		    read(fil,&tempbuf[0],
			 (unsigned)gifbuflen);
		    memcpy(((unsigned char *)lincalc)+i+k,
			   tempbuf,
			   ((gifdatacnt=blocklen-k)+1)&~1);
					
		}
		bytecnt = 0;
		blocklen += i;
	    }
	    if (currstr == numbitgoal)
		if (numbits < 12)
		{
		    numbits++;
		    numbitgoal <<= 1;
		}
	    if (dat == numcols)
	    {
		currstr = firstring;
		numbits = (globalflag&7)+2;
		numbitgoal = (numcols<<1);
	    }
	    else
	    {		
		lzwbuf2[currstr]=dat;
		while(dat>=firstring) {
		    stack[stackp++]=lzwbuf[dat];

		    dat=lzwbuf2[dat];
		}
				    
		lzwbuf[currstr-1]=dat&255;
		lzwbuf[currstr]=dat&255;
				    
		dat=lzwbuf2[dat];
		stack[stackp++]=dat;
				    
		while(stackp>0) {
		    stackp--;

		    screenbuffer[y*screenbufferwidth+x]=stack[stackp];

		    x++;
		    if (x>=340) {
			y++;
			x-=320;
		    }
		}

		currstr++;
	    }
	}
    }
    close(fil);

    gfxUploadOverlay();

    return 0;
}

/* Draw status bar if necessary. */

void ShowStatusBar() {
//    if (statusbaryoffset>=240) return;
    mixing=1;
    ShowPartialOverlay(20,statusbaryoffset,320,statusbaryvisible,1);
    mixing=0;
}

/* Redraw overlay as if screen offset were offset bytes... */

void SetVisibleScreenOffset(K_UINT16 offset) {

    float y=offset/90;

    gfxClearScreen();

    visiblescreenyoffset=y;

    ShowPartialOverlay(0,0+y,360,240,0);
}

void setgamevideomode()
{
    fade(63);
    videotype=0;
}

/* Draw text string in textbuf at (x,y) with colour offset coloffs... */

void textprint(K_INT16 x,K_INT16 y,char coloffs)
{
    unsigned char character;
    K_INT16 charcnt, walnume;

    if ((vidmode == 1) && (y>=statusbaryoffset))
	x += 20;

    charcnt = 0;
    while ((textbuf[charcnt] != 0) && (charcnt < 40))
    {
	character = (textbuf[charcnt])&127;
	if (lab3dversion)
	    walnume = (character>>6)+125;
	else
	    walnume = (character>>6)+textwall;

	drawtooverlay((character&7)<<3,((character&63)>>3)<<3,
		      8,8,x,y,walnume-1,coloffs);
	x+=8;
	charcnt++;
    }
}

/* Load and display some text (story, help, whatever). */

K_INT16 loadstory(K_INT16 boardnume)
{
    unsigned char xordat, otextcol, textcol;
    K_UINT16 storyoffs[128];
    K_INT16 fil, i, textbufcnt, textypos;

    ototclock = totalclock;
    if (((fil = open("story.kzp",O_RDONLY|O_BINARY,0)) == -1)&&
	((fil = open("STORY.KZP",O_RDONLY|O_BINARY,0)) == -1))
	return(-1);
    readLE16(fil,&storyoffs[0],256);
    lseek(fil,(long)(storyoffs[boardnume+34]),SEEK_SET);
    read(fil,&tempbuf[0],4096);
    i = 0;
    xordat = 0;
    if (vidmode == 0)
	textypos = 3;
    else
	textypos = 23;
    if ((boardnume >= 0) || ((boardnume >= -21) && (boardnume <= -16)))
	otextcol = 0;
    else
	otextcol = 96;
    textcol = otextcol;
    if ((boardnume >= -14) && (boardnume <= -2))
	textypos += 12;
    textbufcnt = 0;
    while ((tempbuf[i] != 0) && (textbufcnt < 40))
    {
	xordat ^= tempbuf[i];
	if (tempbuf[i] >= 32)
	{
	    if ((otextcol == 96) && (textbufcnt == 0))
	    {
		textcol = 96;
		if (tempbuf[i] == '!')
		    textcol = 16, textbuf[textbufcnt++] = '@';
		else if (tempbuf[i] == '%')
		    textcol = 80, textbuf[textbufcnt++] = '@';
		else
		    textbuf[textbufcnt++] = tempbuf[i];
	    }
	    else
		textbuf[textbufcnt++] = tempbuf[i];
	}
	if (tempbuf[i] == 13)
	{
	    textbuf[textbufcnt] = 0;
	    textprint(180-(textbufcnt<<2),textypos+1,textcol);
	    textypos += 12;
	    textbufcnt = 0;
	}
	i++;
	tempbuf[i] ^= xordat;
    }
    textbuf[textbufcnt] = 0;
    textprint(180-(textbufcnt<<2),textypos+1,textcol);
    close(fil);
    return 0;
}

K_INT16 setupmouse()
{
    return(0); /* Assume mouse always exists. */
}

/* Redraw status bar... */

void statusbaralldraw()
{
    K_INT16 i, j;

    for(i=0;i<361;i++)
	screenbuffer[i+240*screenbufferwidth]=0x50;
    gfxUploadPartialOverlay(0,240,361,1);
    statusbardraw(0,0,32,32,0,0+statusbaryoffset,statusbarback);
    statusbardraw(32,0,32,32,288,0+statusbaryoffset,statusbarback);
    for(i=32;i<288;i+=32)
	statusbardraw(16,0,32,32,i,0+statusbaryoffset,statusbarback);
    textbuf[0] = 'S', textbuf[1] = 'C', textbuf[2] = 'O';
    textbuf[3] = 'R', textbuf[4] = 'E', textbuf[5] = 0;
    textprint(3,4+statusbaryoffset,(char)176);
    textbuf[0] = 'T', textbuf[1] = 'I', textbuf[2] = 'M';
    textbuf[3] = 'E', textbuf[4] = 0;
    textprint(3,12+statusbaryoffset,(char)176);
    textbuf[0] = 'B', textbuf[1] = 'O', textbuf[2] = 'A';
    textbuf[3] = 'R', textbuf[4] = 'D', textbuf[5] = 0;
    textprint(3,20+statusbaryoffset,(char)176);
    textbuf[0] = ':', textbuf[1] = 0;
    textprint(41,4+statusbaryoffset,(char)176);
    textprint(33,12+statusbaryoffset,(char)176);
    textprint(41,20+statusbaryoffset,(char)176);
    textbuf[0] = 'L', textbuf[1] = 'I', textbuf[2] = 'F';
    textbuf[3] = 'E', textbuf[4] = 0;
    textprint(96,4+statusbaryoffset,(char)176);
    textbuf[0] = 'W', textbuf[1] = 'E', textbuf[2] = 'A';
    textbuf[3] = 'P', textbuf[4] = 'O', textbuf[5] = 'N';
    textbuf[6] = 0;
    textprint(272,3+statusbaryoffset,(char)176);
    if (hiscorenamstat == 1)
    {
	for(i=0;i<9;i++)
	    textbuf[i] = hiscorenam[i];
	textbuf[9] = 0;
	textprint(70,20+statusbaryoffset,(char)176);
    }
    statusbardraw(50,30,7,7,282,12+statusbaryoffset,statusbarinfo);
    statusbardraw(50,37,7,7,282,21+statusbaryoffset,statusbarinfo);
    statusbardraw(57,30,7,7,306,12+statusbaryoffset,statusbarinfo);
    statusbardraw(57,37,7,7,306,21+statusbaryoffset,statusbarinfo);
    statusbardraw(36,37,7,7,104,12+statusbaryoffset,statusbarinfo);
    statusbardraw(43,37,7,7,136,12+statusbaryoffset,statusbarinfo);
    statusbardraw(32,28,1,8,94,4+statusbaryoffset,statusbarinfo);
    statusbardraw(32,28,1,8,94,12+statusbaryoffset,statusbarinfo);
    oldlife = 0;
    drawlife();
    oldlife = 4095;
    drawlife();
    drawscore(scorecount);
    drawtime(scoreclock);
    textbuf[0] = ((boardnum+1)/10)+48;
    textbuf[1] = ((boardnum+1)%10)+48;
    textbuf[2] = 0;
    if (textbuf[0] == 48)
	textbuf[0] = 32;
    textprint(46,20+statusbaryoffset,(char)176);
    if (vidmode == 1)
    {
	statusbardraw(0,32,20,32,-20,0+statusbaryoffset,statusbarback);
	statusbardraw(0,32,20,32,320,0+statusbaryoffset,statusbarback);
    }
    textbuf[0] = 9, textbuf[1] = 0;
    textprint(96,12+statusbaryoffset,(char)0);
    textbuf[0] = lifevests+48, textbuf[1] = 0;
    textprint(96,12+statusbaryoffset,(char)176);
    textbuf[0] = 9, textbuf[1] = 0;
    textprint(296,21+statusbaryoffset,(char)0);
    textbuf[0] = lightnings+48, textbuf[1] = 0;
    textprint(296,21+statusbaryoffset,(char)176);
    textbuf[0] = 9, textbuf[1] = 0;
    textprint(272,12+statusbaryoffset,(char)0);
    textbuf[0] = firepowers[0]+48, textbuf[1] = 0;
    textprint(272,12+statusbaryoffset,(char)176);
    textbuf[0] = 9, textbuf[1] = 0;
    textprint(272,21+statusbaryoffset,(char)0);
    textbuf[0] = firepowers[1]+48, textbuf[1] = 0;
    textprint(272,21+statusbaryoffset,(char)176);
    textbuf[0] = 9, textbuf[1] = 0;
    textprint(296,12+statusbaryoffset,(char)0);
    textbuf[0] = firepowers[2]+48, textbuf[1] = 0;
    textprint(296,12+statusbaryoffset,(char)176);
    if (purpletime >= totalclock)
    {
	statusbardraw(0,0,16,15,159,13+statusbaryoffset,statusbarinfo);
	if (purpletime < totalclock+3840)
	{
	    i = ((3840-(int)(purpletime-totalclock))>>8);
	    if ((i >= 0) && (i <= 15))
		statusbardraw(0,30,16,i,159,13+statusbaryoffset,statusbarinfo);
	}
    }
    if (greentime >= totalclock)
    {
	statusbardraw(0,15,16,15,176,13+statusbaryoffset,statusbarinfo);
	if (greentime < totalclock+3840)
	{
	    i = ((3840-(int)(greentime-totalclock))>>8);
	    if ((i >= 0) && (i <= 15))
		statusbardraw(16,30,16,i,176,13+statusbaryoffset,statusbarinfo);
	}
    }
    if (capetime[0] >= totalclock)
    {
	statusbardraw(16,0,21,28,194,2+statusbaryoffset,statusbarinfo);
	if (capetime[0] < totalclock+3072)
	{
	    i = (int)((capetime[0]-totalclock)>>9);
	    if ((i >= 0) && (i <= 5))
	    {
		if (i == 5) statusbardraw(0,0,21,28,194,2+statusbaryoffset,coatfade);
		if (i == 4) statusbardraw(21,0,21,28,194,2+statusbaryoffset,coatfade);
		if (i == 3) statusbardraw(42,0,21,28,194,2+statusbaryoffset,coatfade);
		if (i == 2) statusbardraw(0,32,21,28,194,2+statusbaryoffset,coatfade);
		if (i == 1) statusbardraw(21,32,21,28,194,2+statusbaryoffset,coatfade);
		if (i == 0) statusbardraw(42,32,21,28,194,2+statusbaryoffset,coatfade);
	    }
	}
    }
    if (capetime[1] >= ototclock)
    {
	statusbardraw(37,0,21,28,216,2+statusbaryoffset,statusbarinfo);
	if (capetime[1] < totalclock+3072)
	{
	    i = (int)((capetime[1]-totalclock)>>9);
	    if ((i >= 0) && (i <= 5))
	    {
		if (i == 5) statusbardraw(0,0,21,28,216,2+statusbaryoffset,coatfade);
		if (i == 4) statusbardraw(21,0,21,28,216,2+statusbaryoffset,coatfade);
		if (i == 3) statusbardraw(42,0,21,28,216,2+statusbaryoffset,coatfade);
		if (i == 2) statusbardraw(0,32,21,28,216,2+statusbaryoffset,coatfade);
		if (i == 1) statusbardraw(21,32,21,28,216,2+statusbaryoffset,coatfade);
		if (i == 0) statusbardraw(42,32,21,28,216,2+statusbaryoffset,coatfade);
	    }
	}
    }
    if (keys[0] > 0)
	statusbardraw(36,44,14,6,144,13+statusbaryoffset,statusbarinfo);
    if (keys[1] > 0)
	statusbardraw(50,44,14,6,144,21+statusbaryoffset,statusbarinfo);
    statusbardraw(41,28,8,9,292,20+statusbaryoffset,statusbarinfo);
    statusbardraw(43,28,6,9,300,20+statusbaryoffset,statusbarinfo);
    statusbardraw(43,28,6,9,306,20+statusbaryoffset,statusbarinfo);
    statusbardraw(45,28,5,9,312,20+statusbaryoffset,statusbarinfo);
    /*if (bulchoose == 0) */
    i = 268, j = 11;
    if (bulchoose == 1) i = 268, j = 20;
    if (bulchoose == 2) i = 292, j = 11;
    statusbardraw(32,28,8,9,i,j+statusbaryoffset,statusbarinfo);
    statusbardraw(34,28,6,9,i+8,j+statusbaryoffset,statusbarinfo);
    statusbardraw(34,28,6,9,i+14,j+statusbaryoffset,statusbarinfo);
    statusbardraw(36,28,5,9,i+20,j+statusbaryoffset,statusbarinfo);
    textbuf[0] = 9, textbuf[1] = 9;
    textbuf[2] = 9, textbuf[3] = 0;
    textprint(112,12+statusbaryoffset,(char)0);
    textbuf[0] = (coins/100)+48;
    textbuf[1] = ((coins/10)%10)+48;
    textbuf[2] = (coins%10)+48;
    textbuf[3] = 0;
    if (textbuf[0] == 48)
    {
	textbuf[0] = 32;
	if (textbuf[1] == 48)
	    textbuf[1] = 32;
    }
    textprint(112,12+statusbaryoffset,(char)176);
}

/* Draw the hiscore box to the overlay... */

void drawscorebox() {
    int x,y;
    float ox=0.0,oy=0.0,od;
    unsigned char *orgl;
    unsigned char *org;
    
    unsigned char *dest;

    od=64.0/320.0;

    ox=0.0;
    for(x=0;(x<320)&&(ox<64.0);x++) {
	if (lab3dversion)
	    orgl=walseg[75]+(((int)ox)<<6);
	else
	    orgl=walseg[scorebox-1]+(((int)ox)<<6);
	oy=0.0;
	dest=screenbuffer+(18+x)+screenbufferwidth*20;

	for(y=0;(y<200)&&(oy<64.0);y++) {
	    org=orgl+((int)oy);
	    if ((*org)!=255)
		*dest=*org;

	    dest+=screenbufferwidth;
	    oy+=od;
	}
	ox+=od;
    }

    menuleft=18; menutop=20; menuwidth=320; menuheight=200;
    menuing=1;
}

/* Hiscore sequence; check for new hiscore and display hiscore box. */

void hiscorecheck()
{
    K_INT16 i, j, k, m, inse, namexist, fil;
    K_INT32 hiscore[8], scorexist, templong;

    if (((fil = open("hiscore.dat",O_RDWR|O_BINARY,0)) == -1)&&
	((fil = open("HISCORE.DAT",O_RDWR|O_BINARY,0)) == -1))
	return;
    gfxDrawFront();
    wipeoverlay(0,0,361,statusbaryoffset);
    lseek(fil,(long)(boardnum<<7),SEEK_SET);
    read(fil,&tempbuf[0],128);
    for(i=0;i<8;i++)
    {
//	hiscore[i]=*((K_INT32 *)(&tempbuf[i*16+12]));
	hiscore[i]=readlong(&tempbuf[i*16+12]);
    }
    /*
      for(i=lside;i<rside;i++)
      height[i] = 0;
    */

    /* Score box looks awful when interpolated, and we need it in the
       overlay for proper anti-aliasing, so software scale. */

    drawscorebox();
/*    spridraw(20,0,320*4,scorebox);*/
    for(i=0;i<8;i++)
	if (tempbuf[i<<4] != 0)
	{
	    textbuf[0] = i+49, textbuf[1] = '.', textbuf[2] = 32, textbuf[3] = 32;
	    for(j=0;j<12;j++)
		textbuf[j+4] = tempbuf[(i<<4)+j];
	    textbuf[16] = 0;
	    if (lab3dversion) {
		if (strcmp(textbuf+4, BADNAME)==0)
		    continue;
		textprint(55,60+(i<<3)+i+1,(char)128);
	    } else
		textprint(55,60+(i<<3)+i+1,(char)130);
	    setuptextbuf(hiscore[i]);
	    if (lab3dversion)
		textprint(215,60+(i<<3)+i+1,(char)0);
	    else
		textprint(215,60+(i<<3)+i+1,(char)96);
	}
    sprintf(&textbuf[0],"Time penalty: 10 * ");
    templong = (scoreclock*5)/12;
    textbuf[19] = (char)((templong/10000000L)%10L)+48;
    textbuf[20] = (char)((templong/1000000L)%10L)+48;
    textbuf[21] = (char)((templong/100000L)%10L)+48;
    textbuf[22] = (char)((templong/10000L)%10L)+48;
    textbuf[23] = (char)((templong/1000L)%10L)+48;
    textbuf[24] = (char)((templong/100L)%10L)+48;
    textbuf[25] = '.';
    textbuf[26] = (char)((templong/10L)%10L)+48;
    textbuf[27] = (char)(templong%10L)+48;
    textbuf[28] = 0;
    j = 19;
    while ((textbuf[j] == 48) && (j < 24))
	j++;
    for(i=19;i<28;i++)
	textbuf[i] = textbuf[i+j-19];
    if (lab3dversion)
	textprint(180-(strlen(textbuf)<<2),155+1,(char)106);
    else
	textprint(180-(strlen(textbuf)<<2),155+1,(char)114);
    scorecount -= (scoreclock/24);
    if (scorecount < 0)
	scorecount = 0;
    setuptextbuf(scorecount);
    if (lab3dversion)
	textprint(215,145+1,(char)33);
    else
	textprint(215,145+1,(char)35);
    if ((scorecount > hiscore[7]) && (cheated == 0))
    {
	if (hiscorenamstat == 0) {
	    getname();
	} else
	    finalisemenu();

	if (hiscorenamstat == 0)
	{
	    for(i=0;i<22;i++)
		textbuf[i] = 8;
	    textbuf[22] = 0;
	    textprint(180-(strlen(textbuf)<<2),135+1,(char)1);
	}
	else
	{
	    m = 0;
	    while ((hiscorenam[m] != 0) && (m < 11))
		m++;
	    namexist = -1;
	    for(i=0;i<8;i++)
	    {
		j = 0;
		for(k=0;k<=m;k++)
		    if (hiscorenam[k] != tempbuf[(i<<4)+k])
			j = 1;
		if (j == 0)
		    namexist = i;
	    }
	    scorexist = scorecount;
	    if (namexist != -1)
	    {
		if (hiscore[namexist] > scorexist)
		    scorexist = hiscore[namexist];
		for(j=namexist;j<7;j++)
		{
		    for(k=0;k<16;k++)
			tempbuf[(j<<4)+k]=tempbuf[((j+1)<<4)+k];
		    hiscore[j] = hiscore[j+1];
		}
		inse = 6;
	    }
	    else
		inse = 7;
	    while ((scorexist > hiscore[inse]) && (inse >= 0))
		inse--;
	    inse++;
	    for(j=7;j>inse;j--)
	    {
		for(k=0;k<16;k++)
		    tempbuf[(j<<4)+k]=tempbuf[((j-1)<<4)+k];
		hiscore[j] = hiscore[j-1];
	    }
	    for(k=0;k<12;k++)
		tempbuf[(inse<<4)+k] = hiscorenam[k];
	    hiscore[inse] = scorexist;
	    writelong(&tempbuf[inse*16+12],scorexist);

	    wipeoverlay(0,0,361,statusbaryoffset);
	    drawscorebox();
	    if (scorecount == scorexist)
	    {
		switch(inse)
		{
		    case 0: strcpy(&textbuf[0],"Congratulations!!! You're #1!"); break;
		    case 1: strcpy(&textbuf[0],"Good Job!! Second place!"); break;
		    case 2: strcpy(&textbuf[0],"Not Bad - You got third place!"); break;
		    case 3: strcpy(&textbuf[0],"Fourth place! Getting there..."); break;
		    case 4: strcpy(&textbuf[0],"Fifth place! Could be improved..."); break;
		    case 5: strcpy(&textbuf[0],"Sixth Place...  Keep trying..."); break;
		    case 6: strcpy(&textbuf[0],"Seventh? Really want your name here?"); break;
		    case 7: strcpy(&textbuf[0],"Eighth? Have a load in your pants?"); break;
		};
	    }
	    else
		strcpy(&textbuf[0],"Are those reflexes dying?");
	    textprint(180-(strlen(textbuf)<<2),45+1,lab3dversion?64:(char)65);
	    for(i=0;i<8;i++)
		if (tempbuf[i<<4] != 0)
		{
		    textbuf[0] = i+49, textbuf[1] = '.', textbuf[2] = 32, textbuf[3] = 32;
		    for(j=0;j<12;j++)
			textbuf[j+4] = tempbuf[(i<<4)+j];
		    textbuf[16] = 0;
		    if (lab3dversion)
			if (strcmp(textbuf+4, BADNAME)==0)
			    continue;
		    if (i == inse)
		    {
			textprint(55,60+(i<<3)+i+1,lab3dversion?0:(char)98);
			for(j=0;j<12;j++)
			    textbuf[j] = textbuf[j+4];
			textprint(87,145+1,lab3dversion?96:(char)98);
		    }
		    else
			textprint(55,60+(i<<3)+i+1,lab3dversion?128:(char)130);
		    setuptextbuf(hiscore[i]);
		    textprint(215,60+(i<<3)+i+1,lab3dversion?0:(char)96);
		}
	    sprintf(&textbuf[0],"Time penalty: 10 * ");
	    textbuf[19] = (char)((templong/10000000L)%10L)+48;
	    textbuf[20] = (char)((templong/1000000L)%10L)+48;
	    textbuf[21] = (char)((templong/100000L)%10L)+48;
	    textbuf[22] = (char)((templong/10000L)%10L)+48;
	    textbuf[23] = (char)((templong/1000L)%10L)+48;
	    textbuf[24] = (char)((templong/100L)%10L)+48;
	    textbuf[25] = '.';
	    textbuf[26] = (char)((templong/10L)%10L)+48;
	    textbuf[27] = (char)(templong%10L)+48;
	    textbuf[28] = 0;
	    j = 19;
	    while ((textbuf[j] == 48) && (j < 24))
		j++;
	    for(i=19;i<28;i++)
		textbuf[i] = textbuf[i+j-19];
	    textprint(180-(strlen(textbuf)<<2),155+1,lab3dversion?106:114);
	    setuptextbuf(scorecount);
	    textprint(215,145+1,lab3dversion?33:(char)35);
	    lseek(fil,(long)(boardnum<<7),SEEK_SET);
	    write(fil,&tempbuf[0],128);
	}
    }
    else
    {
	i = rand()&3;
	if (cheated == 0)
	{
	    switch(i)
	    {
		case 0: strcpy(&textbuf[0],"You rot if you can't beat ");
		    j = 0;
		    while (tempbuf[(i<<4)+j] != 0)
		    {
			textbuf[j+26] = tempbuf[(i<<4)+j];
			j++;
		    }
		    textbuf[j+26] = 0;
		    break;
		case 1: strcpy(&textbuf[0],"Try this level again if you're good."); break;
		case 2: strcpy(&textbuf[0],"Nice job, but no score like Ken's."); break;
		case 3: strcpy(&textbuf[0],"Try playing for speed next time."); break;
	    };
	}
	else
	{
	    switch(i)
	    {
		case 0: strcpy(&textbuf[0],"Try playing for real next time."); break;
		case 1: strcpy(&textbuf[0],"Don't touch those cheat keys!"); break;
		case 2: strcpy(&textbuf[0],"Cheating doesn't pay."); break;
		case 3: strcpy(&textbuf[0],"You can't cheat a score."); break;
	    };
	}
	textprint(180-(strlen(textbuf)<<2),45+1,(char)161);
    }
    close(fil);
    sprintf(&textbuf[0],"Press any key to continue.");
    textprint(180-(strlen(textbuf)<<2),135+1,(char)65);
    finalisemenu();
    gfxFlush();
    gfxDrawBack();
    while ((keystatus[1] == 0) && (keystatus[57] == 0) && (keystatus[28] == 0) && (bstatus == 0)) {
	PollInputs();
	SDL_Delay(10);
    }
}

/* Convert integer to string. */

void setuptextbuf(K_INT32 templong)
{
    int i;

    textbuf[0] = (char)+48;
    textbuf[1] = (char)+48;
    textbuf[2] = (char)+48;
    textbuf[3] = (char)((templong/10000000L)%10L)+48;
    textbuf[4] = (char)((templong/1000000L)%10L)+48;
    textbuf[5] = (char)((templong/100000L)%10L)+48;
    textbuf[6] = (char)((templong/10000L)%10L)+48;
    textbuf[7] = (char)((templong/1000L)%10L)+48;
    textbuf[8] = (char)((templong/100L)%10L)+48;
    textbuf[9] = (char)((templong/10L)%10L)+48;
    textbuf[10] = (char)(templong%10L)+48;
    i = 0;
    while ((textbuf[i] == 48) && (i < 10))
	textbuf[i++] = 32;
    textbuf[11] = 0;
}

/* Get player's name... */

void getname()
{
    unsigned char ch;
    K_INT16 i, j;

    /* Apparently, the program is supposed to remember the name you used last
       time you played and allow you to modify that.
      
       In practice, this code is so buggy and useless that I think it's best to
       just leave it out. */

/*    if (namrememberstat == 0)
      {*/
    for(j=0;j<16;j++)
	hiscorenam[j] = 0;
    for(j=0;j<12;j++)
	textbuf[j] = 8;
    textbuf[12] = 0;
    textprint(94,145+1,(char)0);
    j = 0;
/*    }
      else
      {
      for(j=0;j<12;j++)
      textbuf[j] = hiscorenam[j];
      textbuf[12] = 0;
      j = 12;
      while ((hiscorenam[j] == 0) && (j > 0))
      j--;
      textprint(94,125+1,(char)97);
      }*/

    ch = 0;
    finalisemenu();    
    gfxDrawFront();
    sprintf(&textbuf[0],"Please type your name!");

    textprint(180-(strlen(textbuf)<<2),135+1,(char)161);
    ch = 0;
    SDL_EnableUNICODE(1);
    while ((ch != 13) && (ch != 27))
    {
	while ((ch=getkeypress()) == 0)
	{
	    textbuf[0] = 95;
	    textbuf[1] = 0;
	    textprint(94+(j<<3),145,(char)97);
	    gfxFlush();
	    SDL_Delay(10); /* Just to avoid soaking all CPU. */
	    textbuf[0] = 8;
	    textbuf[1] = 0;
	    textprint(94+(j<<3),145,(char)0);
	    gfxFlush();
	    SDL_Delay(10); /* Just to avoid soaking all CPU. */
	}
	if (ch == SDLK_DELETE)
	{
	    hiscorenam[j] = ch;
	    for(j=0;j<16;j++)
		hiscorenam[j] = 0;
	    for(j=0;j<12;j++)
		textbuf[j] = 8;
	    textbuf[12] = 0;
	    textprint(94,145+1,(char)0);
	    j = 0;
	    ch = 0;
	}
	if ((ch == 8) && (j > 0))
	{
	    j--, hiscorenam[j] = 0;
	    textbuf[0] = ch;
	    textbuf[1] = 0;
	    textprint(94+(j<<3),145+1,(char)0);
	}
	if ((ch >= 32) && (ch <= 127) && (j < 12))
	{
	    textbuf[0] = ch;
	    textbuf[1] = 0;
	    textprint(94+(j<<3),145+1,(char)97);
	    hiscorenam[j] = ch;
	    if ((ch != 32) || (j > 0))
		j++;
	}
    }
    SDL_EnableUNICODE(0);
    for(i=0;i<256;i++)
	keystatus[i] = 0;
    hiscorenam[j] = 0;
    i = j-1;
    while ((hiscorenam[i] == 32) && (i >= 0))
	hiscorenam[i--] = 0;
    if ((hiscorenam[0] == 0) || (ch == 27))
    {
	hiscorenamstat = 0;
	hiscorenam[0] = 0;
    }
    else
    {
	if (hiscorenam[0] == 0)
	    hiscorenamstat = 0;
	else
	{
	    hiscorenamstat = 1;
	    for(i=0;i<9;i++)
		textbuf[i] = hiscorenam[i];
	    textbuf[9] = 0;
	    if (lab3dversion==0)
		textprint(70,20+statusbaryoffset,(char)177);
	}
    }
}

/* Draw score... */

void drawscore(K_INT32 thescore)
{
    K_INT16 i;

    for(i=0;i<6;i++)
	textbuf[i] = 9;
    textbuf[6] = 0;
    textprint(46,4+statusbaryoffset,(char)0);
    textbuf[0] = (char)((thescore/100000L)%10L)+48;
    textbuf[1] = (char)((thescore/10000L)%10L)+48;
    textbuf[2] = (char)((thescore/1000L)%10L)+48;
    textbuf[3] = (char)((thescore/100L)%10L)+48;
    textbuf[4] = (char)((thescore/10L)%10L)+48;
    textbuf[5] = (char)(thescore%10L)+48;
    textbuf[6] = 0;
    i = 0;
    while ((textbuf[i] == 48) && (i < 5))
	textbuf[i++] = 32;
    textprint(46,4+statusbaryoffset,(char)192);
}

/* Draw time... */

void drawtime(K_INT32 thetime)
{
    K_INT16 i;

    for(i=0;i<7;i++)
	textbuf[i] = 9;
    textbuf[7] = 0;
    textprint(38,12+statusbaryoffset,(char)0);
    thetime = thetime/240;
    textbuf[0] = (char)((thetime/1000000L)%10L)+48;
    textbuf[1] = (char)((thetime/100000L)%10L)+48;
    textbuf[2] = (char)((thetime/10000L)%10L)+48;
    textbuf[3] = (char)((thetime/1000L)%10L)+48;
    textbuf[4] = (char)((thetime/100L)%10L)+48;
    textbuf[5] = (char)((thetime/10L)%10L)+48;
    textbuf[6] = (char)(thetime%10L)+48;
    textbuf[7] = 0;
    i = 0;
    while ((textbuf[i] == 48) && (i < 6))
	textbuf[i++] = 32;
    textprint(38,12+statusbaryoffset,(char)192);
}

/* Draw main menu. Separated to eliminate need for saving screen contents. */

void drawmainmenu() {
    K_INT16 n;

    if (vidmode == 0)
	n = 0;
    else
	n = 20;

    drawmenu(192,128,menu);
    strcpy(&textbuf[0],"New game");
    textprint(131,47+n+1,32);
    strcpy(&textbuf[0],"Load game");
    textprint(131,59+n+1,32);
    strcpy(&textbuf[0],"Save game");
    textprint(131,71+n+1,32);
    strcpy(&textbuf[0],"Return to game");
    textprint(131,83+n+1,32);
    strcpy(&textbuf[0],"Help");
    textprint(131,95+n+1,126);
    strcpy(&textbuf[0],"Story");
    textprint(131,107+n+1,126);
    strcpy(&textbuf[0],"Copyright notice");
    textprint(131,119+n+1,126);
    strcpy(&textbuf[0],"Credits");
    textprint(131,131+n+1,126);
    strcpy(&textbuf[0],"Exit");
    textprint(131,143+n+1,126);
    finalisemenu();
}

/* Main menu... */

K_INT16 mainmenu()
{
    K_INT16 j, k, done;

    spriteyoffset=0;

    ksay(27);
    fade(63);
    if (sortcnt == -1) {
	/* Emulating the original a bit too closely, this... */

	gfxDrawBack();
	spriteyoffset=20;
	drawintroduction();
	spriteyoffset=0;
	gfxSwapBuffers();
    } else {
	gfxDrawFront();
	ShowStatusBar();
    }

    gfxDrawFront();

    done = 0;
    if (sortcnt != -1)
    {
	wipeoverlay(0,0,361,statusbaryoffset);
    }
    j = scrsize;
    if (j < 18000)
	j = 18000;
    if (vidmode == 1)
	j = 21600;

    drawmainmenu();

    while ((mainmenuplace >= 0) && (done == 0))
    {
	if ((mainmenuplace = getselection(88,47,mainmenuplace,9)) >= 0)
	{
	    if (mainmenuplace == 0)
		if ((k = newgamemenu()) >= 0)
		{
		    done = 1;
		    if ((k == 1) && (numboards < 20))
		    {
			ksay(12);
			done = 0;
		    }
		    else if ((k == 2) && (numboards < 30))
		    {
			ksay(12);
			done = 0;
		    }
		}
	    if (mainmenuplace == 1)
		if (loadsavegamemenu(1) >= 0)
		    done = 1;
	    if (mainmenuplace == 2)
	    {
		if (sortcnt == -1)
		    ksay(12);
		else
		{
		    if (loadsavegamemenu(2) >= 0)
			done = 1;
		}
	    }
	    if (mainmenuplace == 3)
		mainmenuplace = (-mainmenuplace)-1;
	    if (mainmenuplace == 4) helpmenu();
	    if (mainmenuplace == 5) bigstorymenu();
	    if (mainmenuplace == 6) orderinfomenu();
	    if (mainmenuplace == 7) creditsmenu();
	    if (mainmenuplace == 8) done = areyousure();
	    if (done == 0)
	    {
		/* Redraw whatever was beneath the menu. Double buffer to
		   avoid annoying flicker. */
		if (sortcnt == -1) {		    
		    spriteyoffset=20;
		    gfxDrawBack();
		    kgif(1);
		    drawintroduction();
		    spriteyoffset=0;
		    drawmainmenu();
		    gfxSwapBuffers();
		    gfxDrawFront();
		}
		else {
		    gfxDrawBack();
		    wipeoverlay(0,0,361,statusbaryoffset);
		    statusbaralldraw();
		    if (compass>0) showcompass(ang);
		    picrot(posx,posy,posz,ang);
		    drawmainmenu();
		    gfxSwapBuffers();
		    gfxDrawFront();
		}
	    }
	}
    }
    if (sortcnt != -1) {
	linecompare(statusbar);
	wipeoverlay(0,0,361,statusbaryoffset);
    }
    else
	kgif(1);
    j = mainmenuplace;
    if (mainmenuplace < 0)
	mainmenuplace = (-mainmenuplace)-1;
    gfxDrawBack();
    return(j);
}

/* Get a selection from a menu with totselectors choices, defaulting to
   nowselector, at screen position (xoffs,yoffs). */

K_INT16 getselection(K_INT16 xoffs, K_INT16 yoffs, K_INT16 nowselector, 
		     K_INT16 totselectors)
{
    K_INT16 animater6, n, esckeystate;
    int mousx, mousy;
    K_INT16 bstatus, obstatus;

    gfxDrawFront();
    if (vidmode == 0)
	n = 0;
    else
	n = 20;
    keystatus[0x48] = 0, keystatus[0xc8] = 0, keystatus[0xcb] = 0;
    keystatus[0xd0] = 0, keystatus[0x50] = 0, keystatus[0xcd] = 0;
    keystatus[0x01] = 0, keystatus[0x1c] = 0, keystatus[0x9c] = 0;
    keystatus[0x39] = 0;
    newkeystatus[SDLK_KP_ENTER]=0;
    newkeystatus[SDLK_RETURN]=0;
    newkeystatus[SDLK_SPACE]=0;
    newkeystatus[SDLK_ESCAPE]=0;
    newkeystatus[SDLK_UP]=newkeystatus[SDLK_DOWN]=newkeystatus[SDLK_LEFT]=
	newkeystatus[SDLK_RIGHT]=0;
    newkeystatus[SDLK_KP2]=newkeystatus[SDLK_KP8]=0;
    animater6 = 0;
    esckeystate = 0;
    bstatus = 1;
    obstatus = 1;
    mousx = 0;
    mousy = 0;
    while (esckeystate == 0)
    {
	PollInputs();
	animater6++;
	if (animater6 == 6)
	    animater6 = 0;

	SDL_Delay(10); /* Let's not soak up all CPU... */

	if (lab3dversion) {
	    statusbardraw(16+(animater6/2)*16,0,15,15,xoffs+20-n,nowselector*12+yoffs+n-1,85);
	} else {
	    if (animater6 < 3)
		statusbardraw(20+animater6*14,32,13,13,xoffs+20-n,nowselector*12+yoffs+n-1+1,statusbarback);
	    else
		statusbardraw(20+(animater6-3)*14,46,13,13,xoffs+20-n,nowselector*12+yoffs+n-1+1,statusbarback);
	}
	obstatus = bstatus;
	if (moustat == 0) {
	    bstatus=readmouse(&mousx,&mousy);
	}
	if (((keystatus[0x48]|keystatus[0xc8]|keystatus[0xcb]) != 0) || (mousy < -128))
	{
	    if (mousy < -128)
		mousy += 128;
	    if (lab3dversion)
		wipeoverlay(xoffs+39-n,nowselector*12+yoffs+n-1,15,15);
	    else
		statusbardraw(16,15,13,13,xoffs+20-n,nowselector*12+yoffs+n-1+1,menu);
	    nowselector--;
	    ksay(27);
	    if (nowselector < 0)
		nowselector = totselectors-1;
	    keystatus[0x48] = 0, keystatus[0xc8] = 0, keystatus[0xcb] = 0;
	    newkeystatus[SDLK_UP]=newkeystatus[SDLK_KP8]=
		newkeystatus[SDLK_LEFT]=0;
	}
	if (((keystatus[0xd0]|keystatus[0x50]|keystatus[0xcd]) != 0) || (mousy > 128))
	{
	    if (mousy > 128)
		mousy -= 128;
	    if (lab3dversion)
		wipeoverlay(xoffs+39-n,nowselector*12+yoffs+n-1,15,15);
	    else
		statusbardraw(16,15,13,13,xoffs+20-n,nowselector*12+yoffs+n-1+1,menu);
	    nowselector++;
	    ksay(27);
	    if (nowselector == totselectors)
		nowselector = 0;
	    keystatus[0xd0] = 0, keystatus[0x50] = 0, keystatus[0xcd] = 0;
	    newkeystatus[SDLK_DOWN]=newkeystatus[SDLK_KP2]=
		newkeystatus[SDLK_RIGHT]=0;
	}
	esckeystate = (keystatus[1]|(keystatus[0x1c]<<1)|(keystatus[0x9c]<<1)|(keystatus[0x39]<<1));
	if ((obstatus == 0) && (bstatus > 0))
	    esckeystate |= (bstatus^3);
	gfxFlush();
    }
    ksay(27);
    if (lab3dversion)
	wipeoverlay(xoffs+39-n,nowselector*12+yoffs+n-1,15,15);
    else
	statusbardraw(36-n,15,13,13,xoffs+20-n,nowselector*12+yoffs+n-1+1,menu);
    if ((esckeystate&2) > 0)
	return(nowselector);
    else
	return((-nowselector)-1);
}

void finalisemenu(void) {
    menuing=0;
    gfxUploadPartialOverlay(menuleft,menutop,menuwidth,menuheight);
}

/* Draw menu background and frame... */

void drawmenu(K_INT16 xsiz, K_INT16 ysiz, K_INT16 walnume)
{
    K_INT16 ycent, i, j, x1, y1, x2, y2;
    unsigned char *buf;

    if (vidmode == 0)
	ycent = 100;
    else
	ycent = 120;

    if (vidmode == 0)
	x1 = 180-(xsiz>>1);
    else
	x1 = 160-(xsiz>>1);
    y1 = ycent-(ysiz>>1)-1;
    x2 = x1+xsiz-16;
    y2 = y1+ysiz-16;

    menuleft=x1+20;
    menutop=y1+1+visiblescreenyoffset+spriteyoffset;
    menuwidth=xsiz;
    menuheight=ysiz;
    menuing=1;

    if (ysiz>240) {menutop=0; menuheight=240;}

    if (lab3dversion) {
	wipeoverlay(menuleft, menutop, menuwidth, menuheight);
	buf=screenbuffer+(screenbufferwidth*menutop+(menuleft));
	for(i=0;i<menuwidth;i++)
	    *(buf++)=239;
	buf=screenbuffer+(screenbufferwidth*(menutop+menuheight-1)+menuleft);
	for(i=0;i<menuwidth;i++)
	    *(buf++)=239;
	buf=screenbuffer+(screenbufferwidth*menutop+(menuleft));
	for(i=0;i<menuwidth;i++)
	    *(buf+=screenbufferwidth)=239;
	buf=screenbuffer+(screenbufferwidth*menutop+(menuleft+menuwidth-1));
	for(i=0;i<menuwidth;i++)
	    *(buf+=screenbufferwidth)=239;
	gfxUploadPartialOverlay(menuleft,menutop,menuwidth,menuheight);
	
	return;
    }

    if (ysiz<=240) {
	statusbardraw(0,0,16,16,x1,y1+1,walnume);
	statusbardraw(48,0,16,16,x2,y1+1,walnume);
	statusbardraw(0,48,16,16,x1,y2+1,walnume);
	statusbardraw(48,48,16,16,x2,y2+1,walnume);
	for(i=x1+16;i<x2;i+=16)
	{
	    statusbardraw(16,0,16,16,i,y1+1,walnume);
	    statusbardraw(16,48,16,16,i,y2+1,walnume);
	}
    } else {
	y1=-17; y2=239;
    }

    for(j=y1+16;j<y2;j+=16)
    {
	statusbardraw(0,16,16,16,x1,j+1,walnume);
	statusbardraw(48,16,16,16,x2,j+1,walnume);
    }
    for(i=x1+16;i<x2;i+=16)
	for(j=y1+16;j<y2;j+=16)
	    statusbardraw(16,16,16,16,i,j+1,walnume);
}

/* Show credits. */

void creditsmenu()
{
    K_INT16 n;

    if (vidmode == 0)
	n = 0;
    else
	n = 20;
    drawmenu(320,176,menu);
    strcpy(&textbuf[0],"Credits");
    textprint(149,20+n+1,32);
    loadstory(-1);
    finalisemenu();
    gfxFlush();
    pressakey();
}

/* Show story for episode... */

void bigstorymenu()
{
    K_INT16 i, j, k, nowenterstate, lastenterstate, quitstat, bstatus, obstatus;

/*    if (vidmode == 0)
	n = 0;
    else
    n = 20;*/
    if (boardnum < 10) j = -32, k = -27;
    else if (boardnum < 20) j = -26, k = -24;
    else j = -23, k = -22;
    quitstat = 0;
    bstatus = 1;
    obstatus = 1;
    i = j;
    while (quitstat == 0)
    {
	drawmenu(304,192,menu);
	loadstory(i);
	finalisemenu();
	nowenterstate = 1;
	lastenterstate = 1;
	gfxFlush();
	while ((nowenterstate <= lastenterstate) && (bstatus <= obstatus))
	{
	    PollInputs();
	    lastenterstate = nowenterstate;
	    nowenterstate = keystatus[0x1c];
	    nowenterstate |= keystatus[0x9c];
	    nowenterstate |= keystatus[1];
	    nowenterstate |= newkeystatus[newkeydefs[17]];
	    nowenterstate |= keystatus[0xc9];
	    nowenterstate |= keystatus[0xc8];
	    nowenterstate |= keystatus[0xcb];
	    nowenterstate |= keystatus[0xd1];
	    nowenterstate |= keystatus[0xd0];
	    nowenterstate |= keystatus[0xcd];
	    obstatus = bstatus;
	    if (moustat == 0) {
		bstatus=readmouse(NULL,NULL);
	    }
	}
	if (((keystatus[0xc9]|keystatus[0xc8]|keystatus[0xcb]) > 0) && (i > j))
	    i--;
	if (((keystatus[0xd1]|keystatus[0xd0]|keystatus[0xcd]) > 0) && (i < k))
	    i++;
	quitstat = (newkeystatus[newkeydefs[17]]|keystatus[1]);
	if (((keystatus[0x1c]|keystatus[0x9c]) > 0) || (bstatus > obstatus))
	{
	    bstatus = 1;
	    obstatus = 1;
	    i++;
	    if (i > k)
		quitstat = 1;
	}
	ksay(27);
    }
}

/* Quit (Y/N)? */

K_INT16 areyousure()
{
    K_INT16 i, n;

    if (vidmode == 0)
	n = 0;
    else
	n = 20;
    drawmenu(224,64,menu);
    strcpy(&textbuf[0],"Really want to quit?");
    textprint(99,84+n+1,112);
    strcpy(&textbuf[0],"Yes");
    textprint(105,96+n+1,32);
    strcpy(&textbuf[0],"No");
    textprint(105,108+n+1,32);
    finalisemenu();
    i = getselection(60,95,0,2);
    if (i == 0)
	return(1);
    else
	return(0);
}

/* Help screen. */

void helpmenu()
{
    K_INT16 n;

    if (vidmode == 0)
	n = 0;
    else
	n = 20;
    drawmenu(256,176,menu);
    loadstory(-15);
    strcpy(&textbuf[0],"Help");
    textprint(161,18+n+1,32);
    finalisemenu();
    gfxFlush();
    pressakey();
}

/* Soda menu. */

void sodamenu()
{
    K_INT32 ototclocker;
    K_INT16 n, valid;

    wipeoverlay(0,0,361,statusbaryoffset);
    gfxDrawFront();
    ototclocker = totalclock;
    if (vidmode == 0)
	n = 0;
    else
	n = 20;
    ksay(27);
    drawmenu(256,160,menu);
    if (boardnum < 10)
	loadstory(-34);
    else
	loadstory(-33);
    statusbardraw(0,0,12,36,85-n,49+n+1,sodapics);
    statusbardraw(12,0,12,36,85-n,85+n+1,sodapics);
    statusbardraw(24,0,12,36,85-n,121+n+1,sodapics);
    statusbardraw(36,0,12,12,85-n,157+n+1,sodapics);
    valid = 0;
    finalisemenu();
    while (valid == 0)
    {
	sodaplace = getselection(46,49,sodaplace,10);
	if (sodaplace >= 0)
	{
	    valid = 1;
	    if ((sodaplace == 0) && (coins < 1)) valid = 0;
	    if ((sodaplace == 1) && (coins < 2)) valid = 0;
	    if ((sodaplace == 2) && (coins < 2)) valid = 0;
	    if ((sodaplace == 3) && (coins < 5)) valid = 0;
	    if ((sodaplace == 4) && ((coins < 5) || (boardnum < 10))) valid = 0;
	    if ((sodaplace == 5) && (coins < 75)) valid = 0;
	    if ((sodaplace == 6) && (coins < 100)) valid = 0;
	    if ((sodaplace == 7) && (coins < 150)) valid = 0;
	    if ((sodaplace == 8) && ((coins < 200) || (boardnum < 10))) valid = 0;
	    if ((sodaplace == 9) && (coins < 250)) valid = 0;
	    if (valid == 0)
		ksay(12);
	}
	else
	    valid = 1;
    }
    if ((sodaplace >= 0) && (valid == 1))
    {
	ksay(24);
	switch(sodaplace)
	{
	    case 0:
		coins--;
		life += 320;
		if (life > 4095)
		    life = 4095;
		drawlife();
		break;
	    case 1:
		coins -= 2;
		if (purpletime < totalclock)
		    purpletime = totalclock+9600;
		else purpletime += 9600;
		statusbardraw(0,0,16,15,159,13+statusbaryoffset,statusbarinfo);
		break;
	    case 2:
		coins -= 2;
		if (greentime < totalclock)
		    greentime = totalclock + 9600;
		else greentime += 9600;
		statusbardraw(0,15,16,15,176,13+statusbaryoffset,statusbarinfo);
		break;
	    case 3:
		coins -= 5;
		if (capetime[0] < totalclock)
		    capetime[0] = totalclock + 7200;
		else
		    capetime[0] += 7200;
		statusbardraw(16,0,21,28,194,2+statusbaryoffset,statusbarinfo);
		break;
	    case 4:
		coins -= 5;
		if (capetime[1] < totalclock)
		    capetime[1] = totalclock + 4800;
		else
		    capetime[1] += 4800;
		statusbardraw(37,0,21,28,216,2+statusbaryoffset,statusbarinfo);
		break;
	    case 5:
		coins -= 75;
		lightnings++;
		if (lightnings > 6)
		    lightnings = 6;
		textbuf[0] = 9, textbuf[1] = 0;
		textprint(296,21+statusbaryoffset,(char)0);
		textbuf[0] = lightnings+48, textbuf[1] = 0;
		textprint(296,21+statusbaryoffset,(char)176);
		break;
	    case 6:
		coins -= 100;
		firepowers[0]++;
		if (firepowers[0] > 6)
		    firepowers[0] = 6;
		textbuf[0] = 9, textbuf[1] = 0;
		textprint(272,12+statusbaryoffset,(char)0);
		textbuf[0] = firepowers[0]+48, textbuf[1] = 0;
		textprint(272,12+statusbaryoffset,(char)176);
		break;
	    case 7:
		coins -= 150;
		firepowers[1]++;
		if (firepowers[1] > 6)
		    firepowers[1] = 6;
		textbuf[0] = 9, textbuf[1] = 0;
		textprint(272,21+statusbaryoffset,(char)0);
		textbuf[0] = firepowers[1]+48, textbuf[1] = 0;
		textprint(272,21+statusbaryoffset,(char)176);
		break;
	    case 8:
		coins -= 200;
		firepowers[2]++;
		if (firepowers[2] > 6)
		    firepowers[2] = 6;
		textbuf[0] = 9, textbuf[1] = 0;
		textprint(296,12+statusbaryoffset,(char)0);
		textbuf[0] = firepowers[2]+48, textbuf[1] = 0;
		textprint(296,12+statusbaryoffset,(char)176);
		break;
	    case 9:
		coins -= 250;
		compass = 1;
		break;
	}
	textbuf[0] = 9, textbuf[1] = 9;
	textbuf[2] = 9, textbuf[3] = 0;
	textprint(112,12+statusbaryoffset,(char)0);
	textbuf[0] = (coins/100)+48;
	textbuf[1] = ((coins/10)%10)+48;
	textbuf[2] = (coins%10)+48;
	textbuf[3] = 0;
	if (textbuf[0] == 48)
	{
	    textbuf[0] = 32;
	    if (textbuf[1] == 48)
		textbuf[1] = 32;
	}
	textprint(112,12+statusbaryoffset,(char)176);
    }
    if (sodaplace < 0)
    {
	sodaplace = (-sodaplace)-1;
	ksay(26);
    }
    totalclock = ototclocker;
    SDL_LockMutex(timermutex);
    clockspeed = 0;
    SDL_UnlockMutex(timermutex);
    gfxDrawBack();
    wipeoverlay(0,0,361,statusbaryoffset);
    linecompare(statusbar);
}

/* New credits instead of ordering info. */

void orderinfomenu() {
    drawmenu(320,106,menu);

    strcpy(textbuf,
	   "\"Ken's Labyrinth\"");
    textprint(30,76,32);
    strcpy(textbuf,"Copyright (c) 1992-1993 Ken Silverman");

    textprint(30,86,32);

    strcpy(textbuf,
	   "\"LAB3D/SDL\" conversion");
    textprint(30,96,32);

    /* Fonts only have 7-bit ASCII, and my surname needs a character not in
       this set. Fake the dots. */

    strcpy(textbuf,
	   ".");
    textprint(260,100,32);
    textprint(264,100,32);
    strcpy(textbuf,
	   "Copyright (c) 2002-2013 Jan Lonnberg");
    textprint(30,106,32);

    strcpy(textbuf,
	   "http://icculus.org/LAB3D/");
    textprint(30,116,32);

    strcpy(textbuf,
	   "See readme.txt for license");
    textprint(30,131,32);

    strcpy(textbuf,
	   "Ken Silverman's official web site:");
    textprint(30,146,48);
    
    strcpy(textbuf,
	   "http://www.advsys.net/ken");
    textprint(30,156,48);

    finalisemenu();
    gfxFlush();
    pressakey();
    
}

/* Save/load game selector. */

K_INT16 loadsavegamemenu(K_INT16 whichmenu)
{
    char filename[20];
    K_INT16 fil, i, j, k, n;
    K_INT32 templong;

    if (vidmode == 0)
	n = 0;
    else
	n = 20;
    drawmenu(320,160,menu);
    if (whichmenu == 1)
    {
	strcpy(&textbuf[0],"Load game");
	textprint(137,26+n+1,32);
    }
    else
    {
	strcpy(&textbuf[0],"Save game");
	textprint(137,26+n+1,112);
    }
    strcpy(&textbuf[0],"#: Name:       Board: Score: Time:");
    textprint(55,52+n+1,48);
    if (gameheadstat == 0)
    {
	for(j=0;j<8;j++)
	{
	    filename[0] = 'S', filename[1] = 'A', filename[2] = 'V';
	    filename[3] = 'G', filename[4] = 'A', filename[5] = 'M';
	    filename[6] = 'E', filename[7] = j+48;
	    filename[8] = '.', filename[9] = 'D', filename[10] = 'A';
	    filename[11] = 'T', filename[12] = 0;

	    if((fil=open(filename,O_RDONLY|O_BINARY,0))!=-1)
	    {
		gamexist[j] = 1;
		read(fil,&gamehead[j][0],27);
		close(fil);
	    }
	    else {
		filename[0] = 's', filename[1] = 'a', filename[2] = 'v';
		filename[3] = 'g', filename[4] = 'a', filename[5] = 'm';
		filename[6] = 'e', filename[7] = j+48;
		filename[8] = '.', filename[9] = 'd', filename[10] = 'a';
		filename[11] = 't', filename[12] = 0;	
		if((fil=open(filename,O_RDONLY|O_BINARY,0))!=-1)
		{
		    gamexist[j] = 1;
		    read(fil,&gamehead[j][0],27);
		    close(fil);
		}
		else
		    gamexist[j] = 0;
	    }
	}
	gameheadstat = 1;
    }
    j = 0;
    for(i=70+n;i<166+n;i+=12)
    {
	if (gamexist[j] == 1)
	{
	    textbuf[0] = j+49, textbuf[1] = 32, textbuf[2] = 32;
	    for(k=0;k<12;k++)
	    {
		textbuf[k+3] = gamehead[j][k];
		if (textbuf[k+3] == 0)
		    textbuf[k+3] = 32;
	    }
	    textbuf[15] = 32;
	    textbuf[16] = ((gamehead[j][17]+1)/10)+48;
	    if (textbuf[16] == 48)
		textbuf[16] = 32;
	    textbuf[17] = ((gamehead[j][17]+1)%10)+48;
	    textbuf[18] = 32;
	    textbuf[19] = 32;
	    textbuf[20] = 32;
	    k = j*27;
	    templong=readlong(&gamehead[j][19]);
//	    templong=*((K_INT32 *)(&gamehead[j][19]));

	    textbuf[21] = (char)((templong/100000L)%10L)+48;
	    textbuf[22] = (char)((templong/10000L)%10L)+48;
	    textbuf[23] = (char)((templong/1000L)%10L)+48;
	    textbuf[24] = (char)((templong/100L)%10L)+48;
	    textbuf[25] = (char)((templong/10L)%10L)+48;
	    textbuf[26] = (char)(templong%10L)+48;
	    textbuf[27] = 32;
	    k = 21;
	    while ((textbuf[k] == 48) && (k < 26))
		textbuf[k++] = 32;
	    k = j*27;
	    templong=readlong(&gamehead[j][23]);
//	    templong=*((K_INT32 *)(&gamehead[j][23]));

	    templong /= 240;
	    textbuf[28] = (char)((templong/10000L)%10L)+48;
	    textbuf[29] = (char)((templong/1000L)%10L)+48;
	    textbuf[30] = (char)((templong/100L)%10L)+48;
	    textbuf[31] = (char)((templong/10L)%10L)+48;
	    textbuf[32] = (char)(templong%10L)+48;
	    textbuf[33] = 0;
	    k = 28;
	    while ((textbuf[k] == 48) && (k < 32))
		textbuf[k++] = 32;
	    textprint(56,i-1+1,30);
	    textprint(55,i-1+1,32);
	}
	else
	{
	    textbuf[0] = j+49;
	    textbuf[1] = 0;
	    textprint(56,i-1+1,28);
	    textprint(55,i-1+1,30);
	}
	j++;
    }
    finalisemenu();
    if (whichmenu == 1)
    {
	do
	{
	    loadsavegameplace = getselection(16,67,loadsavegameplace,8);
	}
	while ((loadsavegameplace >= 0) && (gamexist[loadsavegameplace] == 0));
	j = loadsavegameplace;
	if ((loadsavegameplace < 0) || (gamexist[loadsavegameplace] == 0))
	    loadsavegameplace = (-loadsavegameplace)-1;
    }
    else
    {
	loadsavegameplace = getselection(16,67,loadsavegameplace,8);
	j = loadsavegameplace;
	if (loadsavegameplace < 0)
	    loadsavegameplace = (-loadsavegameplace)-1;
    }
    return(j);
}

/* Choose episode... */

K_INT16 newgamemenu()
{
    K_INT16 j, n;

    if (vidmode == 0)
	n = 0;
    else
	n = 20;
    drawmenu(288,64,menu);
    strcpy(&textbuf[0],"New game");
    textprint(137,74+n+1,112);
    strcpy(&textbuf[0],"Episode 1: Search for Sparky");
    textprint(67,88+n+1,32);
    if (numboards >= 20) j = 32; else j = 28;
    strcpy(&textbuf[0],"Episode 2: Sparky's Revenge");
    textprint(67,100+n+1,((char)j));
    if (numboards >= 30) j = 32; else j = 28;
    strcpy(&textbuf[0],"Episode 3: Find the Way Home");
    textprint(67,112+n+1,((char)j));
    if (newgameplace < 0) newgameplace = 0;
    if (newgameplace > 2) newgameplace = 2;
    finalisemenu();
    newgameplace = getselection(28,87,newgameplace,3);
    if ((newgameplace == 1) && (numboards < 20))
	return(newgameplace);
    if ((newgameplace == 2) && (numboards < 30))
	return(newgameplace);
    if (newgameplace<0) {
	newgameplace=(-newgameplace)-1;
	return -1;
    }
    drawmenu(288,64,menu);
    strcpy(&textbuf[0],"New game");
    textprint(137,74+n+1,112);
    strcpy(&textbuf[0],"Easy: Don't touch me.");
    textprint(67,92+n+1,32);
    strcpy(&textbuf[0],"Hard: OUCH!");
    textprint(67,104+n+1,32);
    finalisemenu();
    if (skilevel < 0) skilevel = 0;
    if (skilevel > 1) skilevel = 1;
    skilevel = getselection(28,91,skilevel,2);
    if (skilevel<0) {
	skilevel=(-skilevel)-1;
	return -1;
    }
    j = newgameplace;
    if (newgameplace < 0)
	newgameplace = (-newgameplace)-1;
    return(j);
}

/* Wait for keypress (or mouse click). */

void pressakey(void)
{
    K_INT16 bstatus, obstatus;

    bstatus = 1;
    obstatus = 1;
    keystatus[0x1c] = 0, keystatus[0x9c] = 0, keystatus[0x1] = 0, keystatus[0x39] = 0;
    newkeystatus[SDLK_RETURN]=newkeystatus[SDLK_KP_ENTER]=0;
    newkeystatus[SDLK_ESCAPE]=newkeystatus[SDLK_SPACE]=0;
    while (((keystatus[0x1c]|keystatus[0x9c]|keystatus[1]|keystatus[0x39]) == 0) && (bstatus <= obstatus))
    {
	PollInputs();
	obstatus = bstatus;
	if (moustat == 0) {
	    bstatus=readmouse(NULL,NULL);
	}
    }
    ksay(27);
}

/* Update slot machine... */

void copyslots(K_INT16 slotnum)
{
    K_INT16 i, j, k;
    unsigned char *l;

    l = walseg[slotinfo-1];
    for(i=0;i<3;i++)
	for(j=0;j<8;j++)
	{
	    k = ((slotpos[i]+j)&63)+(((slotpos[i]+j)&64)<<3)+(i<<10);
	    tempbuf[(i<<8)+(j<<3)]=l[k];
	    tempbuf[(i<<8)+(j<<3)+1]=l[k+64];
	    tempbuf[(i<<8)+(j<<3)+2]=l[k+128];
	    tempbuf[(i<<8)+(j<<3)+3]=l[k+192];
	    tempbuf[(i<<8)+(j<<3)+4]=l[k+256];
	    tempbuf[(i<<8)+(j<<3)+5]=l[k+320];
	    tempbuf[(i<<8)+(j<<3)+6]=l[k+384];
	    tempbuf[(i<<8)+(j<<3)+7]=l[k+448];
	}
    l = walseg[slotnum-1];
    for(i=0;i<3;i++)
	for(j=0;j<8;j++)
	{
	    k = 1296+(((i<<3)+i)<<6)+j;
	    l[k]=tempbuf[(i<<8)+(j<<3)];
	    l[k+64]=tempbuf[(i<<8)+(j<<3)+1];
	    l[k+128]=tempbuf[(i<<8)+(j<<3)+2];
	    l[k+192]=tempbuf[(i<<8)+(j<<3)+3];
	    l[k+256]=tempbuf[(i<<8)+(j<<3)+4];
	    l[k+320]=tempbuf[(i<<8)+(j<<3)+5];
	    l[k+384]=tempbuf[(i<<8)+(j<<3)+6];
	    l[k+448]=tempbuf[(i<<8)+(j<<3)+7];
	}
    gfxUpdateImage(slotnum);
}

/* Update map position... */

void youarehere()
{

    walseg[map-1][yourhereoldpos]=board[0][yourhereoldpos];

    yourhereoldpos = ((posx>>10)<<6)+(posy>>10);

    walseg[map-1][yourhereoldpos]=255;
}

/* Get keypress in Unicode form. */

Uint16 getkeypress() {
    SDL_Event event;
    int sk;

    while(SDL_PollEvent(&event))
    {
	switch(event.type)
	{	      
	    case SDL_QUIT:
		quitgame=1;
	    case SDL_KEYDOWN:
		sk=event.key.keysym.sym;
		if ((sk<SDLKEYS)&&(PCkey[sk]>=0)) {
		    keystatus[PCkey[sk]]=1;
		}
		if (sk<SDLKEYS)
		    newkeystatus[sk]=1;
		sk=event.key.keysym.unicode;
		return sk;
	    case SDL_KEYUP:
		sk=event.key.keysym.sym;
		if ((sk<SDLKEYS)&&(PCkey[sk]>=0)) {
		    keystatus[PCkey[sk]]=0;
		}
		if (sk<SDLKEYS)
		    newkeystatus[sk]=0;
		break;
	    default:
		break;
	}
    }
    if (quitgame) quit();
    return 0;
}

/* Poll for keyboard input or quit command. */

void PollInputs() {
    SDL_Event event;
    int sk;

    while(SDL_PollEvent(&event))
    {
	switch(event.type)
	{	      
	    case SDL_QUIT:
		quitgame=1;
	    case SDL_KEYDOWN:
		sk=event.key.keysym.sym;
		if ((sk<SDLKEYS)&&(PCkey[sk]>=0)) {
		    keystatus[PCkey[sk]]=1;
		}
		if (sk<SDLKEYS)
		    newkeystatus[sk]=1;
		break;		
	    case SDL_KEYUP:
		sk=event.key.keysym.sym;
		if ((sk<SDLKEYS)&&(PCkey[sk]>=0)) {
		    keystatus[PCkey[sk]]=0;
		}
		if (sk<SDLKEYS)
		    newkeystatus[sk]=0;
		break;
	    default:
		break;
	}
    }
    if (quitgame) quit();
}

/* Read mouse position (into x and y; NULL to ignore) and return buttons
   (1=left, 2=right, 4=middle). */

unsigned char readmouse(int *x, int *y) {
    int tx,ty;
    unsigned char bstatus=SDL_GetRelativeMouseState(&tx, &ty);

    /* Swap middle and right mouse buttons to match Microsoft style. */

    if (x!=NULL) *x+=5*tx;
    if (y!=NULL) *y+=5*ty;

    return ((bstatus&4)>>1)|(bstatus&1)|(bstatus&2)<<1;
}

/* Read joystick position (into x and y; NULL to ignore) and return buttons
   (1=button 0, 2=button 1). */

unsigned char readjoystick(int *x, int *y) {
    SDL_JoystickUpdate();

    if (x!=NULL) *x=SDL_JoystickGetAxis(joystick, 0);
    if (y!=NULL) *y=SDL_JoystickGetAxis(joystick, 1);

    return (SDL_JoystickGetButton(joystick, 0))|
	(SDL_JoystickGetButton(joystick, 1)<<1);
}

void quit() {

    savesettings();

    /* Start by demolishing all other threads... */

    SDL_UnlockMutex(timermutex); /* Just in case we have it... */
    SDL_UnlockMutex(soundmutex); /* Just in case we have it... */

    musicoff();

    if (speechstatus >= 2) {
	/* SDL is very careful to allow the sound thread to stop. Good for
	   us. */

	SDL_CloseAudio();
	free(SoundBuffer);
    }    

    free(screenbuffer);

    if (quitgame == 2)
	fprintf(stderr,"Error #3:  Invalid saved game.");
    if (numboards < 30)
    {

	/* Print shareware message:
	   Dump 16K of segment walseg[endtext-1] to text mode screen,
	   move cursor to start of line 0x16 (0 is top). 

	   Removed to avoid confusing people (and to save me the trouble
	   of parsing a CGA colour text screen). */

    }
    free((void *)note);
    free(lzwbuf);
    free(lzwbuf2);
    if (convwalls > 0) free(pic);
    if (joystat==0)
	SDL_JoystickClose(joystick);

    SDL_Quit();
    
    exit(0);
}

#define log2(a) (log(a)/log(2))

void randoinsts()
{
    long i, j, k;
    float f;

    if (channels == 2)
    {
	j = (rand()&2)-1; k = 0;
	for(i=0;i<9;i++)
	{
	    if ((i == 0) || (chantrack[i] != chantrack[i-1]))
	    {
		f = (float)rand()/(float)RAND_MAX;
		if (j > 0)
		{
		    //lvol[i] < rvol[i]
		    lvol[i] = log2(f+1);
		    rvol[i] = log2(3-f);
		    lplc[i] = rand()&255;
		    rplc[i] = 0;
		}
		else
		{
		    //lvol[i] > rvol[i]
		    lvol[i] = log2(3-f);
		    rvol[i] = log2(f+1);
		    lplc[i] = 0;
		    rplc[i] = rand()&255;
		}
		j = -j;
		if (((drumstat&32) == 0) || (i < 6)) k++;
	    }
	    else
	    {
		lvol[i] = lvol[i-1]; rvol[i] = rvol[i-1];
		lplc[i] = lplc[i-1]; rplc[i] = rplc[i-1];
	    }
	}
	if (k < 2)  //If only 1 source, force it to be in center
	{
	    if (drumstat&32) i = 5; else i = 8;
	    for(;i>=0;i--)
	    {
		lvol[i] = rvol[i] = 1;
		lplc[i] = rplc[i] = 0;
	    }
	}
    }
}

void drawinputbox() {
    int k;
    
    menuing=1; menuleft=84; menutop=127;
    menuwidth=192; menuheight=64;
    statusbardraw(0, 0, 57, 64, 84-20, 127, lab3dversion?76:scorebox);
    statusbardraw(7, 0, 39, 64, 141-20, 127, lab3dversion?76:scorebox);
    statusbardraw(7, 0, 39, 64, 180-20, 127, lab3dversion?76:scorebox);
    statusbardraw(7, 0, 57, 64, 219-20, 127, lab3dversion?76:scorebox);
    for(k=0;k<22;k++)
	textbuf[k] = 8;
    textbuf[22] = 0;
    textprint(91,145+1,(char)0);
    textbuf[1] = 0;
    textprint(261,145+1,(char)0);
}

void updatebullets(K_UINT16 posxs, K_UINT16 posys, K_INT16 poszs) {
    static int spareframes=0;
    K_INT16 i, j, k;
    K_INT32 x1, y1, x2, y2;

    /* Update bullet rotations... Somewhat misplaced IMHO. */

    if (lab3dversion) {
	spareframes+=clockspd;
	i=(spareframes/TICKS_PER_SPRITE_FRAME)%12;
	spareframes%=TICKS_PER_SPRITE_FRAME;
	for(k=0;k<i;k++) {
	    heatpos = 287-heatpos;
	    j = kenpos;
	    if (j == 66) kenpos = 67;
	    if (j == 67) kenpos = 1024+66;
	    if (j == 1024+66) kenpos = 65;
	    if (j == 65) kenpos = 66;
	    switch(kenpos2) {
		/* Sequence is 193, 65, 66, 67, 194, 67+1024, 66+1024, 65+1024.
		   Sequence appears to match values used in v1.1; determined
		   through observation of v1.1 in slow motion. */
		case 193:
		    kenpos2=65; break;
		case 65:
		    kenpos2=66; break;
		case 66:
		    kenpos2=67; break;
		case 67:
		    kenpos2=194; break;
		case 194:
		    kenpos2=67+1024; break;
		case 67+1024:
		    kenpos2=66+1024; break;
		case 66+1024:
		    kenpos2=65+1024; break;
		case 65+1024:
		    kenpos2=193; break;
	    }
	    ballpos++;
	    if (ballpos == 72)
		ballpos = 68;
	    fanpos++;
	    if (fanpos == 52)
		fanpos = 49;
	    warpos++;
	    if (warpos == 125)
		warpos = 123;
	}
	for(k=0;k<bulnum;k++)
	    if (bulkind[k] == 7)
	    {
		x1 = ((long)bulx[k]-(long)posx);
		y1 = ((long)buly[k]-(long)posy);
		if (labs(x1)+labs(y1) < 32768)
		{
		    x1 >>= 2;
		    y1 >>= 2;
		    x2 = (((x1*sintable[bulang[k]])-(y1*sintable[(bulang[k]+512)&2047]))>>16);
		    y2 = (((x1*sintable[(bulang[k]+512)&2047])+(y1*sintable[bulang[k]]))>>16);
		    if ((x2|y2) != 0)
		    {
			j = ((x2*clockspd)<<11)/(x2*x2+y2*y2);
			bulang[k] += j;
		    }
		}
	    }
    } else {
	for(k=0;k<bulnum;k++)
	    if (bulkind[k] == 7)
	    {
		x1 = ((long)bulx[k]-(long)posxs);
		y1 = ((long)buly[k]-(long)posys);
		if (labs(x1)+labs(y1) < 32768)
		{
		    x1 >>= 2;
		    y1 >>= 2;
		    x2 = (((x1*sintable[bulang[k]])-(y1*sintable[(bulang[k]+512)&2047]))>>16);
		    y2 = (((x1*sintable[(bulang[k]+512)&2047])+(y1*sintable[bulang[k]]))>>16);
		    if ((x2|y2) != 0)
		    {
			j = ((x2*clockspd)<<11)/(x2*x2+y2*y2);
			bulang[k] += j;
		    }
		}
	    }
    }
}

void checkforvisiblestuff(K_UINT16 posxs, K_UINT16 posys, K_INT16 poszs, K_INT16 angs) {
    K_INT16 i, j, k, x, y, temp;
    K_INT32 x1, y1, x2, y2;
    K_INT16 xc, yc;

    /* Check for visible monsters... */

    if (lab3dversion) {
	for(i=0;i<mnum;i++)
	{
	    xc = (mposx[i]>>10);
	    yc = (mposy[i]>>10);
	    if (tempbuf[(xc<<6)+yc] != 0)
	    {
		temp = mboard[xc][yc];
		if ((temp != 68) && (temp != 98))
		    for(k=0;k<bulnum;k++)
			if ((bulkind[k] == 3) || (bulkind[k] == 4))
			{
			    x1 = ((long)bulx[k]-(long)mposx[i]);
			    y1 = ((long)buly[k]-(long)mposy[i]);
			    if (labs(x1)+labs(y1) < 32768)
			    {
				x1 >>= 2;
				y1 >>= 2;
				x2 = (((x1*sintable[bulang[k]])-(y1*sintable[(bulang[k]+512)&2047]))>>16);
				y2 = (((x1*sintable[(bulang[k]+512)&2047])+(y1*sintable[bulang[k]]))>>16);
				if ((x2|y2) != 0)
				{
				    j = ((x2*clockspd)<<11)/(x2*x2+y2*y2);
				    if (temp == 94)
					j >>= 1;
				    if (temp == 109)
					j = -j;
				    bulang[k] += j;
				}
			    }
			}
		if (temp == 66)
		    checkobj(mposx[i],mposy[i],posx,posy,ang,
			     (lab3dversion==2)?kenpos:kenpos2);
		if (temp == 68)
		    checkobj(mposx[i],mposy[i],posx,posy,ang,ballpos);
		if (temp == 38)
		{
		    if (mshock[i] > 0)
			checkobj(mposx[i],mposy[i],posx,posy,ang,61);
		    else
		    {
			if ((kenpos&1023) == 66)
			    checkobj(mposx[i],mposy[i],posx,posy,ang,140);
			else if (kenpos == 65)
			    checkobj(mposx[i],mposy[i],posx,posy,ang,38);
			else if (kenpos == 67)
			    checkobj(mposx[i],mposy[i],posx,posy,ang,141);
		    }
		}
		if (temp == 54)
		{
		    if (mshock[i] > 0)
			checkobj(mposx[i],mposy[i],posx,posy,ang,
				 (lab3dversion==1)?195+((kenpos&1023)==66):56);
		    else
		    {
			j = 54;
			if ((posx > mposx[i]) == (posy > mposy[i]))
			{
			    if ((mgolx[i] < moldx[i]) || (mgoly[i] > moldy[i]))
				j = 53;
			    if ((mgolx[i] > moldx[i]) || (mgoly[i] < moldy[i]))
				j = 55;
			    if ((posx < mposx[i]) && (posy < mposy[i]))
				j = 108 - j;
			}
			if ((posx > mposx[i]) != (posy > mposy[i]))
			{
			    if ((mgolx[i] < moldx[i]) || (mgoly[i] < moldy[i]))
				j = 53;
			    if ((mgolx[i] > moldx[i]) || (mgoly[i] > moldy[i]))
				j = 55;
			    if ((posx > mposx[i]) && (posy < mposy[i]))
				j = 108 - j;
			}
			checkobj(mposx[i],mposy[i],posx,posy,ang,j);
		    }
		}
		if (temp == 94)
		{
		    if (mshock[i] > 0)
			checkobj(mposx[i],mposy[i],posx,posy,ang,95);
		    else
			checkobj(mposx[i],mposy[i],posx,posy,ang,94);
		}
		if (temp == 98)
		{
		    if (mshock[i] > 0)
			checkobj(mposx[i],mposy[i],posx,posy,ang,98);
		    else
			checkobj(mposx[i],mposy[i],posx,posy,ang,kenpos+34);
		}
		if (temp == 109)
		{
		    if (mshock[i] > 0)
			checkobj(mposx[i],mposy[i],posx,posy,ang,110);
		    else
			checkobj(mposx[i],mposy[i],posx,posy,ang,109);
		}
		if (temp == 160)
		    checkobj(mposx[i],mposy[i],posx,posy,ang,kenpos+160-66);
		if (temp == 165)
		    checkobj(mposx[i],mposy[i],posx,posy,ang,165);
		if (temp == 187)
		    checkobj(mposx[i],mposy[i],posx,posy,ang,kenpos+187-66);
	    }
	}

    } else {
	for(i=0;i<mnum;i++)
	{

	    j=((mposx[i]>>10)<<6)+(mposy[i]>>10);

	    if (tempbuf[j] != 0)
	    {
		temp = mstat[i];
		if ((temp != monbal) && (temp != monhol) && (temp != monke2) && (temp != mondog))
		    for(k=0;k<bulnum;k++)
			if ((bulkind[k] == 3) || (bulkind[k] == 4))
			{
			    x1 = ((long)bulx[k]-(long)mposx[i]);
			    y1 = ((long)buly[k]-(long)mposy[i]);
			    if (labs(x1)+labs(y1) < 32768)
			    {
				x1 >>= 2;
				y1 >>= 2;
				x2 = (((x1*sintable[bulang[k]])-(y1*sintable[(bulang[k]+512)&2047]))>>16);
				y2 = (((x1*sintable[(bulang[k]+512)&2047])+(y1*sintable[bulang[k]]))>>16);
				if ((x2|y2) != 0)
				{
				    j = ((x2*clockspd)<<11)/(x2*x2+y2*y2);
				    if (temp == monali)
					j >>= 1;
				    if ((temp == monzor) || (temp == monan2))
					j = -j;
				    if (temp == monan3)
				    {
					if (mshock[i] > 0)
					    j = 0;
					else
					    j = -j;
				    }
				    bulang[k] += j;
				}
			    }
			}
		switch(temp)
		{
		    case monken:
			checkobj(mposx[i],mposy[i],posxs,posys,angs,monken+oscillate5);
			break;
		    case monbal:
			checkobj(mposx[i],mposy[i],posxs,posys,angs,monbal+animate4);
			break;
		    case mongho:
			checkobj(mposx[i],mposy[i],posxs,posys,angs,mongho+oscillate3);
			break;
		    case hive:
			if (((mshock[i]&16384) == 0) && (mstat[i] > 0))
			    checkobj(mposx[i],mposy[i],posxs,posys,angs,hive);
			else
			{
			    checkobj(mposx[i],mposy[i],posxs,posys,angs,hivetohoney+((mshock[i]-16384)>>5));
			    mshock[i] += clockspd;
			    if (((mshock[i]-16384)>>5) >= 5)
			    {
				board[mposx[i]>>10][mposy[i]>>10] = honey+1024;
				mnum--;
				moldx[i] = moldx[mnum];
				moldy[i] = moldy[mnum];
				mposx[i] = mposx[mnum];
				mposy[i] = mposy[mnum];
				mgolx[i] = mgolx[mnum];
				mgoly[i] = mgoly[mnum];
				mstat[i] = mstat[mnum];
				mshot[i] = mshot[mnum];
				mshock[i] = mshock[mnum];
			    }
			}
			break;
		    case monske:
			checkobj(mposx[i],mposy[i],posxs,posys,angs,monske+oscillate3);
			break;
		    case monmum:
			checkobj(mposx[i],mposy[i],posxs,posys,angs,monmum+animate8);
			break;
		    case mongre:
			if (mshock[i] > 0)
			    checkobj(mposx[i],mposy[i],posxs,posys,angs,mongre+5);
			else
			    checkobj(mposx[i],mposy[i],posxs,posys,angs,mongre+oscillate5);
			break;
		    case monrob:
			checkobj(mposx[i],mposy[i],posxs,posys,angs,monrob+animate10);
			break;
		    case monro2:
			checkobj(mposx[i],mposy[i],posxs,posys,angs,monro2+animate8);
			break;
		    case mondog:
			if (boardnum >= 10)
			    checkobj(mposx[i],mposy[i],posxs,posys,angs,mondog+animate15+15);
			else
			    checkobj(mposx[i],mposy[i],posxs,posys,angs,mondog+animate15);
			break;
		    case monwit:
			if (mshock[i] > 0)
			    checkobj(mposx[i],mposy[i],posxs,posys,angs,monwit);
			else
			    checkobj(mposx[i],mposy[i],posxs,posys,angs,monwit+oscillate5);
			break;
		    case monand:
			if (mshock[i] > 0)
			    checkobj(mposx[i],mposy[i],posxs,posys,angs,monand+3+animate2);
			else
			{
			    j = monand;
			    if ((posxs > mposx[i]) == (posys > mposy[i]))
			    {
				if ((mgolx[i] < moldx[i]) || (mgoly[i] > moldy[i]))
				    j = monand+1;
				if ((mgolx[i] > moldx[i]) || (mgoly[i] < moldy[i]))
				    j = monand+2;
				if ((posxs < mposx[i]) && (posys < mposy[i]) && (j != monand))
				    j = monand+monand+3-j;
			    }
			    if ((posxs > mposx[i]) != (posys > mposy[i]))
			    {
				if ((mgolx[i] < moldx[i]) || (mgoly[i] < moldy[i]))
				    j = monand+1;
				if ((mgolx[i] > moldx[i]) || (mgoly[i] > moldy[i]))
				    j = monand+2;
				if ((posxs > mposx[i]) && (posys < mposy[i]) && (j != monand))
				    j = monand+monand+3-j;
			    }
			    checkobj(mposx[i],mposy[i],posxs,posys,angs,j);
			}
			break;
		    case monali:
			if (mshock[i] > 0)
			    checkobj(mposx[i],mposy[i],posxs,posys,angs,monali+1);
			else
			    checkobj(mposx[i],mposy[i],posxs,posys,angs,monali);
			break;
		    case monhol:
			if (mshock[i] > 0)
			    checkobj(mposx[i],mposy[i],posxs,posys,angs,monhol);
			else
			    checkobj(mposx[i],mposy[i],posxs,posys,angs,monhol+oscillate3+1);
			break;
		    case monzor:
			if ((mshock[i]&8192) > 0)
			{
			    if (mshock[i] < 8192+monzor+11)
				mshock[i] = 0;
			    else if (mshock[i] <= 8192+monzor+12)
				checkobj(mposx[i],mposy[i],posxs,posys,angs,mshock[i]&1023);
			    else
				checkobj(mposx[i],mposy[i],posxs,posys,angs,monzor+13+animate2);
			}
			if (mshock[i] < 8192)
			{
			    if (mshock[i] > 0)
				checkobj(mposx[i],mposy[i],posxs,posys,angs,monzor+10);
			    else
				checkobj(mposx[i],mposy[i],posxs,posys,angs,monzor+animate10);
			}
			if ((mshock[i]&16384) > 0)
			{
			    j = mshock[i]&1023;
			    if (mshock[i] <= 16384+monzor+12)
				checkobj(mposx[i],mposy[i],posxs,posys,angs,j);
			    else
			    {
				mshock[i] = monzor+13+8192;
				if (clockspd > 0)
				{
				    mshock[i] += 512/clockspd;
				    if (mshock[i] > 16383)
					mshock[i] = 16383;
				}
			    }
			}
			break;
		    case monbat:
			checkobj(mposx[i],mposy[i],posxs,posys,angs,monbat+oscillate3);
			break;
		    case monear:
			checkobj(mposx[i],mposy[i],posxs,posys,angs,monear+oscillate5);
			break;
		    case monbee:
			checkobj(mposx[i]+(rand()&127)-64,mposy[i]+(rand()&127)-64,posxs,posys,angs,monbee+animate6);
			break;
		    case monspi:
			checkobj(mposx[i],mposy[i],posxs,posys,angs,monspi+animate6);
			break;
		    case mongr2:
			checkobj(mposx[i],mposy[i],posxs,posys,angs,mongr2+animate11);
			break;
		    case monke2:
			if ((mshock[i]&8192) > 0)
			{
			    j = (mshock[i]&1023);
			    if (mshock[i] < 8192+monke2+7)
				mshock[i] = 0;
			    else if (mshock[i] <= 8192+monke2+13)
				checkobj(mposx[i],mposy[i],posxs,posys,angs,mshock[i]&1023);
			}
			if (mshock[i] < 8192)
			{
			    if (mshock[i] > 0)
			    {
				if (mshot[i] < 32)
				    checkobj(mposx[i],mposy[i],posxs,posys,angs,monke2+oscillate3+3);
				else
				    checkobj(mposx[i],mposy[i],posxs,posys,angs,monke2+6);
			    }
			    else
				checkobj(mposx[i],mposy[i],posxs,posys,angs,monke2+oscillate3);
			}
			if ((mshock[i]&16384) > 0)
			{
			    j = mshock[i]&1023;
			    if (mshock[i] <= 16384+monke2+13)
				checkobj(mposx[i],mposy[i],posxs,posys,angs,j);
			    else
			    {
				mshock[i] = monke2+13+8192;
				x = (mgolx[i]>>10);
				y = (mgoly[i]>>10);
				board[moldx[i]>>10][moldy[i]>>10] &= 0xbfff;
				board[x][y] &= 0xbfff;
				for(k=0;k<16;k++)
				{
				    j = (rand()&3);
				    if ((j == 0) && ((board[x-1][y]&0x4c00) == 1024))
					x--;
				    if ((j == 1) && ((board[x+1][y]&0x4c00) == 1024))
					x++;
				    if ((j == 2) && ((board[x][y-1]&0x4c00) == 1024))
					y--;
				    if ((j == 3) && ((board[x][y+1]&0x4c00) == 1024))
					y++;
				}
				board[x][y] |= 0x4000;
				mposx[i] = (((unsigned)x)<<10)+512;
				mposy[i] = (((unsigned)y)<<10)+512;
				mgolx[i] = mposx[i];
				mgoly[i] = mposy[i];
			    }
			}
			break;
		    case monan2:
			if (mshock[i] > 0)
			{
			    if (mshot[i] > 32)
				checkobj(mposx[i],mposy[i],posxs,posys,angs,monan2+3);
			    else
				checkobj(mposx[i],mposy[i],posxs,posys,angs,monan2+4+animate2);
			}
			else
			{
			    j = monan2;
			    if ((posxs > mposx[i]) == (posys > mposy[i]))
			    {
				if ((mgolx[i] < moldx[i]) || (mgoly[i] > moldy[i]))
				    j = monan2+1;
				if ((mgolx[i] > moldx[i]) || (mgoly[i] < moldy[i]))
				    j = monan2+2;
				if ((posxs < mposx[i]) && (posys < mposy[i]) && (j != monan2))
				    j = monan2+monan2+3-j;
			    }
			    if ((posxs > mposx[i]) != (posys > mposy[i]))
			    {
				if ((mgolx[i] < moldx[i]) || (mgoly[i] < moldy[i]))
				    j = monan2+1;
				if ((mgolx[i] > moldx[i]) || (mgoly[i] > moldy[i]))
				    j = monan2+2;
				if ((posxs > mposx[i]) && (posys < mposy[i]) && (j != monan2))
				    j = monan2+monan2+3-j;
			    }
			    checkobj(mposx[i],mposy[i],posxs,posys,angs,j);
			}
			break;
		    case monan3:
			if ((mshock[i]&8192) > 0)
			{
			    if (mshock[i] < 8192+monan3+3)
				mshock[i] = 0;
			    else if (mshock[i] <= 8192+monan3+8)
				checkobj(mposx[i],mposy[i],posxs,posys,angs,mshock[i]&1023);
			}
			if (mshock[i] == 0)
			{
			    j = monan3;
			    if ((posxs > mposx[i]) == (posys > mposy[i]))
			    {
				if ((mgolx[i] < moldx[i]) || (mgoly[i] > moldy[i]))
				    j = monan3+1;
				if ((mgolx[i] > moldx[i]) || (mgoly[i] < moldy[i]))
				    j = monan3+2;
				if ((posxs < mposx[i]) && (posys < mposy[i]) && (j != monan3))
				    j = monan3+monan3+3-j;
			    }
			    if ((posxs > mposx[i]) != (posys > mposy[i]))
			    {
				if ((mgolx[i] < moldx[i]) || (mgoly[i] < moldy[i]))
				    j = monan3+1;
				if ((mgolx[i] > moldx[i]) || (mgoly[i] > moldy[i]))
				    j = monan3+2;
				if ((posxs > mposx[i]) && (posys < mposy[i]) && (j != monan3))
				    j = monan3+monan3+3-j;
			    }
			    checkobj(mposx[i],mposy[i],posxs,posys,angs,j);
			}
			if ((mshock[i]&16384) > 0)
			{
			    j = mshock[i]&1023;
			    if (mshock[i] <= 16384+monan3+8)
				checkobj(mposx[i],mposy[i],posxs,posys,angs,j);
			    else
			    {
				mshock[i] = monan3+8+8192;
				if (clockspd > 0)
				{
				    mshock[i] += 240/clockspd;
				    if (mshock[i] > 16383)
					mshock[i] = 16383;
				}
			    }
			}
			break;
		}
	    }
	}
    }

    if (lab3dversion) {
	for(xc=0;xc<64;xc++)
	    for(yc=0;yc<64;yc++)
		if (tempbuf[(xc<<6)+yc] != 0)
		{
		    tempbuf[(xc<<6)+yc] = 0;
		    i = (board[xc][yc]&1023);
		    if (bmpkind[i] >= 2)
		    {
			if (i == 49)
			    i = fanpos;
			if (i == 123)
			    i = warpos;
			if (i == 163)
			    if (fanpos == 49)
				i = 77;
			j = sortcnt;
			checkobj((xc<<10)+512,(yc<<10)+512,posxs,posys,angs,i);
			if (bmpkind[i] == 4)
			{
			    sortcnt = j+1;
			}
/*          if (bmpkind[i] == 4)
	    {
            sortcnt = j+1;
            sortang[sortcnt-1] = (xc<<6)+yc;
	    }*/
		    }
		}
    } else {
	/* Check for visible transparent walls... */

	for(k=0;k<4096;k++)
	    if (tempbuf[k] != 0)
	    {
		tempbuf[k]=0;
		i=board[0][k]&1023;
		if (bmpkind[i] >= 2)
		{
		    if ((i == exitsign) || (i == soda) || (i == tentacles) || (i == tablecandle))
			i += animate2;
		    if (i == minicolumn)
			i += animate4;
		    j = sortcnt;
		    checkobj(((k&0xfc0)<<4)+512,((k&63)<<10)+512,posxs,posys,angs,i);
		    if (bmpkind[i] == 4)
		    {
			sortcnt = j+1;

			if (i == door3+1)
			{
			    if (sorti[sortcnt-1] > 512)
				sortbnum[sortcnt-1] = door3;
			    else if (sorti[sortcnt-1] > 470)
				sortbnum[sortcnt-1] = door3+1;
			    else if (sorti[sortcnt-1] > 431)
				sortbnum[sortcnt-1] = door3+2;
			    else if (sorti[sortcnt-1] > 395)
				sortbnum[sortcnt-1] = door3+3;
			    else if (sorti[sortcnt-1] > 362)
				sortbnum[sortcnt-1] = door3+4;
			    else if (sorti[sortcnt-1] > 332)
				sortbnum[sortcnt-1] = door3+5;
			    else if (sorti[sortcnt-1] > 279)
				sortbnum[sortcnt-1] = door3+6;
			    else if (sorti[sortcnt-1] > 256)
				sortbnum[sortcnt-1] = door3+7;
			    else
				sortcnt--;
			}
		    }
		}
	    }
    }
}

double angcan(double angle) {
    while(angle<0) angle+=M_PI*2;
    while(angle>=M_PI*2) angle-=M_PI*2;
    return angle;
}
