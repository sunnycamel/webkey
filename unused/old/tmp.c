/*
Copyright (C) 2010  Peter Mora, Zoltan Papp

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "suinput.h"
#include <stdio.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include "KeycodeLabels.h"
#include <vector>
#include "kcm.h"
#include <sys/mman.h>
#include <linux/fb.h>
#include "png.h"
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#include <limits.h>
#include "mongoose.h"

//#include <sys/wait.h>
//#include <unistd.h>
#include <linux/input.h>

#undef stderr
FILE *stderr = stderr;

#ifdef MYDEB
#define FB_DEVICE "/android/dev/graphics/fb0"
#else
#define FB_DEVICE "/dev/graphics/fb0"
#endif

static int exit_flag;
static int touchcount=0;
static std::string dir;
static int dirdepth = 0;
static std::string passfile;
static std::string admin_password;
struct mg_context       *ctx;

static void
signal_handler(int sig_num)
{
        if (sig_num == SIGCHLD)
                while (waitpid(-1, &sig_num, WNOHANG) > 0);
        else
                exit_flag = sig_num;
}


static int fbfd = -1;
static void *fbmmap = MAP_FAILED;
static int bytespp = 0;
static struct fb_var_screeninfo scrinfo;
static png_byte* pict = NULL;
static png_byte** graph = NULL;
static int touchfd = -1;
static __s32 xmin;
static __s32 xmax;
static __s32 ymin;
static __s32 ymax;
static int newsockfd = -1;

//base
static const char* KCM_BASE[] ={
"A 'A' '2' 'a' 'A'",
"B 'B' '2' 'b' 'B'",
"C 'C' '2' 'c' 'C'",
"D 'D' '3' 'd' 'D'",
"E 'E' '3' 'e' 'E'",
"F 'F' '3' 'f' 'F'",
"G 'G' '4' 'g' 'G'",
"H 'H' '4' 'h' 'H'",
"I 'I' '4' 'i' 'I'",
"J 'J' '5' 'j' 'J'",
"K 'K' '5' 'k' 'K'",
"L 'L' '5' 'l' 'L'",
"M 'M' '6' 'm' 'M'",
"N 'N' '6' 'n' 'N'",
"O 'O' '6' 'o' 'O'",
"P 'P' '7' 'p' 'P'",
"Q 'Q' '7' 'q' 'Q'",
"R 'R' '7' 'r' 'R'",
"S 'S' '7' 's' 'S'",
"T 'T' '8' 't' 'T'",
"U 'U' '8' 'u' 'U'",
"V 'V' '8' 'v' 'V'",
"W 'W' '9' 'w' 'W'",
"X 'X' '9' 'x' 'X'",
"Y 'Y' '9' 'y' 'Y'",
"Z 'Z' '9' 'z' 'Z'",
"COMMA ',' ',' ',' '?'",
"PERIOD '.' '.' '.' '/'",
"AT '@' 0x00 '@' '~'",
"SPACE 0x20 0x20 0x20 0x20",
"ENTER 0xa 0xa 0xa 0xa",
"0 '0' '0' '0' ')'",
"1 '1' '1' '1' '!'",
"2 '2' '2' '2' '@'",
"3 '3' '3' '3' '#'",
"4 '4' '4' '4' '$'",
"5 '5' '5' '5' '%'",
"6 '6' '6' '6' '^'",
"7 '7' '7' '7' '&'",
"8 '8' '8' '8' '*'",
"9 '9' '9' '9' '('",
"TAB 0x9 0x9 0x9 0x9",
"GRAVE '`' '`' '`' '~'",
"MINUS '-' '-' '-' '_'",
"EQUALS '=' '=' '=' '+'",
"LEFT_BRACKET '[' '[' '[' '{'",
"RIGHT_BRACKET ']' ']' ']' '}'",
"BACKSLASH '\\' '\\' '\\' '|'",
"SEMICOLON ';' ';' ';' ':'",
"APOSTROPHE '\'' '\'' '\'' '\"'",
"STAR '*' '*' '*' '<'",
"POUND '#' '#' '#' '>'",
"PLUS '+' '+' '+' '+'",
"SLASH '/' '/' '/' '?'"
};

//  ,!,",#,$,%,&,',(,),*,+,,,-,.,/
int spec1[] = {62,8,75,10,11,12,14,75,16,7,15,70,55,69,56,56,52};                      
int spec1sh[] = {0,1,1,1,1,1,1,0,1,1,1,1,0,0,0,1};
// :,;,<,=,>,?,@
int spec2[] = {74,74,17,70,18,55,77};
int spec2sh[] = {1,0,1,0,1,1,0};
// [,\,],^,_,`
int spec3[] = {71,73,72,13,69,68};
int spec3sh[] = {0,0,0,1,1,0};
// {,|,},~
int spec4[] = {71,73,72,68};
int spec4sh[] = {1,1,1,1,0};


struct BIND{
	int ajax;
	int disp;
	int kcm;
	bool kcm_sh;
	bool sms;
};

struct FAST{
	bool show;
	char* name;
	int  ajax;
};

static std::vector<BIND*> speckeys;
static std::vector<FAST*> fastkeys;
static int uinput_fd = -1;

bool startswith(char* st, char* patt)
{
	int i = 0;
	while(patt[i])
	{
		if (st[i] == 0 || st[i]-patt[i])
			return false;
		i++;
	}
	return true;
}

char* itoa(char* buff, int value)
{
	unsigned int i = 0;
	bool neg = false;
	if (value<0)
	{
		neg = true;
		value=-value;
	}
	if (value==0)
		buff[i++] = '0';
	else
		while(value)
		{
			buff[i++] = '0'+(char)(value%10);
			value = value/10;
		}
	if (neg)
		buff[i++] = '-';
	unsigned int j = 0;
	for(j = 0; j < (i>>1); j++)
	{
		char t = buff[j];
		buff[j] = buff[i-j-1];
		buff[i-j-1] = t;
	}
	buff[i++]=0;
	return buff;
}

int getnum(char* st)
{
	int r = 0;
	int i = 0;
	bool neg = false;
	if (st[i]=='-')
	{
		neg = true;
		i++;
	}
	while ('0'<=st[i] && '9'>=st[i])
	{
		r = r*10+(int)(st[i]-'0');
		i++;
	}
	if (neg)
		return -r;
	else
		return r;
}

void send_ok(struct mg_connection *conn)

{
	mg_printf(conn,"HTTP/1.1 200 OK\r\nCache-Control: no-store, no-cache, must-revalidate\r\nCache-Control: post-check=0, pre-check=0\r\nPragma: no-cache\r\n\r\n");
}
void clear()
{
	int i;
	for(i=0; i < speckeys.size(); i++)
		if (speckeys[i])
			delete speckeys[i];
	speckeys.clear();
	for(i=0; i < fastkeys.size(); i++)
		if (fastkeys[i])
		{
			delete[] fastkeys[i]->name;
			delete fastkeys[i];
		}
	fastkeys.clear();
	if (uinput_fd != -1)
		suinput_close(uinput_fd);
	if (pict)
		delete[] pict;
	pict = NULL;
	if (graph)
		delete[] graph;
	graph = NULL;
}

void error(const char *msg,const char *msg2 = NULL, const char *msg3=NULL, const char * msg4=NULL)
{
    perror(msg);
    if (msg2) perror(msg2);
    if (msg3) perror(msg3);
    if (msg4) perror(msg4);
    clear();
    exit(1);
}
FILE* fo(const char* filename, const char* mode)
{
	std::string longname = dir + filename;
	FILE* ret = fopen(longname.c_str(),mode);
	if (!ret)
		error("Couldn't open ",longname.c_str(), " with mode ", mode);
	else
		printf("%s is opened.\n",longname.c_str());
	return ret;
}



void init_uinput()
{
	clear();
	int i;
//	FILE* kl = fopen("/android/system/usr/keylayout/AndroControll.kl","w");
#ifdef MYDEB
	FILE* kl = fopen("/android/dev/AndroControll.kl","w");
#else
	FILE* kl = fopen("/dev/AndroControll.kl","w");
#endif
	if (!kl)
	{
		error("Couldn't open AndroControll.kl for writing.\n");
	}
	printf("AndroControll.kl is opened.\n");
	i = 0;
	while (KEYCODES[i].value)
	{
		fprintf(kl,"key %d   %s   WAKE\n",KEYCODES[i].value,KEYCODES[i].literal);
		FAST* load = new FAST;
		load->show = 0;
		load->name = new char[strlen(KEYCODES[i].literal)+1];
		strcpy(load->name,KEYCODES[i].literal);
		load->ajax = 0;
		fastkeys.push_back(load);
		i++;
	}
	fclose(kl);

	FILE* fk = fo("fast_keys.txt","r");
	int test;
	int ajax, show, id;
	while(fscanf(fk,"%d %d %d\n",&id,&show,&ajax) == 3)
	{
		if (id-1 < i)
		{
			fastkeys[id-1]->ajax = ajax;
			fastkeys[id-1]->show = show;
		}
	}
	fclose(fk);

	FILE* sk = fo("spec_keys.txt","r");
	int disp, sms;
	while(fscanf(sk,"%d %d %d\n",&sms,&ajax,&disp) == 3)
	{
		BIND* load = new BIND;
		load->ajax = ajax;
		load->disp = disp;
		load->sms = sms;
		speckeys.push_back(load);
	}
	fclose(sk);
	

//	FILE* kcm = fopen("/android/system/usr/keychars/AndroControll.kcm","w");
#ifdef MYDEB
	FILE* kcm = fopen("/android/dev/AndroControll.kcm","w");
#else
	FILE* kcm = fopen("/dev/AndroControll.kcm","w");
#endif
	if (!kcm)
	{
		error("Couldn't open AndroControll.kcm for writing.\n");
	}
	printf("AndroControll.kcm is opened.\n");
	fprintf(kcm,"[type=QWERTY]\n# keycode       display number  base    caps    alt     caps_alt\n");
	for(i=0; i<53; i++)
	{
		int k = 0;
		if (2*i < speckeys.size())
		{
			k = speckeys[2*i]->disp;
			speckeys[2*i]->kcm_sh = false;
			if (i<28)
				speckeys[2*i]->kcm = 29+i;
			if (28==i)
				speckeys[2*i]->kcm = 77;
			if (29==i)
				speckeys[2*i]->kcm = 62;
			if (30==i)
				speckeys[2*i]->kcm = 66;
			if (30<i && i<41)
				speckeys[2*i]->kcm = i-31+7;
			if (42==i)
				speckeys[2*i]->kcm = 61;
			//finish it....
		}
		int l = 0;
		if (2*i+1 < speckeys.size())
		{
			l = speckeys[2*i+1]->disp;
			speckeys[2*i+1]->kcm_sh = true;
			if (i<28)
				speckeys[2*i+1]->kcm = 29+i;
			if (28==i)
				speckeys[2*i+1]->kcm = 77;
			if (29==i)
				speckeys[2*i+1]->kcm = 62;
			if (30==i)
				speckeys[2*i+1]->kcm = 66;
			if (30<i && i<41)
				speckeys[2*i+1]->kcm = i-31+7;
			if (42==i)
				speckeys[2*i+1]->kcm = 61;
			//finish it....
		}
		fprintf(kcm,"%s %d %d\n",KCM_BASE[i],k,l);
	}
	fclose(kcm);
//	if (compile("/android/system/usr/keychars/AndroControll.kcm","/android/system/usr/keychars/AndroControll.kcm.bin"))
#ifdef MYDEB
	if (compile("/android/dev/AndroControll.kcm","/android/dev/AndroControll.kcm.bin"))
#else
	if (compile("/dev/AndroControll.kcm","/dev/AndroControll.kcm.bin"))
#endif
	{
		error("Couldn't compile kcm to kcm.bin\n");
	}
	printf("kcm.bin is compiled.\n");

	struct input_id uid = {
		0x06,//BUS_VIRTUAL, /* Bus type. */
		1, /* Vendor id. */
		1, /* Product id. */
		1 /* Version id. */
	};
//	uinput_fd = suinput_open("../../../sdcard/AndroControll", &uid);
	printf("suinput init...\n");
	std::string devname;
	for (i=0; i < dirdepth; i++)
		devname = devname + "../";
	devname = devname + "dev/AndroControll";
	uinput_fd = suinput_open(devname.c_str(), &uid);
}

//from android-vnc-server

static void init_fb(void)
{
        size_t pixels;

        pixels = scrinfo.xres * scrinfo.yres;
        bytespp = scrinfo.bits_per_pixel / 8;

        printf("xres=%d, yres=%d, xresv=%d, yresv=%d, xoffs=%d, yoffs=%d, bpp=%d\n",
          (int)scrinfo.xres, (int)scrinfo.yres,
          (int)scrinfo.xres_virtual, (int)scrinfo.yres_virtual,
          (int)scrinfo.xoffset, (int)scrinfo.yoffset,
          (int)scrinfo.bits_per_pixel);


        fbmmap = mmap(NULL, pixels * bytespp, PROT_READ, MAP_SHARED, fbfd, 0);

        if (fbmmap == MAP_FAILED)
        {
                perror("mmap");
                exit(EXIT_FAILURE);
        }
	pict  = new png_byte[scrinfo.yres*scrinfo.xres*3];
	if (scrinfo.yres > scrinfo.xres)	//orientation might be changed
		graph = new png_byte*[scrinfo.yres];
	else
		graph = new png_byte*[scrinfo.xres];
}

void init_touch()
{
	int i;
#ifdef MYDEB
	char touch_device[26] = "/android/dev/input/event0";
#else
	char touch_device[18] = "/dev/input/event0";
#endif
	for (i=0; i<10; i++)
	{
		char name[256]="Unknown";
		touch_device[sizeof(touch_device)-2] = '0'+(char)i;
		struct input_absinfo info;
		if((touchfd = open(touch_device, O_RDWR)) == -1)
		{
			continue;
		}
		printf("searching for touch device, opening %s ... ",touch_device);
		// Get the Range of X and Y
		if(ioctl(touchfd, EVIOCGABS(ABS_X), &info))
		{
			printf("failed\n");
			close(touchfd);
			touchfd = -1;
			continue;
		}
		xmin = info.minimum;
		xmax = info.maximum;
		if(ioctl(touchfd, EVIOCGABS(ABS_Y), &info)) {
			printf("failed\n");
			close(touchfd);
			touchfd = -1;
			continue;
		}
		if (xmin < 0 || xmin == xmax)	// xmin < 0 for the compass
		{
			printf("failed\n");
			close(touchfd);
			touchfd = -1;
			continue;
		}
		if (ioctl(touchfd, EVIOCGNAME(sizeof(name)),name) < 0)
		{
			printf("failed\n");
			close(touchfd);
			touchfd = -1;
			continue;
		}
		printf("success, device name is %s\n",name);
		ymin = info.minimum;
		ymax = info.maximum;
		printf("xmin = %d, xmax = %d, ymin = %d, ymax = %d\n",xmin,xmax,ymin,ymax);
		return;
	}
}

void update_image(int orient,int lowres)
{
	FILE            *fp;
	png_structp     png_ptr;
	png_infop       info_ptr;

	fp = fo("tmp.png", "wb");

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);
	png_init_io(png_ptr, fp);
	png_set_compression_level(png_ptr, 1);


	int rr = scrinfo.red.offset;
	int rl = 8-scrinfo.red.length;
	int gr = scrinfo.green.offset;
	int gl = 8-scrinfo.green.length;
	int br = scrinfo.blue.offset;
	int bl = 8-scrinfo.blue.length;
	int i = 0;
	int j;
	printf("rr=%d, rl=%d, gr=%d, gl=%d, br=%d, bl=%d\n",rr,rl,gr,gl,br,bl);
	
	if (orient == 0)
		png_set_IHDR(png_ptr, info_ptr, scrinfo.xres>>lowres, scrinfo.yres>>lowres,
			8,//  scrinfo.bits_per_pixel,
			PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	else
		png_set_IHDR(png_ptr, info_ptr, scrinfo.yres>>lowres, scrinfo.xres>>lowres,
			8,//  scrinfo.bits_per_pixel,
			PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	int x = scrinfo.xres;
	int y = scrinfo.yres;
	int m = scrinfo.yres*scrinfo.xres;
	int k;
	int p = 0;
	i = 0;
	if (lowres) // I use if outside the for, maybe it's faster this way
	{
		if (bytespp == 2) //16 bit
		{
			unsigned short int* map = (unsigned short int*)fbmmap;
			if (orient == 0) //vertical
			{
				for (j = 0; j < y; j+=2)
				{
					for (k = 0; k < x; k+=2)
					{
						pict[i++] = ((map[p]>>rr<<rl)+(map[p+1]>>rr<<rl)+(map[p+x]>>rr<<rl)+(map[p+x+1]>>rr<<rl)>>2);
						pict[i++] = ((map[p]>>gr<<gl)+(map[p+1]>>gr<<gl)+(map[p+x]>>gr<<gl)+(map[p+x+1]>>gr<<gl)>>2);
						pict[i++] = ((map[p]>>br<<bl)+(map[p+1]>>br<<bl)+(map[p+x]>>br<<bl)+(map[p+x+1]>>br<<bl)>>2);
						p+=2;
					}
					p+=x;
				}
			}
			else //horizontal
			{
				for (j = 0; j < x; j+=2)
				{
					p = (x-j-1);
					for (k = 0; k < y; k+=2)
					{
						pict[i++] = ((map[p]>>rr<<rl)+(map[p-1]>>rr<<rl)+(map[p+x]>>rr<<rl)+(map[p+x-1]>>rr<<rl)>>2);
						pict[i++] = ((map[p]>>gr<<gl)+(map[p-1]>>gr<<gl)+(map[p+x]>>gr<<gl)+(map[p+x-1]>>gr<<gl)>>2);
						pict[i++] = ((map[p]>>br<<bl)+(map[p-1]>>br<<bl)+(map[p+x]>>br<<bl)+(map[p+x-1]>>br<<bl)>>2);
						p += 2*x;
					}
				}
			}
		}
		if (bytespp == 4) //32 bit
		{
			unsigned int* map = (unsigned int*)fbmmap;
			if (orient == 0) //vertical
			{
				for (j = 0; j < y; j+=2)
				{
					for (k = 0; k < x; k+=2)
					{
						pict[i++] = ((map[p]>>rr<<rl)+(map[p+1]>>rr<<rl)+(map[p+x]>>rr<<rl)+(map[p+x+1]>>rr<<rl)>>2);
						pict[i++] = ((map[p]>>gr<<gl)+(map[p+1]>>gr<<gl)+(map[p+x]>>gr<<gl)+(map[p+x+1]>>gr<<gl)>>2);
						pict[i++] = ((map[p]>>br<<bl)+(map[p+1]>>br<<bl)+(map[p+x]>>br<<bl)+(map[p+x+1]>>br<<bl)>>2);
						p+=2;
					}
					p+=x;
				}
			}
			else //horizontal
			{
				for (j = 0; j < x; j+=2)
				{
					p = (x-j-1);
					for (k = 0; k < y; k+=2)
					{
						pict[i++] = ((map[p]>>rr<<rl)+(map[p-1]>>rr<<rl)+(map[p+x]>>rr<<rl)+(map[p+x-1]>>rr<<rl)>>2);
						pict[i++] = ((map[p]>>gr<<gl)+(map[p-1]>>gr<<gl)+(map[p+x]>>gr<<gl)+(map[p+x-1]>>gr<<gl)>>2);
						pict[i++] = ((map[p]>>br<<bl)+(map[p-1]>>br<<bl)+(map[p+x]>>br<<bl)+(map[p+x-1]>>br<<bl)>>2);
						p += 2*x;
					}
				}
			}
		}
	}
	else //hires
	{
		if (bytespp == 2)
		{
			unsigned short int* map = (unsigned short int*)fbmmap;
			if (orient == 0) //vertical
			{
				for (j = 0; j < m; j++)
				{
					pict[i++] = map[j]>>rr<<rl;
					pict[i++] = map[j]>>gr<<gl;
					pict[i++] = map[j]>>br<<bl;
				}
			}
			else //horizontal
			{
				for (j = 0; j < x; j++)
				{
					p = (x-j-1);
					for (k = 0; k < y; k++)
					{
						pict[i++] = map[p]>>rr<<rl;
						pict[i++] = map[p]>>gr<<gl;
						pict[i++] = map[p]>>br<<bl;
						p += x;
					}
				}
			}
		}
		if (bytespp == 4)
		{
			unsigned int* map = (unsigned int*)fbmmap;
			if (orient == 0) //vertical
			{
				for (j = 0; j < m; j++)
				{
					pict[i++] = map[j]>>rr<<rl;
					pict[i++] = map[j]>>gr<<gl;
					pict[i++] = map[j]>>br<<bl;
				}
			}
			else //horizontal
			{
				for (j = 0; j < x; j++)
				{
					p = (x-j-1);
					for (k = 0; k < y; k++)
					{
						pict[i++] = map[p]>>rr<<rl;
						pict[i++] = map[p]>>gr<<gl;
						pict[i++] = map[p]>>br<<bl;
						p += x;
					}
				}
			}
		}
	}
	if (orient == 0)
	{
		for (i = 0; i < y>>lowres; i++)
			graph[i] = pict+(i*x*3>>lowres);
	}	
	else
	{
		for (i = 0; i < x>>lowres; i++)
			graph[i] = pict+(i*y*3>>lowres);
	}

	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, graph);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
}

static void
touch(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	int n = strlen(ri->uri);
       	if (n<8)
		return;
	int orient = 0;
	if (ri->uri[7]=='h')
		orient = 1;
	int i = 8;
	int x = getnum(ri->uri+i);
	while (i<n && ri->uri[i++]!='_');
	int y = getnum(ri->uri+i);
	while (i<n && ri->uri[i++]!='_');
	int down = getnum(ri->uri+i);
	if(touchfd != -1 && scrinfo.xres && scrinfo.yres)
	{
		struct input_event ev;

		if (orient)
		{
			int t = x;
			x = scrinfo.xres-y;
			y = t;
		}
		if (x < 0) x = 0;
		if (y < 0) y = 0;
		if (x > scrinfo.xres) x = scrinfo.xres;
		if (y > scrinfo.yres) y = scrinfo.yres;
		// Calculate the final x and y
		x = xmin + (x * (xmax - xmin)) / (scrinfo.xres);
		y = ymin + (y * (ymax - ymin)) / (scrinfo.yres);

		memset(&ev, 0, sizeof(ev));

		// Then send a BTN_TOUCH
		if (down != 2)
		{
			gettimeofday(&ev.time,0);
			ev.type = EV_KEY;
			ev.code = BTN_TOUCH;
			ev.value = down;
			if(write(touchfd, &ev, sizeof(ev)) < 0)
				printf("touchfd write failed.\n");
		}
		gettimeofday(&ev.time,0);
		ev.type = EV_ABS;
		ev.code = 48;
		ev.value = down>0?100:0;
		if(write(touchfd, &ev, sizeof(ev)) < 0)
			printf("touchfd write failed.\n");
		// Then send the X
		gettimeofday(&ev.time,0);
		ev.type = EV_ABS;
		ev.code = ABS_X;
		ev.value = x;
		if(write(touchfd, &ev, sizeof(ev)) < 0)
			printf("touchfd write failed.\n");
		// Then send the X
		gettimeofday(&ev.time,0);
		ev.type = EV_ABS;
		ev.code = 53;//ABS_X;
		ev.value = x;
		if(write(touchfd, &ev, sizeof(ev)) < 0)
			printf("touchfd write failed.\n");

		// Then send the Y
		gettimeofday(&ev.time,0);
		ev.type = EV_ABS;
		ev.code = ABS_Y;
		ev.value = y;
		if(write(touchfd, &ev, sizeof(ev)) < 0)
			printf("touchfd write failed.\n");
		// Then send the Y
		gettimeofday(&ev.time,0);
		ev.type = EV_ABS;
		ev.code = 54;//ABS_Y;
		ev.value = y;
		if(write(touchfd, &ev, sizeof(ev)) < 0)
			printf("touchfd write failed.\n");

		// Finally send the SYN
		gettimeofday(&ev.time,0);
		ev.type = EV_SYN;
		ev.code = 2;
		ev.value = 0;
		if(write(touchfd, &ev, sizeof(ev)) < 0)
			printf("touchfd write failed.\n");
		gettimeofday(&ev.time,0);
		ev.type = EV_SYN;
		ev.code = 0;
		ev.value = 0;
		if(write(touchfd, &ev, sizeof(ev)) < 0)
			printf("touchfd write failed.\n");

		//printf("%d. injectTouch x=%d, y=%d, down=%d\n", touchcount++, x, y, down);    

	}
	send_ok(conn);
}
static void
key(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	bool sms_mode = false;
	int key = 0;
	bool old = false;
	int orient = 0;
	printf("%s\n",ri->uri);
		// /oldkey_hn-22 -> -22 key with normal mode, horisontal
	int n = 0;
	if (startswith(ri->uri,"/oldkey") && strlen(ri->uri)>10) 
	{
		n = 9;
		old = true;
	}
	else
	if (strlen(ri->uri)>6)
	{
		n = 6;
	}
	else
		return;
	if (ri->uri[n-2] == 'h')
		orient = 1;
	if (ri->uri[n-1] == 's')
		sms_mode = true;
	if (ri->uri[++n] == 0)
		return;
	send_ok(conn);
	while(1)
	{
		key = getnum(ri->uri+n);
		if (old && key < 0)
			key = -key;
		if (key == 0) // I don't know how, but it happens
		{
			send_ok(conn);
			return;
		}
		printf("KEY %d\n",key);
		int i;
		if (sms_mode)
			for (i=0; i < speckeys.size(); i++)
				if ((speckeys[i]->ajax == key || (old && speckeys[i]->ajax == -key)) && speckeys[i]->sms)
				{
					printf("pressed: %d\n",speckeys[i]->kcm);
					suinput_press(uinput_fd, 57); //left alt
					if (speckeys[i]->kcm_sh)
						suinput_press(uinput_fd, 59); //left shift
					suinput_click(uinput_fd, speckeys[i]->kcm);
					suinput_release(uinput_fd, 57); //left alt
					if (speckeys[i]->kcm_sh)
						suinput_release(uinput_fd, 59); //left shift
					break;
				}
		if (!sms_mode || i == speckeys.size())
			for (i=0; i < speckeys.size(); i++)
				if ((speckeys[i]->ajax == key|| (old && speckeys[i]->ajax == -key) ) && speckeys[i]->sms == false)
				{
					printf("pressed: %d\n",speckeys[i]->kcm);
					suinput_press(uinput_fd, 57); //left alt
					if (speckeys[i]->kcm_sh)
						suinput_press(uinput_fd, 59); //left shift
					suinput_click(uinput_fd, speckeys[i]->kcm);
					suinput_release(uinput_fd, 57); //left alt
					if (speckeys[i]->kcm_sh)
						suinput_release(uinput_fd, 59); //left shift
				}
		for (i=0; i < fastkeys.size(); i++)
			if (fastkeys[i]->ajax == key || (old && fastkeys[i]->ajax == -key))
			{
				int k = i+1;
				if (orient)
				{
					if (k == 19) //up -> right
						k = 22;
					else if (k == 20) //down -> left
						k = 21;
					else if (k == 21) //left -> up
						k = 19;
					else if (k == 22) //right -> down
						k = 20;
				}
				suinput_click(uinput_fd, k);
			}
		if (48<=key && key < 58)
			suinput_click(uinput_fd, key-48+7);
		if (97<=key && key < 123)
			suinput_click(uinput_fd, key-97+29);
		if (65<=key && key < 91)
		{
			suinput_press(uinput_fd, 59); //left shift
			suinput_click(uinput_fd, key-65+29);
			suinput_release(uinput_fd, 59); //left shift
		}
		if (32<=key && key <= 47)
		{
			if (spec1sh[key-32]) suinput_press(uinput_fd, 59); //left shift
			suinput_click(uinput_fd, spec1[key-32]);
			if (spec1sh[key-32]) suinput_release(uinput_fd, 59); //left shift
		}
		if (58<=key && key <= 64)
		{
			if (spec2sh[key-58]) suinput_press(uinput_fd, 59); //left shift
			suinput_click(uinput_fd, spec2[key-58]);
			if (spec2sh[key-58]) suinput_release(uinput_fd, 59); //left shift
		}
		if (91<=key && key <= 96)
		{
			if (spec3sh[key-91]) suinput_press(uinput_fd, 59); //left shift
			suinput_click(uinput_fd, spec3[key-91]);
			if (spec3sh[key-91]) suinput_release(uinput_fd, 59); //left shift
		}
		if (123<=key && key <= 127)
		{
			if (spec4sh[key-123]) suinput_press(uinput_fd, 59); //left shift
			suinput_click(uinput_fd, spec4[key-123]);
			if (spec4sh[key-123]) suinput_release(uinput_fd, 59); //left shift
		}
		if (key == -8 || (old && key == 8)) suinput_click(uinput_fd, 67); //BACKSPACE -> DEL
		if (key == -13 || (old && key == 13)) suinput_click(uinput_fd, 66); //ENTER
		while (ri->uri[n] != '_')
		{
		       if (ri->uri[++n] == 0)
			       return;
		}
		if (ri->uri[++n] == 0)
			return;
		usleep(1000);
	}

}


static void
savebuttons(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	int i;
	for (i=0; i < fastkeys.size(); i++)
		fastkeys[i]->show = false;
	int n = ri->post_data_len;
	i = 0;
	while(i<n)
	{
		if (startswith(ri->post_data+i,"show_"))
		{
			i+=5;
			int id = getnum(ri->post_data+i);
			if (id-1<fastkeys.size())
				fastkeys[id-1]->show = true;
			printf("SHOW %d\n",id-1);
		}
		else if (startswith(ri->post_data+i,"keycode_"))
		{
			i+=8;
			int id = getnum(ri->post_data+i);
			while (i<n && ri->post_data[i++]!='=');
			int ajax = getnum(ri->post_data+i);
			if (id-1<fastkeys.size())
				fastkeys[id-1]->ajax = ajax;
		}
		while (i<n && ri->post_data[i++]!='&');
	}

	FILE* fk = fo("fast_keys.txt","w");
	for (i=0;i<fastkeys.size();i++)
	{
		fprintf(fk,"%d %d %d\n",i+1,fastkeys[i]->show,fastkeys[i]->ajax);
	}
	fclose(fk);
	mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n<html><head><meta http-equiv=\"refresh\" content=\"0;url=/\"></head><body>redirecting</body></html>");

}
static void
savekeys(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	int i;
	int n = ri->post_data_len;
	for (i=0; i < speckeys.size(); i++)
		if (speckeys[i])
			delete speckeys[i];
	speckeys.clear();
	for (i=0; i < 106; i++)
	{
		BIND* load = new BIND;
		load->ajax = load->disp = load->sms = 0;
		speckeys.push_back(load);
	}
	i = 0;
	while(i<n)
	{
		if (startswith(ri->post_data+i,"sms_"))
		{
			i+=4;
			int id = getnum(ri->post_data+i);
			if (id-1<speckeys.size())
				speckeys[id-1]->sms = true;
		}
		else if (startswith(ri->post_data+i,"keycode_"))
		{
			i+=8;
			int id = getnum(ri->post_data+i);
			while (i<n && ri->post_data[i++]!='=');
			int ajax = getnum(ri->post_data+i);
			if (id-1<speckeys.size())
				speckeys[id-1]->ajax = ajax;
		}
		else if (startswith(ri->post_data+i,"tokeycode_"))
		{
			i+=10;
			int id = getnum(ri->post_data+i);
			while (i<n && ri->post_data[i++]!='=');
			int disp = getnum(ri->post_data+i);
			if (id-1<speckeys.size())
				speckeys[id-1]->disp = disp;
		}
		while (i<n && ri->post_data[i++]!='&');
	}

	FILE* sk = fo("spec_keys.txt","w");
	for(i=0; i <speckeys.size(); i++)
		if (speckeys[i]->ajax || speckeys[i]->disp || speckeys[i]->disp)
			fprintf(sk,"%d %d %d\n",speckeys[i]->sms,speckeys[i]->ajax,speckeys[i]->disp);
	fclose(sk);
	init_uinput();
	mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n<html><head><meta http-equiv=\"refresh\" content=\"0;url=/\"></head><body>redirecting</body></html>");
}

static void
helppng(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	send_ok(conn);
	FILE* f = fo("help.png","rb");
	fseek (f , 0 , SEEK_END);
	int lSize = ftell (f);
	rewind (f);
	char* filebuffer = new char[lSize+1];
	fread(filebuffer,1,lSize,f);
	filebuffer[lSize] = 0;
	mg_write(conn,filebuffer,lSize);
	fclose(f);
}
static void
helphtml(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	send_ok(conn);
	FILE* f = fo("help.html","rb");
	fseek (f , 0 , SEEK_END);
	int lSize = ftell (f);
	rewind (f);
	char* filebuffer = new char[lSize+1];
	fread(filebuffer,1,lSize,f);
	filebuffer[lSize] = 0;
	mg_write(conn,filebuffer,lSize);
	fclose(f);
}
static void
index(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	send_ok(conn);
	FILE* f = fo("index.html","rb");
	fseek (f , 0 , SEEK_END);
	int lSize = ftell (f);
	rewind (f);
	char* filebuffer = new char[lSize+1];
	fread(filebuffer,1,lSize,f);
	filebuffer[lSize] = 0;
	mg_write(conn,filebuffer,lSize);
	fclose(f);
	int i;
	for (i=0; i < fastkeys.size(); i++)
		if (fastkeys[i]->show)
		{
			mg_printf(conn,"<input type=\"button\" class=\"butt\" value=\"%s\" onclick=\"makeRequest('button_%d','')\"/>",fastkeys[i]->name,i+1);
		}
	mg_printf(conn,"</div><div id=\"res\">results</div></div><script type=\"text/javascript\" language=\"javascript\">document.images.screenshot.onmousemove=move; function setWidth(orient){ if (orient == 1) document.images.screenshot.width=\"%d\"; else document.images.screenshot.width=\"%d\";}</script></body></html>",scrinfo.yres,scrinfo.xres);
	delete[] filebuffer;
}
static void
button(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	int key = getnum(ri->uri+8);
	printf("button %d\n",key);
	suinput_click(uinput_fd, key);
	send_ok(conn);
}
static void
config_buttons(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	FILE* f = fo("config_buttons.html","rb");
	fseek (f , 0 , SEEK_END);
	printf("file fseeked\n");
	int lSize = ftell (f);
	printf("file ftold, size=%d\n",lSize);
	rewind (f);
	printf("file rewinded\n");
	char* filebuffer = new char[lSize+1];
	fread(filebuffer,1,lSize,f);
	printf("file read\n");
	filebuffer[lSize] = 0;
	mg_write(conn,filebuffer,lSize);
	printf("file sent\n");
	fclose(f);
	printf("file closed\n");
	mg_printf(conn,"<table border=\"1\">\n");
	int i;
	for (i=0; i < fastkeys.size(); i++)
	{
		mg_printf(conn, "<tr><td>%s</td><td><input type=\"checkbox\" name=\"show_%d\">Show</input></td><td>Key: <input type=\"text\" name=\"key_%d\" maxlength=\"1\" size=\"1\" onkeypress=\"var unicode=event.charCode? event.charCode : -event.keyCode;document.config_buttons.keycode_%d.value=unicode; document.config_buttons.key_%d.value=String.fromCharCode(document.config_buttons.keycode_%d.value)\"/></td><td>Keycode: <input type=\"text\" name=\"keycode_%d\" maxlength=\"8\" size=\"8\" onkeyup=\"document.config_buttons.key_%d.value=String.fromCharCode(document.config_buttons.keycode_%d.value)\"/></td></tr>\n",fastkeys[i]->name,i+1,i+1,i+1,i+1,i+1,i+1,i+1,i+1);
	}
	mg_printf(conn,"</table></form>\n<script type=\"text/javascript\" language=\"javascript\">");

	for (i=0; i < fastkeys.size(); i++)
	{
		mg_printf(conn,"document.config_buttons.key_%d.value=String.fromCharCode(%d);document.config_buttons.keycode_%d.value='%d';document.config_buttons.show_%d",i+1,fastkeys[i]->ajax,i+1,fastkeys[i]->ajax,i+1);
		if (fastkeys[i]->show)
			mg_printf(conn,".checked='checked';\n");
		else
			mg_printf(conn,".checked='';\n");
	}
	mg_printf(conn,"</script></body></html>");

	delete[] filebuffer;
}
static void
config_keys(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	FILE* f = fo("config_keys.html","rb");
	fseek (f , 0 , SEEK_END);
	printf("file fseeked\n");
	int lSize = ftell (f);
	printf("file ftold, size=%d\n",lSize);
	rewind (f);
	printf("file rewinded\n");
	char* filebuffer = new char[lSize+1];
	fread(filebuffer,1,lSize,f);
	printf("file read\n");
	filebuffer[lSize] = 0;
	mg_write(conn,filebuffer,lSize);
	printf("file sent\n");
	fclose(f);
	printf("file closed\n");
	mg_printf(conn,"<table border=\"1\">\n");
	int i;
	for (i=0; i < 106; i++)
	{
		mg_printf(conn,"<tr><td>Char: <input type=\"text\" name=\"key_%d\" maxlength=\"1\" size=\"1\" onkeypress=\"var unicode=event.charCode? event.charCode : event.keyCode;document.config_keys.keycode_%d.value=unicode; document.config_keys.key_%d.value=String.fromCharCode(document.config_keys.keycode_%d.value)\"/></td><td>Keycode: <input type=\"text\" name=\"keycode_%d\" maxlength=\"8\" size=\"8\" onkeyup=\"document.config_keys.key_%d.value=String.fromCharCode(document.config_keys.keycode_%d.value)\"/></td><td> converts to </td><td>Char: <input type=\"text\" name=\"tokey_%d\" maxlength=\"1\" size=\"1\" onkeypress=\"var unicode=event.charCode? event.charCode : event.keyCode;document.config_keys.tokeycode_%d.value=unicode; document.config_keys.tokey_%d.value=String.fromCharCode(document.config_keys.tokeycode_%d.value)\"/></td><td>Keycode: <input type=\"text\" name=\"tokeycode_%d\" maxlength=\"8\" size=\"8\" onkeyup=\"document.config_keys.tokey_%d.value=String.fromCharCode(document.config_keys.tokeycode_%d.value)\"/></td><td><input type=\"checkbox\" name=\"sms_%d\">works in SMS mode</input></td></tr>\n",i+1,i+1,i+1,i+1,i+1,i+1,i+1,i+1,i+1,i+1,i+1,i+1,i+1,i+1,i+1);
	}
	mg_printf(conn,"</table></form>\n<script type=\"text/javascript\" language=\"javascript\">\n");

	for (i=0; i < speckeys.size(); i++)
	{
		mg_printf(conn,"document.config_keys.key_%d.value=String.fromCharCode(%d);document.config_keys.keycode_%d.value='%d';document.config_keys.tokey_%d.value=String.fromCharCode(%d);document.config_keys.tokeycode_%d.value='%d';document.config_keys.sms_%d",i+1,speckeys[i]->ajax,i+1,speckeys[i]->ajax,i+1,speckeys[i]->disp,i+1,speckeys[i]->disp,i+1);
		if (speckeys[i]->sms)
			mg_printf(conn,".checked='checked';\n");
		else
			mg_printf(conn,".checked='';\n");
	}
	mg_printf(conn,"</script></body></html>");

	delete[] filebuffer;
}
static void
config(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
        int n = ri->post_data_len;
	int i = 0;
	char name[256];
	char pass[256];
	bool changed = false;
	int j = 0;
	while(i < n)
	{
		printf("%s\n",ri->post_data);
		if (!memcmp(ri->post_data+i, "username",8))
		{	
			i+=9;
			j = 0;
			while(i<n && ri->post_data[i] != '&' && j<255)
				name[j++] = ri->post_data[i++];		
			name[j] = 0; i++;
		}
		else if (!memcmp(ri->post_data+i, "password",8))
		{
			i+=9;
			int k = 0;
			while(i<n && ri->post_data[i] != '&' && k<255)
				pass[k++] = ri->post_data[i++];		
			pass[k] = 0; i++;
			if (j && k)
			{
				mg_modify_passwords_file(ctx, (dir+passfile).c_str(), name, pass);
				changed = true;
			}
		}
		else if (!memcmp(ri->post_data+i, "remove",6))
		{
			i=n;
			if (j)
			{
				mg_modify_passwords_file(ctx, (dir+passfile).c_str(), name, "");
				changed = true;
			}
		}
		else
			i++;
	}
	if (changed)
	{
		mg_printf(conn,"HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n<html><head><meta http-equiv=\"refresh\" content=\"0;url=/config\"></head><body>redirecting</body></html>");
		return;
	}
	send_ok(conn);
	mg_printf(conn,"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n<html>\n<head>\n<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"><style>.menu a{color: #000000; font-size: 18px; text-decoration: none; padding-right: 20px; padding-left: 20px;} </style>\n<title>WebKey for Android</title>\n</head>\n");
	mg_printf(conn,"<body><div class=\"menu\" style=\"background-color: #eeaa44; padding-top: 8px; padding-bottom: 8px; text-align: center;\"><a href=\"/\">phone</a> <a href=\"/config_buttons.html\">configure buttons</a> <a href=\"/config_keys.html\">configure keys</a><a href=\"/config\">users</a> <a href=\"/sdcard\">sdcard</a><a href=\"/help.html\">help</a></div>Users (empty passwords are not allowed):");
	char line[256]; char domain[256];
	FILE* fp = fo(passfile.c_str(),"r");
	while (fgets(line, sizeof(line), fp) != NULL) 
	{
		if (sscanf(line, "%[^:]:%[^:]:%s", name, domain, pass) != 3)
			continue;
		mg_printf(conn,"<form name=\"%s_form\" method=\"post\">username: <input type=\"text\" readonly=\"readonly\" value=\"%s\" name=\"username\"> password: <input type=\"password\" name=\"password\"></input><input type=\"submit\" value=\"Change password\"></input><input name=\"remove\" type=\"submit\" value=\"Remove\"></input>",name,name,name);
		if (!strcmp(name,"admin"))
			mg_printf(conn,"This password will change on every restart\n");
		mg_printf(conn,"</form>\n");
	}
	mg_printf(conn,"<form name=\"newuser\" method=\"post\">New user:<input type=\"text\" name=\"username\"> password: <input type=\"password\" name=\"password\"></input><input type=\"submit\" value=\"Create\"></input></form>\n");
	fclose(fp);
	mg_printf(conn,"</body></html>");
}
static void
screenshot(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	int orient = 0;
	if (ri->uri[16] == 'h') // horizontal
		orient = 1;
	int lowres = 0;
	if (ri->uri[17] == 'l') // low res
		lowres = 1;
	if (!pict)
		init_fb();
	update_image(orient,lowres);
	send_ok(conn);
	FILE* f = fo("tmp.png","rb");
	fseek (f , 0 , SEEK_END);
	int lSize = ftell (f);
	rewind (f);
	char* filebuffer = new char[lSize+1];
	if(!filebuffer)
	{
		error("not enough memory for loading tmp.png\n");
	}
	fread(filebuffer,1,lSize,f);
	filebuffer[lSize] = 0;
	printf("sent bytes = %d\n",mg_write(conn,filebuffer,lSize));
	fclose(f);
	delete[] filebuffer;

}
static void
stop(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	send_ok(conn);
	if (ri->remote_ip==2130706433) //localhost
	{
		printf("Stopping server...\n");
		exit_flag = 2;
		mg_printf(conn,"Goodbye.\n");
	}
}
static void
run(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	send_ok(conn);
	int n = 4;
	std::string call = "";
        while (ri->uri[++n])
	{
		int k = getnum(ri->uri+n);
		while(ri->uri[n] && ri->uri[n] != '_') n++;
		if (k<=0 || 255<k)
			continue;
		call += (char)k;
	}
	call += " 2>&1";
	FILE* in;
	printf("%s\n",call.c_str());
	if ((in = popen(call.c_str(),"r")) == NULL)
		return;
	char buff[256];
	bool empty = true;
	while (fgets(buff, sizeof(buff), in) != NULL)
	{
		mg_printf(conn, "%s",buff);
		empty = false;
	}
	if (empty)
		mg_printf(conn,"</pre>empty<pre>");
	pclose(in);
}
static void
intent(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	send_ok(conn);
	int n = 7;
	std::string call = "/system/bin/am start ";
        while (ri->uri[++n])
	{
		int k = getnum(ri->uri+n);
		while(ri->uri[n] && ri->uri[n] != '_') n++;
		if (k<=0 || 255<k)
			continue;
		char ch = (char)k;
		if ((ch >= 'a' && ch <= 'z') ||
			(ch >= 'A' && ch <= 'Z') || ch == ' ')
			call += ch;
		else
		{
			call += '\\';
			call += ch;
		}
	}
	printf("%s\n",ri->uri+8);
	printf("%s\n",call.c_str());
		
	system(call.c_str());
}
static void
password(struct mg_connection *conn,
                const struct mg_request_info *ri, void *data)
{
	send_ok(conn);
	if (ri->remote_ip==2130706433) //localhost
	{
		mg_printf(conn,admin_password.c_str());
	}
}
int main(int argc, char **argv)
{

	int port;
	if (argc == 2)
		port = strtol (argv[1], 0, 10);
	else
		port = 80;
	if (port <= 0)
		error("Invalid port\n");
	int i; 
	for (i = strlen(argv[0])-1; i>=0; i--)
		if (argv[0][i] == '/')
		{
			argv[0][i+1]=0;
			break;
		}
	dir = argv[0];
	dirdepth = -1;
	for (i = 0; i < strlen(argv[0]); i++)
		if (argv[0][i] == '/')
			dirdepth++;
	printf("%d\n",dirdepth);
        if ((fbfd = open(FB_DEVICE, O_RDONLY)) == -1)
        {
                error("open framebuffer\n");
        }

        if (ioctl(fbfd, FBIOGET_VSCREENINFO, &scrinfo) != 0)
        {
                error("error reading screeninfo\n");
        }

	char buffer[8192];
        (void) signal(SIGCHLD, signal_handler);
        (void) signal(SIGTERM, signal_handler);
        (void) signal(SIGINT, signal_handler);
        if ((ctx = mg_start()) == NULL) {
                error("Cannot initialize Mongoose context");
        }
	
	itoa(buffer,port);
        if (mg_set_option(ctx, "ports", buffer) != 1)
                error("Error in configurate port.\n");
	mg_set_option(ctx, "aliases", "/sdcard=/sdcard");
	passfile = "passwords.txt";
        mg_set_option(ctx, "auth_realm", "webkey");
	
	admin_password = "";
	struct timeval tv;
	gettimeofday(&tv,0);
	srand ( time(NULL)+tv.tv_usec );
	for (i = 0; i < 8; i++)
	{
		char c[2]; c[1] = 0; c[0] = (char)(rand()%26+97);
		admin_password += c;
	}
	
	mg_modify_passwords_file(ctx, (dir+passfile).c_str(), "admin", admin_password.c_str());
        mg_set_uri_callback(ctx, "/", &index, NULL);
        mg_set_uri_callback(ctx, "/key*", &key, NULL);
        mg_set_uri_callback(ctx, "/oldkey*", &key, NULL);
        mg_set_uri_callback(ctx, "/touch*", &touch, NULL);
        mg_set_uri_callback(ctx, "/savebuttons", &savebuttons, NULL);
        mg_set_uri_callback(ctx, "/savekeys", &savekeys, NULL);
        mg_set_uri_callback(ctx, "/button*", &button, NULL);
        mg_set_uri_callback(ctx, "/config_buttons.html", &config_buttons, NULL);
        mg_set_uri_callback(ctx, "/config_keys.html", &config_keys, NULL);
        mg_set_uri_callback(ctx, "/screenshot.png*", &screenshot, NULL);
        mg_set_uri_callback(ctx, "/intent_*", &intent, NULL);
        mg_set_uri_callback(ctx, "/run_*", &run, NULL);
        mg_set_uri_callback(ctx, "/stop", &stop, NULL);
        mg_set_uri_callback(ctx, "/password", &password, NULL);
        mg_set_uri_callback(ctx, "/config", &config, NULL);
        mg_set_uri_callback(ctx, "/help.png", &helppng, NULL);
        mg_set_uri_callback(ctx, "/help.html", &helphtml, NULL);
	mg_set_option(ctx, "protect", (std::string("/password=,/stop=,/=")+dir+passfile).c_str());
//	mg_set_option(ctx, "protect", (std::string("/password=,/stop=,/config=")+dir+passfile).c_str());
	mg_set_option(ctx, "root","none");


        printf("WebKey %s started on port(s) [%s], serving directory [%s]\n",
            mg_version(),
            mg_get_option(ctx, "ports"),
            mg_get_option(ctx, "root"));

	printf("starting uinput...\n");
	init_uinput();
	printf("starting touch...\n");
	init_touch();
        fflush(stdout);
        while (exit_flag == 0)
                sleep(1);

        (void) printf("Exiting on signal %d, "
            "waiting for all threads to finish...", exit_flag);
        fflush(stdout);
        mg_stop(ctx);
        (void) printf("%s", " done.\n");

	clear();
        return (EXIT_SUCCESS);

  return 0;
}
