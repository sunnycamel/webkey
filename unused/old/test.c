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

#include <stdio.h>

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <android/log.h>
#include <fcntl.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>

       #include <sys/types.h>
       #include <sys/socket.h>


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

int main(int argc, char **argv)
{
	int fd = open("/dev/nvrm", O_RDWR|O_SYNC);
	//void* p = mmap(NULL, 1540096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	char c[1024];
	printf("read %d bytes\n",recv(fd,c,10,0));
	printf("1\n");fflush(NULL);
//	memcpy(c,(char*)p,10);
	printf("2\n");fflush(NULL);
	FILE* f = fopen("/sdcard/fb","w");
	printf("file = %d\n",f);
	//printf("p = %d\n",p);
	printf("written %d bytes\n",fwrite(c,1, 1024,f));
	fclose(f);
//	munmap(p,1540096);
	close(fd);
	

	int i;
	for (i=0; i < argc; i++)
		__android_log_print(ANDROID_LOG_INFO,"screencaptured - sajat",argv[i]);
	char s[10];
	pid_t pid = getppid();
	__android_log_print(ANDROID_LOG_INFO,"screencaptured - sajat pid:",itoa(s,pid));

	char** a = environ;
	while (*a)
		__android_log_print(ANDROID_LOG_INFO,"screencaptured - sajat env:",*(a++));

  return 0;
}
