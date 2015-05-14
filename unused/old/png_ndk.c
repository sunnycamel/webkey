#include <stdio.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <netinet/in.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <png.h>
#include <errno.h>

#undef stderr
FILE *stderr = stderr;

#ifdef MYDEB
#define FB_DEVICE "/android/dev/graphics/fb0"
#else
#define FB_DEVICE "/dev/graphics/fb0"
#endif

static int fbfd = -1;
static unsigned short int *fbmmap = (unsigned short int*)MAP_FAILED;
static struct fb_var_screeninfo scrinfo;
static png_byte* pict = NULL;
static png_byte** graph = NULL;
static int touchfd = -1;
static __s32 xmin;
static __s32 xmax;
static __s32 ymin;
static __s32 ymax;
static int newsockfd = -1;
struct timeval oldt;

void log(const char *msg)
{
    struct timeval t;
    gettimeofday(&t,0);
    printf(msg);
    printf("Time spent: %d ms\n",(t.tv_sec-oldt.tv_sec)*1000+(t.tv_usec-oldt.tv_usec)/1000);
    oldt.tv_sec = t.tv_sec;
    oldt.tv_usec = t.tv_usec;
}

void error(const char *msg,const char *msg2 = NULL, const char *msg3=NULL, const char * msg4=NULL)
{
    perror(msg);
    if (msg2) perror(msg2);
    if (msg3) perror(msg3);
    if (msg4) perror(msg4);
    exit(1);
}
FILE* fo(const char* filename, const char* mode)
{
	FILE* ret = fopen(filename,mode);
	if (!ret)
		error("Couldn't open.\n");
	return ret;
}


//from android-vnc-server

static void init_fb(void)
{
        size_t pixels;
        size_t bytespp;

        if ((fbfd = open(FB_DEVICE, O_RDONLY)) == -1)
        {
                perror("open");
                exit(EXIT_FAILURE);
        }

        if (ioctl(fbfd, FBIOGET_VSCREENINFO, &scrinfo) != 0)
        {
                perror("ioctl");
                exit(EXIT_FAILURE);
        }

        pixels = scrinfo.xres * scrinfo.yres;
        bytespp = scrinfo.bits_per_pixel / 8;

        printf("xres=%d, yres=%d, xresv=%d, yresv=%d, xoffs=%d, yoffs=%d, bpp=%d\n",
          (int)scrinfo.xres, (int)scrinfo.yres,
          (int)scrinfo.xres_virtual, (int)scrinfo.yres_virtual,
          (int)scrinfo.xoffset, (int)scrinfo.yoffset,
          (int)scrinfo.bits_per_pixel);

        fbmmap = (short unsigned int *)mmap(NULL, pixels * bytespp, PROT_READ, MAP_SHARED, fbfd, 0);

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

void update_image(int orient, int compression)
{
	FILE            *fp;
	png_structp     png_ptr;
	png_infop       info_ptr;

	fp = fo("/dev/tmp.png", "wb");

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);
	png_init_io(png_ptr, fp);
	png_set_compression_level(png_ptr, compression);


	int rr = scrinfo.red.offset;
	int rl = 8-scrinfo.red.length;
	int gr = scrinfo.green.offset;
	int gl = 8-scrinfo.green.length;
	int br = scrinfo.blue.offset;
	int bl = 8-scrinfo.blue.length;
	int i = 0;
	int j;
	log("file is opened, ");
	if (orient == 0) //vertical
	{
		png_set_IHDR(png_ptr, info_ptr, scrinfo.xres, scrinfo.yres,
			8,//  scrinfo.bits_per_pixel,
			PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		for (j = 0; j < scrinfo.yres*scrinfo.xres; j++)
		{
			pict[i++] = fbmmap[j]>>rr<<rl;
			pict[i++] = fbmmap[j]>>gr<<gl;
			pict[i++] = fbmmap[j]>>br<<bl;
		}
		for (i = 0; i < scrinfo.yres; i++)
			graph[i] = pict+i*scrinfo.xres*3;
	}
	else //horizontal
	{
		png_set_IHDR(png_ptr, info_ptr, scrinfo.yres, scrinfo.xres,
			8,//  scrinfo.bits_per_pixel,
			PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		for (j = 0; j < scrinfo.xres; j++)
		{
			int k;
			int p = (scrinfo.xres-j-1);
			for (k = 0; k < scrinfo.yres; k++)
			{
				pict[i++] = fbmmap[p]>>rr<<rl;
				pict[i++] = fbmmap[p]>>gr<<gl;
				pict[i++] = fbmmap[p]>>br<<bl;
				p += scrinfo.xres;
			}
		}
		for (i = 0; i < scrinfo.xres; i++)
			graph[i] = pict+i*scrinfo.yres*3;
	}

	log("copied to memory, ");
	png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, graph);
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
	log("image was written, ");
}
int main(int argc, char **argv)
{
	int c;
	for (c=0; c < 7; c++)
	{
		printf("compression: %d\n",c);
		gettimeofday(&oldt,0);

		init_fb();
		log("init_fb done, ");
		update_image(0,c);
		FILE* f = fo("/dev/tmp.png","rb");
		fseek (f , 0 , SEEK_END);
		int lSize = ftell (f);
		printf("file ftold, size=%d\n",lSize);
		fclose(f);
		log("file was reopened, ");

		delete[] pict;
		delete[] graph;
		pict = NULL;
		graph = NULL;
		printf("------------------------------------------------------\n");
	}
  return 0;
}
