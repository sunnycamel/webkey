/*
Copyright (C) 2010  Peter Mora, Zoltan Papp

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public Licgetfilefor more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <stdio.h>
//#include <pthread.h>


int main(int argc, char **argv)
{
//	pthread_mutex_t mutex;
//	pthread_mutex_init(&mutex, NULL);
//	pthread_mutex_lock(&mutex);
//	pthread_mutex_unlock(&mutex);
//	pthread_mutex_destroy(&mutex);
  float f,f2;
  double d;
  f = 1.1;
  f2 = 5.1;
  for (int i =0; i < 100; i++)
  {
	  f2 += 1.1;
  }
  d = 1.2;
  printf("size of float = %d\n",sizeof(float));
  printf("size of double = %d\n",sizeof(double));
  printf("1.1 = %f\n",f);
  printf("1.2 = %f\n",d);
  printf("1.1 + 1.2 = %f\n",f+d);
  f = f + f2;
  printf("f = %f\n",f);
  printf("returning 0\n");
  return 0;
}
