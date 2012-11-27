#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"

void ERROR(char * message,...)
{
	va_list args;

    va_start(args, message);
    vprintf(message, args);
    va_end(args);

	printf("\n");
	exit(1);
}

//stolen from furrycat
int str_nequals(char *one, char *two,int n){
  int i;
  for (i = 0;i<n; i++) {
    if (two[i] != one[i]) return 0;
    if (! one[i]) return 1;
  }
  return 1;
}
int str_equals(char *one, char *two) {
  int i;
  for (i = 0; ; i++) {
    if (two[i] != one[i]) return 0;
    if (! one[i]) return 1;
  }
}

int str_equiv(const char *one,const char *two) {
  int i;
  for (i = 0; ; i++) {
    if (tolower(two[i]) != tolower(one[i])) return 0;
    if (! one[i]) return 1;
  }
}

void read_generic(void * buffer, std::ifstream* file, int nbytes)
{
    if (file->is_open())
    {
        file->read((char*)buffer,nbytes);
        if (!file->fail())
            return;
    }
    //else
    //ERROR("read_generic: Error reading %s, %i bytes",file->getFileName(),nbytes);
    ERROR("read_generic: Error reading %i bytes",nbytes);
}

u32 read_u32(std::ifstream* file){
    u32 ret;
    read_generic(&ret,file,sizeof(u32));
    return ret;
}

s32 read_s32(std::ifstream* file){
    s32 ret;
    read_generic(&ret,file,sizeof(s32));
    return ret;
}


u16 read_u16(std::ifstream* file){
    u16 ret;
    read_generic(&ret,file,sizeof(u16));
    return ret;
}

s16 read_s16(std::ifstream* file){
    s16 ret;
    read_generic(&ret,file,sizeof(s16));
    return ret;
}

f32 read_f32(std::ifstream* file){
    f32 ret;
    read_generic(&ret,file,sizeof(f32));
    return ret;
}
