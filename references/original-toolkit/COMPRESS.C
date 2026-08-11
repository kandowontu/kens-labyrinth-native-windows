#include <dos.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#define numwalls 448
#define numlevels 30
#define wavnum 33

static char wavfiles[wavnum][9] =
{
	"batdie","blowup","bounce","bullseye","closdoor","death","fall",
	"getstuff","hitenemy","hitfan","killcube","nicejob","notnow",
	"opendoor","ouch","shoot","unlock","warp","welcome","cheat",
	"diearth","dienow","shoot2","wingame","getsoda","clink","nothing",
	"select","water","updowndr","crumble","clock","bark"
};

static unsigned char value[4360], buf1[4096], buf2[20000];
static unsigned int child[4360], sibly[4360];
static unsigned int buf3[128];
static unsigned int pow2[13] = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096};
static int filecount, filength[256], tileng[numwalls+1];
static long offset[256];
static unsigned char filenames[256][9];

main(int argc,char **argv)
{
	unsigned char oldat, dat, bitcnt, filename[80], buffer[80];
	int i, j, status, fil1, fil2, leng;
	int addrcnt, bytecnt1, bytecnt2, numbits, addr, newaddr;
	unsigned int cnt;
	long bytecnt;

	if (argc == 2)
		strcpy(filename,argv[1]);
	if ((argc != 2) || (strcmp(filename,"walls") == 0))
	{
		printf("\n");
		if ((fil1 = open("walls.dat",O_BINARY|O_RDWR,S_IREAD)) == -1)
		{
			printf("Unable to open walls.dat\n");
			exit(-1);
		}
		if ((fil2 = open("walls.kzp",O_BINARY|O_TRUNC|O_CREAT|O_WRONLY,S_IWRITE)) == -1)
		{
			printf("Unable to create walls.kzp\n");
			exit(-1);
		}
		read(fil1,&buf1[0],1024);
		write(fil2,&buf1[0],numwalls*3);
		for(j=0;j<numwalls;j++)
		{
			printf("%d ",j);
			read(fil1,&buf1[0],4096);
			for(i=0;i<256;i++)
			{
				value[i] = i;
				child[i] = 65535;
				sibly[i] = (i+1)&255;
			}
			addrcnt = 256;
			bytecnt1 = 0;
			bitcnt = 0;
			bytecnt2 = 0;
			dat = 0;
			numbits = 9;
			do
			{
				addr = buf1[bytecnt1];
				do
				{
					bytecnt1++;
					if (bytecnt1 == 4096)
						break;
					if (child[addr] == 65535)
					{
						child[addr] = addrcnt;
						break;
					}
					newaddr = child[addr];
					while (value[newaddr] != buf1[bytecnt1])
					{
						if (sibly[newaddr] == 65535)
						{
							sibly[newaddr] = addrcnt;
							break;
						}
						newaddr = sibly[newaddr];
					}
					if (sibly[newaddr] == addrcnt)
						break;
					addr = newaddr;
				}
				while (addr >= 0);
				value[addrcnt] = buf1[bytecnt1];
				child[addrcnt] = 65535;
				sibly[addrcnt] = 65535;
				for(i=0;i<numbits;i++)
				{
					if ((addr&pow2[i]) > 0)
						dat += pow2[bitcnt];
					bitcnt++;
					if (bitcnt == 8)
					{
						bitcnt = 0;
						buf2[bytecnt2] = dat;
						bytecnt2++;
						dat = 0;
					}
				}
				addrcnt++;
				if ((addrcnt == 512) || (addrcnt == 1024) || (addrcnt == 2048) || (addrcnt == 4096))
					numbits++;
			}
			while (bytecnt1 < 4096);
			for(i=0;i<numbits;i++)
			{
				if ((addr&pow2[i]) > 0)
					dat += pow2[bitcnt];
				bitcnt++;
				if (bitcnt == 8)
				{
					bitcnt = 0;
					buf2[bytecnt2] = dat;
					bytecnt2++;
					dat = 0;
				}
			}
			if (bytecnt2 < 4096)
			{
				write(fil2,&addrcnt,2);
				write(fil2,&buf2[0],bytecnt2);
			}
			else
			{
				addrcnt = 0;
				bytecnt2 = 4096;
				write(fil2,&addrcnt,2);
				write(fil2,&buf1[0],bytecnt2);
			}
			tileng[j] = bytecnt2;
		}
		lseek(fil2,numwalls,SEEK_SET);
		write(fil2,&tileng[0],numwalls*2);
		close(fil1);
		close(fil2);
	}

	//-----------------------------------------------------------------------

	if ((argc != 2) || (strcmp(filename,"boards") == 0))
	{
		printf("\n");
		if ((fil1 = open("boards.dat",O_BINARY|O_RDWR,S_IREAD)) == -1)
		{
			printf("Unable to open boards.dat\n");
			exit(-1);
		}
		if ((fil2 = open("boards.kzp",O_BINARY|O_TRUNC|O_CREAT|O_WRONLY,S_IWRITE)) == -1)
		{
			printf("Unable to create boards.kzp\n");
			exit(-1);
		}
		write(fil2,&tileng[0],30*4);
		for(j=0;j<(numlevels<<1);j++)
		{
			if ((j&1) == 0)
				printf("%d ",j>>1);
			read(fil1,&buf1[0],4096);
			for(i=0;i<256;i++)
			{
				value[i] = i;
				child[i] = 65535;
				sibly[i] = (i+1)&255;
			}
			addrcnt = 256;
			bytecnt1 = 0;
			bitcnt = 0;
			bytecnt2 = 0;
			dat = 0;
			numbits = 9;
			do
			{
				addr = buf1[bytecnt1];
				do
				{
					bytecnt1++;
					if (bytecnt1 == 4096)
						break;
					if (child[addr] == 65535)
					{
						child[addr] = addrcnt;
						break;
					}
					newaddr = child[addr];
					while (value[newaddr] != buf1[bytecnt1])
					{
						if (sibly[newaddr] == 65535)
						{
							sibly[newaddr] = addrcnt;
							break;
						}
						newaddr = sibly[newaddr];
					}
					if (sibly[newaddr] == addrcnt)
						break;
					addr = newaddr;
				}
				while (addr >= 0);
				value[addrcnt] = buf1[bytecnt1];
				child[addrcnt] = 65535;
				sibly[addrcnt] = 65535;
				for(i=0;i<numbits;i++)
				{
					if ((addr&pow2[i]) > 0)
						dat += pow2[bitcnt];
					bitcnt++;
					if (bitcnt == 8)
					{
						bitcnt = 0;
						buf2[bytecnt2] = dat;
						bytecnt2++;
						dat = 0;
					}
				}
				addrcnt++;
				if ((addrcnt == 512) || (addrcnt == 1024) || (addrcnt == 2048) || (addrcnt == 4096))
					numbits++;
			}
			while (bytecnt1 < 4096);
			for(i=0;i<numbits;i++)
			{
				if ((addr&pow2[i]) > 0)
					dat += pow2[bitcnt];
				bitcnt++;
				if (bitcnt == 8)
				{
					bitcnt = 0;
					buf2[bytecnt2] = dat;
					bytecnt2++;
					dat = 0;
				}
			}
			if (bytecnt2 < 4096)
			{
				write(fil2,&addrcnt,2);
				write(fil2,&buf2[0],bytecnt2);
			}
			else
			{
				addrcnt = 0;
				bytecnt2 = 4096;
				write(fil2,&addrcnt,2);
				write(fil2,&buf1[0],bytecnt2);
			}
			tileng[j] = bytecnt2;
		}
		lseek(fil2,0,SEEK_SET);
		write(fil2,&tileng[0],30*4);
		close(fil1);
		close(fil2);
	}

	//-----------------------------------------------------------------------

	if ((argc != 2) || (strcmp(filename,"story") == 0))
	{
		printf("\n");
		if ((fil1 = open("story.dat",O_BINARY|O_RDWR,S_IREAD)) == -1)
		{
			printf("Unable to open story.dat\n");
			exit(-1);
		}
		if ((fil2 = open("story.kzp",O_BINARY|O_TRUNC|O_CREAT|O_WRONLY,S_IWRITE)) == -1)
		{
			printf("Unable to create story.kzp\n");
			exit(-1);
		}
		write(fil2,&buf3[0],256);
		oldat = 0;
		cnt = 256;
		j = 1;
		buf3[0] = 256;
		leng = read(fil1,&dat,1);
		while (leng > 0)
		{
			if ((dat != 10) && (dat != 92))
			{
				dat ^= oldat, write(fil2,&dat,1), oldat = dat;
				dat = 0;
				cnt++;
			}
			if (dat == 92)
			{
				dat = oldat, write(fil2,&dat,1), oldat = 0;
				printf("%d ",j);
				cnt++;
				buf3[j++] = cnt;
				leng = read(fil1,&dat,1);
				leng = read(fil1,&dat,1);
			}
			leng = read(fil1,&dat,1);
		}
		lseek(fil2,0,SEEK_SET);
		write(fil2,&buf3[0],256);
		close(fil1);
		close(fil2);
	}

	//-----------------------------------------------------------------------

	if ((argc != 2) || (strcmp(filename,"ksm") == 0))
	{
		printf("\n");
		filecount = 0;
		if ((status = findfiles("*.KSM")) == -1)
		{
			printf("\nNo *.KSM files found!");
			exit(0);
		}
		if ((fil2 = open("songs.kzp",O_BINARY|O_TRUNC|O_CREAT|O_WRONLY,S_IWRITE)) == -1)
		{
			printf("Unable to create songs.kzp\n");
			exit(-1);
		}
		for(i=0;i<20000;i++)
			buf2[i] = 0;
		bytecnt = 2+12*((long)filecount);
		write(fil2,&buf2[0],((int)bytecnt));
		for(i=0;i<filecount;i++)
		{
			strcpy(buffer,filenames[i]);
			j = strlen(buffer);
			buffer[j+0] = '.';
			buffer[j+1] = 'K';
			buffer[j+2] = 'S';
			buffer[j+3] = 'M';
			buffer[j+4] = 0;
			if ((fil1 = open(buffer,O_BINARY|O_RDWR,S_IREAD)) == -1)
			{
				printf("Unable to open %s\n",buffer);
				exit(-1);
			}
			leng = read(fil1,&buf2[0],20000);
			close(fil1);
			offset[i] = bytecnt;
			bytecnt += ((long)leng);
			printf("%s ",buffer);
			write(fil2,&buf2[0],leng);
		}
		buf2[0] = (char)((filecount&255));
		buf2[1] = (char)((filecount>>8)&255);
		for(i=0;i<filecount;i++)
		{
			for(j=0;j<8;j++)
				buf2[2+i*12+j] = filenames[i][j];
			buf2[2+i*12+8] = (char)(offset[i]&255);
			buf2[2+i*12+9] = (char)((offset[i]>>8)&255);
			buf2[2+i*12+10] = (char)((offset[i]>>16)&255);
			buf2[2+i*12+11] = (char)((offset[i]>>24)&255);
		}
		lseek(fil2,0,SEEK_SET);
		write(fil2,&buf2[0],2+12*filecount);
		close(fil2);
	}

	//-----------------------------------------------------------------------

	if ((argc != 2) || (strcmp(filename,"wav") == 0))
	{
		printf("\n");
		filecount = 0;
		if ((status = findfiles("*.WAV")) == -1)
		{
			printf("\nNo *.WAV files found!");
			exit(0);
		}
		if ((fil2 = open("sounds.kzp",O_BINARY|O_TRUNC|O_CREAT|O_WRONLY,S_IWRITE)) == -1)
		{
			printf("Unable to create sounds.kzp\n");
			exit(-1);
		}
		for(i=0;i<20000;i++)
			buf2[i] = 0;
		bytecnt = 2+6*((long)filecount);
		write(fil2,&buf2[0],((int)bytecnt));
		for(i=0;i<filecount;i++)
		{
			strcpy(buffer,wavfiles[i]);
			j = strlen(buffer);
			buffer[j+0] = '.';
			buffer[j+1] = 'W';
			buffer[j+2] = 'A';
			buffer[j+3] = 'V';
			buffer[j+4] = 0;
			if ((fil1 = open(buffer,O_BINARY|O_RDWR,S_IREAD)) == -1)
			{
				printf("Unable to open %s\n",buffer);
				exit(-1);
			}
			offset[i] = bytecnt;
			leng = read(fil1,&buf2[0],20000);
			if (leng > 44)
			{
				bytecnt += ((long)leng-44);
				filength[i] = leng-44;
				write(fil2,&buf2[44],leng-44);
			}
			if (leng == 20000)
			{
				leng = read(fil1,&buf2[0],12768);
				bytecnt += ((long)leng);
				filength[i] += leng;
				write(fil2,&buf2[0],leng);
			}
			close(fil1);
			printf("%s\n",buffer);
		}
		buf2[0] = (char)((filecount&255));
		buf2[1] = (char)((filecount>>8)&255);
		for(i=0;i<filecount;i++)
		{
			buf2[2+i*6+0] = (char)(offset[i]&255);
			buf2[2+i*6+1] = (char)((offset[i]>>8)&255);
			buf2[2+i*6+2] = (char)((offset[i]>>16)&255);
			buf2[2+i*6+3] = (char)((offset[i]>>24)&255);
			buf2[2+i*6+4] = (char)(filength[i]&255);
			buf2[2+i*6+5] = (char)((filength[i]>>8)&255);
		}
		lseek(fil2,0,SEEK_SET);
		write(fil2,&buf2[0],2+6*filecount);
		close(fil2);
	}
}

findfiles(kind)
char kind[6];
{
	int i, count;
	struct find_t fileinfo;
	char buffer[80];

	if (_dos_findfirst(kind,_A_NORMAL,&fileinfo) == 0)
	{
		strcpy(buffer,fileinfo.name);
		i = 8;
		while (buffer[i] != '.')
			i--;
		buffer[i] = 0;
		for(i=0;i<9;i++)
			filenames[filecount][i] = buffer[i];
		filecount++;
		while (_dos_findnext(&fileinfo) == 0)
		{
			strcpy(buffer,fileinfo.name);
			i = 8;
			while ((buffer[i] != '.') && (i > 0))
				i--;
			buffer[i] = 0;
			for(i=0;i<9;i++)
				filenames[filecount][i] = buffer[i];
			filecount++;
		}
		return(0);
	}
	else
		return(-1);
}
