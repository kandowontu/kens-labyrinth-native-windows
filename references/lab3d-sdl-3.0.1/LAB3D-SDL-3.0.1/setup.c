#include "lab3d.h"
#ifdef USE_OSS
#define MUSIC_SOURCES 3
#else
#ifdef WIN32
#define MUSIC_SOURCES 3
//#include "objbase.h"
//#include "shlobj.h"
#else
#define MUSIC_SOURCES 2
#endif
#endif

#define largescreentexture 0

#ifdef NEVER
HRESULT CreateLink(LPCSTR lpszPathObj, 
		   LPSTR lpszPathLink, LPSTR lpszDesc,
		   LPSTR lpszArgs) { 
    HRESULT hres; 
    IShellLink* psl; 
    char p[MAX_PATH];
 
    CoInitialize(NULL);
    hres = CoCreateInstance(&CLSID_ShellLink, NULL, 
			    CLSCTX_INPROC_SERVER, &IID_IShellLink,
			    (void *)&psl); 
    if (SUCCEEDED(hres)) { 
        IPersistFile* ppf; 
	
	GetCurrentDirectory(MAX_PATH, p);
        psl->lpVtbl->SetWorkingDirectory(psl, p); 
        hres=psl->lpVtbl->SetPath(psl, lpszPathObj); 

        psl->lpVtbl->SetArguments(psl, lpszArgs); 

        psl->lpVtbl->SetDescription(psl, lpszDesc); 
 
        hres = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, 
					   (void *)&ppf); 
 
        if (SUCCEEDED(hres)) { 
            WORD wsz[MAX_PATH]; 
 
	    fprintf(stderr,"Trying to save shortcut...\n");
            MultiByteToWideChar(CP_ACP, 0, lpszPathLink, -1, 
				wsz, MAX_PATH); 

            hres = ppf->lpVtbl->Save(ppf, wsz, TRUE); 
            ppf->lpVtbl->Release(ppf);
	    if (SUCCEEDED(hres))
		fprintf(stderr,"Done.\n");
        } 
        psl->lpVtbl->Release(psl); 
    } 
    CoUninitialize();
    return hres; 
} 

void createshortcut(void) {
    ITEMIDLIST *l;
    char p[MAX_PATH];
    char p2[MAX_PATH];
    char p3[MAX_PATH];
    int i;

    fprintf(stderr,"Getting desktop location.\n");
    if (SHGetSpecialFolderLocation(GetDesktopWindow(),
				   CSIDL_DESKTOPDIRECTORY,&l)!=NOERROR)
	return;
    fprintf(stderr,"Converting...\n");
    if (SHGetPathFromIDList(l,p)!=TRUE) return;
    fprintf(stderr,"Desktop location is %s.\n",p);

    i=strlen(p);
    if (i>MAX_PATH-20) return;
    if (p[i-1]=='\\') p[i-1]=0;
    strcpy(p3,p);
    strcat(p3,"\\Ken's Labyrinth.lnk");

    fprintf(stderr,"Creating link as %s.\n",p3);

    GetCurrentDirectory(MAX_PATH,p2);

    i=strlen(p2);
    if (i>MAX_PATH-10) return;
    if (p2[i-1]=='\\') p2[i-1]=0;
    strcat(p2,"\\ken.exe");

    CreateLink(p2, p3,"Ken's Labyrinth","");

    strcpy(p3,p);
    strcat(p3,"\\Ken's Labyrinth Setup.lnk");

    CreateLink(p2, p3,"Ken's Labyrinth Setup","-setup");
}
#endif

static int inputdevice=1,resolutionnumber=3,nearest=0;
static int music=1,sound=1,fullscr=1,cheat=0,channel=1,musicchannel=1;
static int soundblock=0,timing=0,texturedepth=1,scaling=2,renderer=0;

static char keynames[numkeys][30]={
    "Move FORWARD",
    "Move BACKWARD",
    "Turn LEFT",
    "Turn RIGHT",
    "STRAFE (walk sideways)",
    "STAND HIGH",
    "STAND LOW",
    "RUN",
    "FIRE",
    "FIREBALLS (red)",
    "BOUNCY-BULLETS (green)",
    "HEAT-SEEKING MISSILES",
    "UNLOCK / OPEN / CLOSE / USE",
    "CHEAT for more life",
    "RAISE / LOWER STATUS BAR",
    "PAUSE GAME",
    "MUTE KEY",
    "SHOW MENU"
};

static int newdefaultkey[numkeys]={
    SDLK_UP,
    SDLK_DOWN,
    SDLK_LEFT,
    SDLK_RIGHT,
    SDLK_RCTRL,
    SDLK_a,
    SDLK_z,
    SDLK_LSHIFT,
    SDLK_LCTRL,
    SDLK_F1,
    SDLK_F2,
    SDLK_F3,
    SDLK_SPACE,
    SDLK_BACKSPACE,
    SDLK_RETURN,
    SDLK_p,
    SDLK_m,
    SDLK_ESCAPE
};

static char inputdevicemenu[4][30]={
    "Keyboard only",
    "Keyboard + mouse",
    "Keyboard + joystick",
    "Keyboard + mouse + joystick"
};

/* Predefined resolutions for backward compatibility with early betas. */

static int resx[19]={360,512,640,800,1024,1152,1280,1600,
		     320,320,400,480,640,720,720,960,1920,1920,1280};
static int resy[19]={240,384,480,600,768,864,1024,1200,
		     200,240,300,360,400,480,576,720,1080,1200,960};

static char resolutiontypemenu[3][30]={
    "Fullscreen resolutions",
    /*    "Standard resolutions",
	  "Non-standard resolutions",*/
    "Custom resolution",
    "Return to setup menu"
};

static char resolutionstandardmenu[8][30]={
    "360x240",
    "512x384",
    "640x480",
    "800x600",
    "1024x768",
    "1152x864",
    "1280x1024",
    "1600x1200"
};

static char resolutionspecialmenu[11][30]={
    "320x200",
    "320x240",
    "400x300",
    "480x360",
    "640x400",
    "720x480",
    "720x576",
    "960x720",
    "1920x1080",
    "1920x1200",
    "1280x960"
};

static char fullscreenmenu[2][30]={
    "Windowed",
    "Fullscreen"
};

static char filtermenu[2][30]={
    "Trilinear filtering",
    "No filtering"
};

static char renderermenu[2][30]={
    "OpenGL",
    "Software"
};

static char musicmenu[3][30]={
    "No music",
    "Adlib emulation",
    "General MIDI",
};

static char soundmenu[2][30]={
    "No sound",
    "Digital sound effects"
};

static char channelmenu[2][30]={
    "Mono",
    "Stereo"
};

static char cheatmenu[3][30]={
    "No cheats",
    "LSHIFT-RSHIFT",
    "LSHIFT-LCTRL"
};

static char soundblockmenu[10][30]={
    "Default (11.6 ms)",
    "1.5 ms",
    "2.9 ms",
    "5.8 ms",
    "11.6 ms",
    "23.2 ms",
    "46.4 ms",
    "92.9 ms",
    "185.8 ms",
    "371.5 ms"
};

static char timingmenu[2][30]={
    "System timer",
    "Sound output"
};

static char texturedepthmenu[3][30]={
    "Driver default",
    "32 bit",
    "16 bit"
};

static char scalingtypemenu[4][30]={
    "Fill screen (4:3 view)",
    "Integer scale (4:3 view)",
    "Fill screen (square pixels)",
    "Integer scale (square pixels)"
};

void selectionmenu(int alts,char titles[][30],int *value) {
    int i;
    int j=12*alts+24;

    drawmenu(304,j,menu);
    
    for(i=0;i<alts;i++) {
	strcpy(textbuf,titles[i]);
	textprint(71,120-6*alts+12*i,lab3dversion?32:34);
    }

    finalisemenu();

    if (j>240)
	i=getselection(28,97-6*alts,*value,alts);
    else
	i=getselection(28,99-6*alts,*value,alts);

    if (i>=0) *value=i;   
}

int resolutionmenu(int alts,int start,char titles[][30],int def) {
    int i;
    int j=12*alts+24;
    char t[12];

    if (start<0) {
	for(i=0;i<alts;i++) {
	    if (def>=10000)
		sprintf(t,"%dx%d",def/10000,def%10000);
	    else
		sprintf(t,"%dx%d",resx[def],resy[def]);
	    if (!strcmp(t,titles[i]))
		break;
	}
	if (i<alts)
	    def=i;
	else
	    def=0;
    } else {
	def-=start;
	if (def<0) def=0;
	if (def>=alts) def=0;
    }

    drawmenu(304,j,menu);
    
    for(i=0;i<alts;i++) {
	strcpy(textbuf,titles[i]);
	textprint(71,120-6*alts+12*i,lab3dversion?32:34);
    }

    finalisemenu();

    i=getselection(28,99-6*alts,def,alts);

    if (i>=0) return i; else return -1;
}

int getnumber(void) {
    unsigned char ch;
    char buf[10];
    K_INT16 i,j;

    for(j=0;j<12;j++)
	textbuf[j] = 8;
    textbuf[12] = 0;
    textprint(94,145+1,(char)0);
    j = 0;
    buf[0]=0;
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
	    buf[j] = ch;
	    for(j=0;j<10;j++)
		buf[j] = 0;
	    for(j=0;j<12;j++)
		textbuf[j] = 8;
	    textbuf[12] = 0;
	    textprint(94,145+1,(char)0);
	    j = 0;
	    ch = 0;
	}
	if ((ch == 8) && (j > 0))
	{
	    j--, buf[j] = 0;
	    textbuf[0] = ch;
	    textbuf[1] = 0;
	    textprint(94+(j<<3),145+1,(char)0);
	}
	if ((ch >= 48) && (ch <= 57) && (j < 4))
	{
	    textbuf[0] = ch;
	    textbuf[1] = 0;
	    textprint(94+(j<<3),145+1,(char)97);
	    buf[j] = ch;
	    if ((ch != 32) || (j > 0))
		j++;
	}
    }
    SDL_EnableUNICODE(0);
    for(i=0;i<256;i++)
	keystatus[i] = 0;
    if (ch==27) return -1;
    return strtol(buf,NULL,10);
}

void customresolution(void) {
    int x,y;

    drawinputbox();
    finalisemenu();    
    sprintf(&textbuf[0],"Enter screen width:");
    textprint(180-(strlen(textbuf)<<2),135+1,(char)161);
    x=getnumber();
    if (x>0) {
	drawinputbox();
	finalisemenu();
	sprintf(&textbuf[0],"Enter screen height:");
	textprint(180-(strlen(textbuf)<<2),135+1,(char)161);
	y=getnumber();
	if (y>0)
	    resolutionnumber=x*10000+y;
    }
}

void setupinputdevices(void) {
    selectionmenu(4,inputdevicemenu,&inputdevice);
}

int modecompare(const void *a, const void *b) {
    SDL_Rect *c=*(SDL_Rect **)a;
    SDL_Rect *d=*(SDL_Rect **)b;

    return ((K_INT32)d->w*((K_INT32)d->h)-((K_INT32)c->w)*((K_INT32)c->h));
}

void setupsetresolution(void) {
    int a,i,m;
    int resolutionmenusize;
    int detectedresolution[11];
    char resolutiondetectmenu[11][30];
    SDL_Rect **modes,**umodes;
    a=resolutionmenu(3,0,resolutiontypemenu,0);

    switch(a) {
        case 0:
	    modes=umodes=SDL_ListModes(NULL,SDL_FULLSCREEN);
	    if ((modes==NULL)||(modes==(SDL_Rect **)-1))
		return;
	    m=0;

	    while(*modes) {
		m++;
		modes++;
	    }

	    modes=malloc(sizeof(SDL_Rect *)*(m+1));
	    for(i=0;i<=m;i++)
		modes[i]=umodes[i];

	    qsort(modes, m, sizeof(SDL_Rect *), modecompare);

	    i=0;
	    do {
		resolutionmenusize=0;
		while(modes[i]&&(resolutionmenusize<10)) {
		    if (modes[i]->w<10000&&
			modes[i]->h<10000) {
			detectedresolution[resolutionmenusize]=
			    modes[i]->w*10000+modes[i]->h;		
			sprintf(resolutiondetectmenu[resolutionmenusize],
				"%dx%d",
				detectedresolution[resolutionmenusize]/10000,
				detectedresolution[resolutionmenusize]%10000);
			resolutionmenusize++;
		    }
		    i++;
		}
		if (modes[i]) {
		    strcpy(resolutiondetectmenu[resolutionmenusize],"More...");
		}
		a=resolutionmenu(resolutionmenusize+(modes[i]!=0),
				 -100,resolutiondetectmenu,
				 resolutionnumber);
		if (a<0) {free(modes); return;}
		} while(a==resolutionmenusize);
	    resolutionnumber=detectedresolution[a];
	    free(modes);
	    break;
	    /*
	case 1:	    
	    a=resolutionmenu(8,0,resolutionstandardmenu,resolutionnumber);
	    if (a>=0) resolutionnumber=a;
	    break;
	case 2:
	    a=resolutionmenu(11,8,resolutionspecialmenu,resolutionnumber);
	    if (a>=0) resolutionnumber=a+8;
	    break;
	    */
	case 1:
	    customresolution();
	    break;
    }
}

void setupsetfullscreen(void) {
    selectionmenu(2,fullscreenmenu,&fullscr);
}

void setupsetfiltering(void) {
    if (renderer!=1)
	selectionmenu(2,filtermenu,&nearest);
}

void setupsetrenderer(void) {
    selectionmenu(2,renderermenu,&renderer);
    if (renderer==1)
	nearest=1;
}

void setupsetmusic(void) {
    selectionmenu(MUSIC_SOURCES,musicmenu,&music);
}

void setupsetsound(void) {
    selectionmenu(2,soundmenu,&sound);
}

void setupcheatmenu(void) {
    selectionmenu(3,cheatmenu,&cheat);
}

void setupsetsoundchannels(void) {
    selectionmenu(2,channelmenu,&channel);
}

void setupsetmusicchannels(void) {
    selectionmenu(2,channelmenu,&musicchannel);
}

void setupsoundblockmenu(void) {
    selectionmenu(10,soundblockmenu,&soundblock);
}

void setuptimingmenu(void) {
    selectionmenu(2,timingmenu,&timing);
}

void setuptexturedepthmenu(void) {
    selectionmenu(3,texturedepthmenu,&texturedepth);
}

void setupscalingmodemenu(void) {
    selectionmenu(4,scalingtypemenu,&scaling);
}

void setupsetkeys(void) {
    int i=0,j,quit=0,sk;
    SDL_Event event;

    i=0;
    while(!quit) {
	drawmenu(360,240,menu);
    
	for(j=0;j<numkeys;j++) {
	    strcpy(textbuf,keynames[j]);
	    textprint(31,13+12*j,lab3dversion?32:34);
	    strncpy(textbuf,SDL_GetKeyName(newkeydefs[j]),11);
	    textbuf[11]=0;
	    textprint(261,13+12*j,lab3dversion?32:34);
	}
    
	finalisemenu();
	i=getselection(-12,-9,i,numkeys);
	if (i<0) quit=1;
	else if (i>=numkeys) quit=1;
	else {
	    j=-1;
	    while(j<0) {
		while(SDL_PollEvent(&event))
		{
		    switch(event.type)
		    {	      
			case SDL_KEYDOWN:
			    sk=event.key.keysym.sym;
			    if (sk<SDLKEYS) {
				j=sk;
			    }
			    break;
			default:
			    break;
		    }
		}
		SDL_Delay(10);
	    }
	    newkeydefs[i]=j;
	}
    }
}

void setupmenu(void) {
    int quit=0,sel=0;

    while(!quit) {
	drawmenu(360,240,menu);

	strcpy(textbuf,"LAB3D/SDL setup menu");
	textprint(81,22,126);

	strcpy(textbuf,"Input: ");
	strcat(textbuf,inputdevicemenu[inputdevice]);
	textprint(51,36,lab3dversion?32:34);
	strcpy(textbuf,"Set keys");
	textprint(51,48,lab3dversion?32:34);
	strcpy(textbuf,"Resolution: ");
	if (resolutionnumber<8)
	    strcat(textbuf,resolutionstandardmenu[resolutionnumber]);
	else if (resolutionnumber<19)
	    strcat(textbuf,resolutionspecialmenu[resolutionnumber-8]);
	else sprintf(textbuf,"Resolution: %dx%d",resolutionnumber/10000,
		     resolutionnumber%10000);
	textprint(51,60,64);
    	strcpy(textbuf,"Display type: ");
	strcat(textbuf,fullscreenmenu[fullscr]);
	textprint(51,72,64);
    	strcpy(textbuf,"Filtering: ");
	strcat(textbuf,filtermenu[nearest]);
	textprint(51,84,64);
    	strcpy(textbuf,"Renderer: ");
	strcat(textbuf,renderermenu[renderer]);
	textprint(51,96,64);
    	strcpy(textbuf,"Music: ");
	strcat(textbuf,musicmenu[music]);
	textprint(51,108,96);
    	strcpy(textbuf,"Effects: ");
	strcat(textbuf,soundmenu[sound]);
	textprint(51,120,96);
    	strcpy(textbuf,"Sound channels: ");
	strcat(textbuf,channelmenu[channel]);
	textprint(51,132,96);
    	strcpy(textbuf,"Music channels: ");
	strcat(textbuf,channelmenu[musicchannel]);
	textprint(51,144,96);
    	strcpy(textbuf,"Cheats: ");
	strcat(textbuf,cheatmenu[cheat]);
	textprint(51,156,96);
    	strcpy(textbuf,"Sound block size: ");
	strcat(textbuf,soundblockmenu[soundblock]);
	textprint(51,168,lab3dversion?32:34);
    	strcpy(textbuf,"Texture colour depth: ");
	strcat(textbuf,texturedepthmenu[texturedepth]);
	textprint(51,180,lab3dversion?32:34);
    	strcpy(textbuf,"View: ");
	strcat(textbuf,scalingtypemenu[scaling]);
	textprint(51,192,lab3dversion?32:34);
    	strcpy(textbuf,"Exit setup");
	textprint(51,204,lab3dversion?128:130);
#ifdef NEVER
    	strcpy(textbuf,"Create desktop shortcuts");
	textprint(51,204,96);
#endif

	strcpy(textbuf,"Use cursor keys and Return to select.");
	textprint(31,220,lab3dversion?32:34);

	finalisemenu();
#ifdef NEVER
	if ((sel = getselection(12,15,sel,15)) < 0)
#else
	if ((sel = getselection(12,15,sel,15)) < 0)
#endif
	    quit=1;
	else {
	    switch(sel) {
		case 0:
		    setupinputdevices();
		    break;
		case 1:
		    setupsetkeys();
		    break;
		case 2:
		    setupsetresolution();
		    break;
		case 3:
		    setupsetfullscreen();
		    break;
		case 4:
		    setupsetfiltering();
		    break;
		case 5:
		    setupsetrenderer();
		    break;
		case 6:
		    setupsetmusic();
		    break;
		case 7:
		    setupsetsound();
		    break;
		case 8:
		    setupsetsoundchannels();
		    break;
		case 9:
		    setupsetmusicchannels();
		    break;
		case 10:
		    setupcheatmenu();
		    break;
		case 11:
		    setupsoundblockmenu();
		    break;
		case 12:
		    setuptexturedepthmenu();
		    break;
		case 13:
		    setupscalingmodemenu();
		    break;
#ifdef NEVER
		case 14:
		    createshortcut();
		    break;
#endif
		case 14:
		    quit=1;
		    break;
	    }
	}
    }
}

void setrenderer(int i) {
    switch(i) {
	case 1: /* Software */
	    gfxInit=softwareInit;
	    gfxInitFailsafe=softwareInitFailsafe;
	    gfxSwapBuffers=softwareSwapBuffers;
	    gfxClearScreen=softwareClearScreen;
	    gfxClearScreenScrollyPalette=softwareClearScreenScrollyPalette;
	    gfxUploadImage=softwareUploadImage;
	    gfxTransitionTexture=softwareTransitionTexture;
	    gfxUpdateImage=softwareUpdateImage;
	    gfxFlush=softwareFlush;
	    gfxDrawFront=softwareDrawFront;
	    gfxDrawBack=softwareDrawBack;
	    gfxScreencapture=softwareScreenCapture;
	    fade=softwareFade;
	    gfxFilter=softwareFilter;
	    gfxTextureDepth=softwareTextureDepth;
	    gfxUploadPartialOverlay=softwareUploadPartialOverlay;
	    gfxUploadOverlay=softwareUploadOverlay;
	    ShowPartialOverlay=softwareShowPartialOverlay;
	    pictur=softwarePictur;
	    picrot=softwarePicrot;
	    setdarkenedpalette=softwareSetDarkenedPalette;
	    updateoverlaypalette=softwareUpdateOverlayPalette;
	    settransferpalette=softwareSetTransferPalette;
	    drawvolumebar=softwareDrawVolumeBar;
	    flatsprite=softwareFlatsprite;
	    doordraw=softwareDoordraw;
	    paletterenderer=1;
	    break;
	default: /* OpenGL (0) */
	    gfxInit=openGLInit;
	    gfxInitFailsafe=openGLInitFailsafe;
	    gfxSwapBuffers=openGLSwapBuffers;
	    gfxClearScreen=openGLClearScreen;
	    gfxClearScreenScrollyPalette=openGLClearScreen;
	    gfxUploadImage=openGLUploadImage;
	    gfxTransitionTexture=openGLTransitionTexture;
	    gfxUpdateImage=openGLUpdateImage;
	    gfxFlush=openGLFlush;
	    gfxDrawFront=openGLDrawFront;
	    gfxDrawBack=openGLDrawBack;
	    gfxScreencapture=openGLScreencapture;
	    fade=openGLFade;
	    gfxFilter=openGLFilter;
	    gfxTextureDepth=openGLTextureDepth;
	    gfxUploadPartialOverlay=openGLUploadPartialOverlay;
	    gfxUploadOverlay=openGLUploadOverlay;
	    ShowPartialOverlay=openGLShowPartialOverlay;
	    pictur=openGLPictur;
	    picrot=openGLPicrot;
	    setdarkenedpalette=openGLSetDarkenedPalette;
	    updateoverlaypalette=openGLUpdateOverlayPalette;
	    settransferpalette=openGLSetTransferPalette;
	    drawvolumebar=openGLDrawVolumeBar;
	    flatsprite=openGLFlatsprite;
	    doordraw=openGLDoordraw;
	    paletterenderer=0;
    }
}

void configure(void) {
    int div1,div2;

    setrenderer(renderer);

    if (resolutionnumber<19) {
	screenwidth=resx[resolutionnumber];
	screenheight=resy[resolutionnumber];
    } else {
	/* Encoding chosen to be human-readable. Backwards compatibility
	   kludge. */
	screenwidth=resolutionnumber/10000;
	screenheight=resolutionnumber%10000;
    }
    fullscreen=fullscr;
    gfxFilter(!nearest);
    speechstatus = sound?2:0;
    switch(music) {
	case 2:
	    musicsource=1;
	    break;	    
	case 1:
	    musicsource=2;
	    break;
	default:
	    musicsource=-1;
	    break;
    }
    moustat = ((3-inputdevice)&1);
    joystat = ((3-inputdevice)>>1);
    cheatenable=cheat;
    if (channel||musicchannel)
	channels=2;
    else
	channels=1;

    soundpan=channel;
    musicpan=musicchannel;

    soundblocksize=channels*
	((musicsource==2)?SOUNDBLOCKSIZE44KHZ:SOUNDBLOCKSIZE11KHZ);
    if (soundblock>0) {
	soundblocksize>>=4;
	soundblocksize<<=soundblock;
    }
    soundtimer=0;
    gfxTextureDepth(texturedepth);
    aspw=1.0;
    asph=1.0;
    switch(scaling) {
	case 1:
	case 3:
	    div1=screenwidth/320;
	    div2=screenheight/200;

	    if (div2<div1) div1=div2;
	    if (div1<1) {
	        fprintf(stderr,
		        "Warning: resolution must be 320x200 or more"
		        " for integer scaling.\n");	    
	        virtualscreenwidth=360;
	        virtualscreenheight=240;
	    } else {
	        virtualscreenwidth=screenwidth/div1;
	        virtualscreenheight=screenheight/div1;
            }
            if (scaling==3) {
	        if (screenwidth*3>screenheight*4)
		    aspw=((((double)screenwidth*3.0))/(((double)screenheight)*4.0));
		else
		    asph=((((double)screenheight)*4.0))/(((double)screenwidth*3.0));
    	    }
            break;
	case 0:
	    virtualscreenwidth=360;
	    virtualscreenheight=240;
            break;
	default:
	    if (screenwidth*3>screenheight*4) {
		aspw=((((double)screenwidth*3.0))/(((double)screenheight)*4.0));
	        virtualscreenwidth=360.0*aspw;
	        virtualscreenheight=240;
    	    } else {
		asph=((((double)screenheight)*4.0))/(((double)screenwidth*3.0));
	        virtualscreenwidth=360;
	        virtualscreenheight=240.0*asph;
    	    }
    }
}

void loadsettings(void) {
    FILE *file=fopen("settings.ini","r");
    int i;
    SDL_Rect **modes;

    channels=2; musicvolume=64; soundvolume=64; gammalevel=1.0;
    modes=SDL_ListModes(NULL,SDL_FULLSCREEN);
    i=0;
    if ((modes!=NULL)&&(modes!=(SDL_Rect **)-1)&&(modes[0]!=NULL))
	resolutionnumber=modes[0]->w*10000+modes[0]->h;

    for(i=0;i<numkeys;i++)
	newkeydefs[i]=newdefaultkey[i];

    if (lab3dversion) {
	newkeydefs[16]=SDLK_l;
	newkeydefs[13]=SDLK_s;
    }

    if (file==NULL)
	setup();
    i=fscanf(file,"%d %d %d %d %d %d",&inputdevice,&resolutionnumber,&fullscr,
	     &nearest,&music,&sound); /* Non-existent set to defaults. */
    if (i==6) {
	for(i=0;i<numkeys;i++)
	    if (fscanf(file,"%d\n",newkeydefs+i)!=1) break;
    } else i=0;

    if (i==numkeys) {
	i=fscanf(file,"%d %d\n",&soundvolume,&musicvolume);
    } else i=0;

    if (i==2) {
	i=fscanf(file,"%d\n",&cheat);
    } else i=0;

    if (i) i=fscanf(file,"%d\n",&channel);

    if (i) i=fscanf(file,"%f\n",&gammalevel);

    if (i) i=fscanf(file,"%d\n",&soundblock);

    if (i) i=fscanf(file,"%d\n",&timing); /* Left in case I put it back. */

    if (i) i=fscanf(file,"%d\n",&texturedepth);

    if (i) i=fscanf(file,"%d\n",&musicchannel);

    if (i) i=fscanf(file,"%d\n",&scaling);

    if (i) i=fscanf(file,"%d\n",&renderer);

    fclose(file);
}

void savesettings(void) {
    FILE *file=fopen("settings.ini","w");
    int i;

    if (file==NULL) return;
    fprintf(file,"%d %d %d %d %d %d\n",inputdevice,resolutionnumber,fullscr,
	    nearest,music,sound);    
    for(i=0;i<numkeys;i++)
	fprintf(file,"%d\n",newkeydefs[i]);
    fprintf(file,"%d %d\n",soundvolume,musicvolume);
    fprintf(file,"%d\n",cheat);
    fprintf(file,"%d\n",channel);
    fprintf(file,"%f\n",gammalevel);
    fprintf(file,"%d\n",soundblock);
    fprintf(file,"%d\n",timing);
    fprintf(file,"%d\n",texturedepth);
    fprintf(file,"%d\n",musicchannel);
    fprintf(file,"%d\n",scaling);
    fprintf(file,"%d\n",renderer);

    fclose(file);
}

void setup(void) {
    K_INT16 i, j, k, walcounter;
    K_UINT16 l;
    char *v;

    configure();
    setrenderer(1);
    statusbaryoffset=250;

    /* Initialise SDL; uncomment the NOPARACHUTE bit if the parachute
       routine (which catches stuff like segfaults) gets in the way of your
       debugging. */

    /* Display accuracy not important in setup... */

    SDL_Init(SDL_INIT_TIMER|SDL_INIT_VIDEO|SDL_INIT_NOPARACHUTE|
	     SDL_INIT_JOYSTICK);

    gfxInitFailsafe();

    linecompare(479);

    if (screenbuffer==NULL) {
	fprintf(stderr,"Insufficient memory.\n");
	SDL_Quit();
	exit(-1);
    }
    
    fprintf(stderr,"Loading configuration file...\n");

    loadtables();
    gfxFilter(0);
    vidmode = 1;
    mute = 0;
    moustat = 1;
    joystat = 1;

    musicsource=-1;
    speechstatus=0;

    fprintf(stderr,"Allocating memory...\n");
    if (((lzwbuf = malloc(12304-8200)) == NULL)||
	((lzwbuf2=malloc(8200))==NULL))
    {
	fprintf(stderr,"Error #3: Memory allocation failed.\n");
	SDL_Quit();
	exit(-1);
    }

    convwalls = numwalls;

    if ((pic = malloc((numwalls-initialwalls)<<12)) == NULL)
    {
	fprintf(stderr,
		"Error #4: This computer does not have enough memory.\n");
	SDL_Quit();
	exit(-1);
    }
    walcounter = initialwalls;
    if (convwalls > initialwalls)
    {
	v = pic;
	for(i=0;i<convwalls-initialwalls;i++)
	{
	    walseg[walcounter] = v;
	    walcounter++;
	    v += 4096;
	}
    }
    l = 0;
    for(i=0;i<240;i++)
    {
	times90[i] = l;
	l += 90;
    }
    less64inc[0] = 16384;
    for(i=1;i<64;i++)
	less64inc[i] = 16384 / i;
    for(i=0;i<256;i++)
	keystatus[i] = 0;

    saidwelcome = 0;
    fprintf(stderr,"Loading intro pictures...\n");

    if (lab3dversion) {
	kgif(-1);
	k=0;
	for(i=0;i<16;i++)
	    for(j=1;j<17;j++)
	    {
		spritepalette[k++] = (opaldef[i][0]*j)/17;
		spritepalette[k++] = (opaldef[i][1]*j)/17;
		spritepalette[k++] = (opaldef[i][2]*j)/17;
	    }
	fprintf(stderr,"Loading old graphics...\n");
	loadwalls();
	fade(63);
	k=0;
	for(i=0;i<16;i++)
	    for(j=1;j<17;j++)
	    {
		palette[k++] = (opaldef[i][0]*j)/17;
		palette[k++] = (opaldef[i][1]*j)/17;
		palette[k++] = (opaldef[i][2]*j)/17;
	    }
	settransferpalette();
    } else {
	/* The ingame palette is stored in this GIF! */
	kgif(1);
	memcpy(spritepalette,palette,768);
	
	kgif(0);
	settransferpalette();
	fprintf(stderr,"Loading graphics...\n");
	loadwalls();

	kgif(1);
    }
    fade(63);
    gfxDrawFront();

    strcpy(keynames[16], "LOAD game");
    strcpy(keynames[13], "SAVE game");

    setupmenu();
  
    savesettings();
    SDL_Quit();
    exit(0);
}
