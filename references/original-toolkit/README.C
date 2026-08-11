#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>

static char buffer[16384];

main()
{
	char ch;
	int i, fil, bytecnt, leng, xpos, incr, extralines;

	if ((fil = open("readme.txt",O_BINARY|O_RDONLY,S_IREAD)) == -1)
		return(-1);
	leng = read(fil,&buffer[0],16384);
	incr = 1;
	bytecnt = 0;
	extralines = 0;
	_asm \
	{
		mov ax, 0x3
		int 0x10
		mov ax, 0xb800
		mov es, ax
		mov di, 0
		mov cx, 2080
		mov ax, 0x0700
		rep stosw
	}
	outp(0x3d4,0xa); outp(0x3d5,inp(0x3d5)|32);
	while ((extralines < 27) && (ch != 27))
	{
		for(i=incr;i<16;i+=incr)
		{
			_asm \
			{
				mov dx, 0x3da
waitnoretrace1:
				in al, dx
				test al, 8
				jnz waitnoretrace1
				sub dx, 6
				mov al, 0x8
				out dx, al
				inc dx
				mov ax, i
				out dx, al
				mov dx, 0x3da
waitretrace1:
				in al, dx
				test al, 8
				jz waitretrace1
			}
		}
		_asm \
		{
			mov dx, 0x3da
waitnoretrace2:
			in al, dx
			test al, 8
			jnz waitnoretrace2
			sub dx, 6
			mov al, 0x8
			out dx, al
			inc dx
			mov al, 0
			out dx, al
			mov cx, 0
			mov ax, 0xb800
			push ds
			mov ds, ax
			mov di, 0
			mov si, 160
			mov cx, 2080
			cld
			rep movsw
			pop ds
		}
		if (extralines == 0)
		{
			xpos = 4000;
			while (buffer[bytecnt] != 13)
			{
				if (buffer[bytecnt] != 9)
					_asm \
					{
						mov di, xpos
						mov bx, bytecnt
						mov al, byte ptr buffer[bx]
						mov ah, 0x5a
						stosw
						add xpos, 2
					}
				else
					_asm \
					{
						mov di, xpos
						mov ax, 0
						stosw
						stosw
						stosw
						add xpos, 6
					}
				bytecnt++;
				if (bytecnt == leng)
				{
					leng = read(fil,&buffer[0],16384);
					bytecnt = 0;
				}
			}
			if (xpos < 4160)
				_asm \
				{
					mov di, xpos
					mov cx, 4160
					sub cx, di
					shr cx, 1
					mov ax, 0x0000
					rep stosw
				}
			if (buffer[bytecnt] == 13)
			{
				bytecnt++;
				if (bytecnt == leng)
				{
					leng = read(fil,&buffer[0],16384);
					bytecnt = 0;
				}
				if (buffer[bytecnt] == 10)
				{
					bytecnt++;
					if (bytecnt == leng)
					{
						leng = read(fil,&buffer[0],16384);
						bytecnt = 0;
					}
				}
			}
			if (leng == 0)
				extralines = 1;
		}
		else
		{
			_asm \
			{
				mov di, 4000
				mov cx, 80
				mov ax, 0x0000
				cld
				rep stosw
			}
			extralines++;
		}
		_asm \
		{
			mov dx, 0x3da
waitretrace2:
			in al, dx
			test al, 8
			jz waitretrace2
		}
		if (kbhit() != 0)
			ch = getch();
		if ((ch == '+') && (incr < 16))
			incr = (incr<<1), ch = 0;
		if (ch == '-')
		{
			if (incr == 1)
				ch = getch();
			else
				incr = (incr>>1), ch = 0;
		}
		if (ch == 32)
			ch = getch(), ch = 0;
	}
	close(fil);
	_asm \
	{
		mov ax, 0x3
		int 0x10
		mov ax, 0xb800
		mov es, ax
		mov di, 0
		mov cx, 2080
		mov ax, 0x0700
		rep stosw
	}
}
