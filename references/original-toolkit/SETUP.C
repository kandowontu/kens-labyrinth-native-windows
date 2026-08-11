#include <dos.h>
#include <stdio.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#define numoptions 4
#define numkeys 18
#define keystart 30

static unsigned char readch, oldreadch, keystatus[256], extended;
static unsigned char screeninfo[4000];
static unsigned char *keytable[] =
{
	"-","ESC","1","2","3","4","5","6","7","8","9","0","-","=","BACKSPC","TAB",
	"Q","W","E","R","T","Y","U","I","O","P","[","]","ENTER","L-CTRL","A","S",
	"D","F","G","H","J","K","L",";","'","`","L-SHIFT","|","Z","X","C","V",
	"B","N","M",",",".","/","R-SHIFT","KP-*","L-ALT","SPACEBAR","CAPSLOCK","F1","F2","F3","F4","F5",
	"F6","F7","F8","F9","F10","NUMLOCK","SCROLL","KP-7","KP-8","KP-9","KP--","KP-4","KP-5","KP-6","KP-+","KP-1",
	"KP-2","KP-3","KP-0","KP-.","-","-","-","F11","F12","-","-","-","-","-","-","-",
	"-","-","-","-","-","-","-","-","-","-","-","-","-","-","-","-",
	"-","-","-","-","-","-","-","-","-","-","-","-","-","-","-","-",
	"-","-","-","-","-","-","-","-","-","-","-","-","-","-","-","-",
	"-","-","-","-","-","-","-","-","-","-","-","-","KP-ENTER","R-CTRL","-","-",
	"-","-","-","-","-","-","-","-","-","-","-","-","-","-","-","-",
	"-","-","-","-","-","KP-/","-","-","R-ALT","-","-","-","-","-","-","-",
	"-","-","-","-","-","-","-","HOME","UP","PAGEUP","-","LEFT","-","RIGHT","-","END",
	"DOWN","PAGEDOWN","INSERT","DELETE","-","-","-","-","-","-","-","-","-","-","-","-",
	"-","-","-","-","-","-","-","-","-","-","-","-","-","-","-","-",
	"-","-","-","-","-","-","-","-","-","-","-","-","-","-","-","-"
};
static unsigned char defaultkey[numkeys] =
{
	0xc8,0xd0,0xcb,0xcd,0x9d,0x1e,0x2c,0x2a,0x1d,
	0x3b,0x3c,0x3d,0x39,0x0e,0x1c,0x19,0x32,0x01
};
static unsigned char *strtable[] =
{
	"Graphics Mode",
	"Digitized Sound",
	"Music Source",
	"Ä",
	"Input Devices",
	"Custom Keys",
	"Calibrate Joystick",
	"Ä",
	"Ignore Changes and Quit",
	"Save Changes and Quit",

	"320*200 (Standard VGA resolution)",
	"360*240 (May not work with some monitors)",

	"No digitized sound",
	"Ä",
	"PC speaker digitized sound (Fast machine required)",
	"Ä",
	"Sound Blaster (Port 220 hex) DEFAULT",
	"Sound Blaster (Port 230 hex)",
	"Sound Blaster (Port 240 hex)",
	"Sound Blaster (Port 250 hex)",
	"Sound Blaster (Port 260 hex)",

	"No background music",
	"Ä",
	"PC speaker background music",
	"MPU-401 MIDI background music",
	"Adlib background music",

	"Keyboard º ÄÄÄÄÄÄÄ º ÄÄÄÄÄÄÄ",
	"Keyboard º  Mouse  º ÄÄÄÄÄÄÄ",
	"Keyboard º ÄÄÄÄÄÄÄ º Joystick",
	"Keyboard º  Mouse  º Joystick",

	"Move FORWARD                                                                    ",
	"Move BACKWARD                                                                   ",
	"Turn LEFT                                                                       ",
	"Turn RIGHT                                                                      ",
	"STRAFE (walk sideways)                                                          ",
	"STAND HIGH                                                                      ",
	"STAND LOW                                                                       ",
	"RUN                                                                             ",
	"Ä",
	"FIRE                                                                            ",
	"Select FIREBALLS (red)                                                          ",
	"Select BOUNCY-BULLETS (green)                                                   ",
	"Select HEAT-SEEKING MISSILES                                                    ",
	"UNLOCK / OPEN / CLOSE / USE                                                     ",
	"Ä",
	"CHEAT for more life                                                             ",
	"RAISE / LOWER STATUS BAR                                                        ",
	"PAUSE GAME                                                                      ",
	"MUTE KEY                                                                        ",
	"SHOW MENU (load,save,info,quit,etc.)                                            ",
};

static int joyx1, joyy1, joyx2, joyy2, joyx3, joyy3;
static int xchange, ychange, bstatus, keypos;
static unsigned ksayfreq;
unsigned char option[numoptions], keys[numkeys];
void interrupt far keyhandler(void);
void (interrupt far *oldkeyhandler)();

main()
{
	int i, j;
	unsigned char ch, col;

	loadsetup();
	drawscreen();
	savescreen();
	i = 0;
	keypos = 0;
	do
	{
		restorescreen();
		printstr(35,5,"Main Menu",32);
		i = menu(0,10,i);
		restorescreen();
		if (i == 0)
		{
			printstr(29,9,"Graphics Mode Selection",32);
			if ((j = menu(10,2,option[0])) >= 0)
				option[0] = j;
		}
		if (i == 1)
		{
			printstr(28,6,"Digitized Sound Selection",32);
			if ((j = menu(12,9,option[1])) >= 0)
			{
				option[1] = j;
				if (option[1] == 1)
				{
					restorescreen();
					printstr(17,11,"ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿",19);
					printstr(17,12,"³                                                 ³",19);
					printstr(17,13,"³                                                 ³",19);
					printstr(17,14,"ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ",19);
					printstr(19,12,"PC Speaker Interrupt Frequency (+/- to change)",19);
					printstr(19,13,"(Default is 22050hz)",19);
					printchr(55,13,'h',75,1);
					printchr(56,13,'z',75,1);
					ch = 0;
					j = ksayfreq;
					while ((ch != 13) && (ch != 27))
					{
						if (j < 11025) j = 11025;
						if (j > 22050) j = 22050;
						printchr(50,13,(j/10000)%10+48,75,1);
						printchr(51,13,(j/1000)%10+48,75,1);
						printchr(52,13,(j/100)%10+48,75,1);
						printchr(53,13,(j/10)%10+48,75,1);
						printchr(54,13,j%10+48,75,1);
						ch = getch();
						if (ch == '+')
							j += 315;
						if (ch == '-')
							j -= 315;
					}
					if (ch == 13)
						ksayfreq = j;
				}
			}
		}
		if (i == 2)
		{
			printstr(29,8,"Music Source Selection",32);
			if ((j = menu(21,5,option[2])) >= 0)
				option[2] = j;
		}
		if (i == 3)
		{
			printstr(29,8,"Input Device Selection",32);
			if ((j = menu(26,4,option[3])) >= 0)
				option[3] = j;
		}
		if (i == 4) definekeys();
		if (i == 5) adjustjoystick();
		if (i == 7)
			savesetup();
	}
	while ((i >= 0) && (i <= 5));
	_asm \
	{
		mov ax, 0x3
		int 0x10
	}
}

loadsetup()
{
	int fil, i, j;

	if ((fil = open("tables.dat",O_BINARY|O_RDONLY,S_IREAD)) == -1)
	{
		printf("Cannot load options");
		exit(0);
	}
	lseek(fil,8192L+4096L+720L,SEEK_SET);
	read(fil,&option[0],numoptions);
	read(fil,&keys[0],numkeys);
	read(fil,&joyx1,2);
	read(fil,&joyy1,2);
	read(fil,&joyx2,2);
	read(fil,&joyy2,2);
	read(fil,&joyx3,2);
	read(fil,&joyy3,2);
	read(fil,&ksayfreq,2);
	close(fil);
}

savesetup()
{
	int fil, i, j;

	if ((fil = open("tables.dat",O_BINARY|O_WRONLY,S_IWRITE)) == -1)
	{
		printf("Cannot save options");
		exit(0);
	}
	lseek(fil,8192L+4096L+720L,SEEK_SET);
	write(fil,&option[0],numoptions);
	write(fil,&keys[0],numkeys);
	write(fil,&joyx1,2);
	write(fil,&joyy1,2);
	write(fil,&joyx2,2);
	write(fil,&joyy2,2);
	write(fil,&joyx3,2);
	write(fil,&joyy3,2);
	write(fil,&ksayfreq,2);
	close(fil);
}

printchr(x,y,character,attribute,len)
int x, y, len;
unsigned char character, attribute;
{
	int pos;

	pos = (y*80+x)<<1;
	_asm \
	{
		mov ax, 0xb800
		mov es, ax
		mov di, pos
		mov al, character
		mov ah, attribute
		mov cx, len
		cld
		rep stosw
	}
}

printstr(x,y,string,attribute)
int x, y;
unsigned char string[81], attribute;
{
	unsigned char character;
	int i, pos;

	pos = (y*80+x)<<1;
	i = 0;
	while (string[i] != 0)
	{
		character = string[i];
		_asm \
		{
			mov ax, 0xb800
			mov es, ax
			mov di, pos
			mov al, character
			mov ah, attribute
			stosw
		}
		i++;
		pos+=2;
	}
}

void interrupt far keyhandler()
{
	oldreadch = readch;
	_asm \
	{
		in al, 0x60
		mov readch, al
		in al, 0x61
		or al, 0x80
		out 0x61, al
		and al, 0x7f
		out 0x61, al
	}
	if (readch == 0xe0)
		extended = 128;
	else
	{
		if (oldreadch != readch)
			keystatus[(readch&127)+extended] = ((readch>>7)^1);
		extended = 0;
	}
	_asm \
	{
		mov al, 0x20
		out 0x20, al
	}
}

drawscreen()
{
	int i;

	_asm \
	{
		mov ax, 0x3
		int 0x10
	}
	outp(0x3d4,0x0e); outp(0x3d5,255);
	printchr(0,0,218,78,1);
	printchr(1,0,196,78,78);
	printchr(79,0,191,78,1);
	for(i=1;i<24;i++)
	{
		printchr(0,i,179,78,1);
		printchr(1,i,32,78,78);
		printchr(79,i,179,78,1);
	}
	printchr(0,24,192,78,1);
	printchr(1,24,196,78,78);
	printchr(79,24,217,78,1);
	printstr(26,1,"Ken's Labyrinth Setup Screen",79);
	printstr(29,21,"ARROWS to move cursor.",71);
	printstr(32,22,"ENTER to select.",71);
	printstr(33,23,"ESC to cancel.",71);
}

joystick(x, y, b)
int x,y,b;
{
	_asm \
	{
		mov dx, 0x201
		xor al, al
		out dx, al
		mov si, 0
		mov di, 0
		mov cx, 0xffff
joystickloop:
		in al, dx
		mov bl, al
		mov bh, al
		and bx, 0x0201
		add bl, 0xff
		adc si, 0
		add bh, 0xff
		adc di, 0
		and al, 3
		loopnz joystickloop
endjoystickloop:
		mov bx, x
		mov ds:[bx], si
		mov bx, y
		mov ds:[bx], di
		in al, dx
		mov cl, 4
		shr al, cl
		and ax, 3
		xor al, 3
		mov bx, b
		mov ds:[bx], ax
	}
}

adjustjoystick()
{
	int i;
	unsigned char ch;

	printstr(29,10,"Calibration Joystick...",32);
	printstr(20,11,"ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿",19);
	printstr(20,12,"³                                           ³",19);
	printstr(20,13,"ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ",19);
	printstr(22,12,"Center joystick then press the space bar.",75);
	ch = getch();
	joyx2 = 0, joyy2 = 0;
	for(i=0;i<4;i++)
	{
		joystick(&xchange,&ychange,&bstatus);
		joyx2 += xchange, joyy2 += ychange;
	}
	joyx2 >>= 2, joyy2 >>= 2;
	printstr(11,11,"ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿",19);
	printstr(11,12,"³                                                             ³",19);
	printstr(11,13,"ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ",19);
	printstr(13,12,"Move joystick in all 4 directions then press the space bar.",75);
	joyx1 = 32767, joyy1 = 32767, joyx3 = 0, joyy3 = 0;
	while (kbhit() == 0)
	{
		joystick(&xchange,&ychange,&bstatus);
		if (xchange < joyx1) joyx1 = xchange;
		if (ychange < joyy1) joyy1 = ychange;
		if (xchange > joyx3) joyx3 = xchange;
		if (ychange > joyy3) joyy3 = ychange;
	}
	joyx1 += 10, joyy1 += 10, joyx3 -= 10, joyy3 -= 10;
	ch = getch();
	printstr(20,16,"                                                        ",79);
}

savescreen()
{
	_asm \
	{
		cld
		mov cx, 4000
		mov ax, seg screeninfo
		mov es, ax
		mov di, offset screeninfo
		xor si, si
		mov ax, 0xb800
		push ds
		mov ds, ax
		rep movsb
		pop ds
	}
}

restorescreen()
{
	_asm \
	{
		cld
		mov cx, 4000
		mov ax, 0xb800
		mov es, ax
		xor di, di
		mov si, offset screeninfo
		rep movsb
	}
}

definekeys()
{
	int i, j, k;

	printstr(33,0,"Custom Key Menu",32);
	printstr(1,23,"Change selected key by pressing ENTER, then the KEY. ESC returns to main menu.",71);
	printstr(26,24," Press F1 for default keys. ",71);
	for(i=0;i<20;i++)
		if (strtable[i+keystart][0] != 196)
		{
			j = 0;
			while (strtable[i+keystart][j] != 0)
				j++;
			while (j < 62)
			{
				strtable[i+keystart][j] = 32;
				j++;
			}
			k = i;
			if (k >= 14) k--;
			if (k >= 8) k--;
			j = 0;
			while ((j < 8) & (keytable[keys[k]][0] != 0))
			{
				strtable[i+keystart][j+54] = keytable[keys[k]][j];
				j++;
			}
			while (j < 8)
			{
				strtable[i+keystart][j+54] = 32;
				j++;
			}
			strtable[i+keystart][62] = 0;
		}
	do
	{
		if (((keypos = menu(keystart,20,keypos))&512) == 0)
		{
			k = keypos;
			if (k >= 8) k++;
			if (k >= 14) k++;
			k = (k+2)*160+17;
			_asm \
			{
				mov ax, 0xb800
				mov es, ax
				mov di, k
				mov cx, 54
erasedefinekeyline:
				mov es:[di], 19
				add di, 2
				loop erasedefinekeyline
			}
			oldkeyhandler = _dos_getvect(0x9);
			_disable(); _dos_setvect(0x9, keyhandler); _enable();
			for(i=0;i<256;i++)
				keystatus[i] = 0;
			j = 0;
			i = 0;
			while (j == 0)
			{
				i = ((i+1)&255);
				if ((i != 0) && (i != 0xaa) && (keystatus[i] != 0))
				{
					k = keypos;
					if (k >= 8) k++;
					if (k >= 14) k++;
					keys[keypos] = i;
					j = 0;
					while ((j < 8) & (keytable[keys[keypos]][0] != 0))
					{
						strtable[k+keystart][j+54] = keytable[keys[keypos]][j];
						j++;
					}
					while (j < 8)
					{
						strtable[k+keystart][j+54] = 32;
						j++;
					}
					strtable[k+keystart][62] = 0;
					j = 1;
				}
			}
			_disable(); _dos_setvect(0x9, oldkeyhandler); _enable();
		}
		if (keypos >= 512)
		{
			keypos -= 512;
			for(i=0;i<20;i++)
				if (strtable[i+keystart][0] != 196)
				{
					j  = 0;
					while (strtable[i+keystart][j] != 0)
						j++;
					while (j < 62)
						strtable[i+keystart][j++] = 32;
					k = i;
					if (k >= 14) k--;
					if (k >= 8) k--;
					j = 0;
					while ((j < 8) & (keytable[keys[k]][0] != 0))
					{
						strtable[i+keystart][j+54] = keytable[keys[k]][j];
						j++;
					}
					while (j < 8)
					{
						strtable[i+keystart][j+54] = 32;
						j++;
					}
					strtable[i+keystart][62] = 0;
				}
		}
	}
	while (keypos >= 0);
	keypos = -(keypos+1);
}

menu(firstring,numstrings,selection)
int firstring, numstrings, selection;
{
	unsigned char ch, col, buffer[80];
	int i, j, k, xdim, ydim, x1, y1;

	xdim = 0;
	for(i=firstring;i<firstring+numstrings;i++)
	{
		k = strlen(&strtable[i][0]);
		if (k > xdim)
			xdim = strlen(&strtable[i][0]);
	}
	xdim += 4;
	if (firstring == keystart)
		xdim = 66;
	ydim = numstrings+2;
	x1 = 39-(xdim>>1);
	y1 = 12-(ydim>>1);
	buffer[0] = 218;
	for(i=1;i<xdim-1;i++)
		buffer[i] = 196;
	buffer[xdim-1] = 191;
	buffer[xdim] = 0;
	printstr(x1,y1,buffer,19);
	for(i=y1+1;i<y1+ydim-1;i++)
	{
		if (strtable[firstring+i-(y1+1)][0] == 196)
		{
			buffer[0] = 195;
			for(j=1;j<xdim-1;j++)
				buffer[j] = 196;
			buffer[xdim-1] = 180;
			buffer[xdim] = 0;
			printstr(x1,i,buffer,19);
			if (selection >= i-(y1+1))
				selection++;
		}
		else
		{
			buffer[0] = 179;
			for(j=1;j<xdim-1;j++)
				buffer[j] = 32;
			buffer[xdim-1] = 179;
			buffer[xdim] = 0;
			printstr(x1,i,buffer,19);
		}
	}
	buffer[0] = 192;
	for(i=1;i<xdim-1;i++)
		buffer[i] = 196;
	buffer[xdim-1] = 217;
	buffer[xdim] = 0;
	printstr(x1,y1+ydim-1,buffer,19);
	ch = 0;
	while ((ch != 13) && (ch != 27))
	{
		for(i=firstring;i<firstring+numstrings;i++)
		{
			if (i == selection+firstring)
				col = 75;
			else
				col = 19;
			if (strtable[i][0] != 196)
				printstr(x1+2,y1+1+i-firstring,&strtable[i][0],col);

		}
		ch = getch();
		if (ch == 0)
		{
			ch = getch();
			if ((ch == 72) || (ch == 75))
			{
				do
				{
					selection--;
					if (selection < 0)
						selection = numstrings-1;
				}
				while (strtable[firstring+selection][0] == 196);
			}
			if ((ch == 80) || (ch == 77))
			{
				do
				{
					selection++;
					if (selection >= numstrings)
						selection = 0;
				}
				while (strtable[firstring+selection][0] == 196);
			}
			if ((ch == 59) && (firstring == keystart))
			{
				for(i=0;i<numkeys;i++)
					keys[i] = defaultkey[i];
				for(i=ydim-1;i>=0;i--)
					if (strtable[firstring+i][0] == 196)
						if (selection >= i)
							selection--;
				return(512+selection);
			}
			ch = 0;
		}
	}
	for(i=ydim-1;i>=0;i--)
		if (strtable[firstring+i][0] == 196)
			if (selection >= i)
				selection--;
	if (ch == 27)
		return(-1-selection);
	else
		return(selection);
}
