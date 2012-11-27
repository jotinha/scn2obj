#include "scn2obj.h"
#include <iostream>
#include <fstream>

#define SAY printf


//from furrycat's
int str_equals(char *one, char *two);
int str_nequals(char *one, char *two,int n);
int str_equiv(const char *one, const char *two);

void ERROR(char * message,...);

void read_generic(void * buffer, std::ifstream* file, int nbytes);

u32 read_u32(std::ifstream* file);

s32 read_s32(std::ifstream* file);

u16 read_u16(std::ifstream* file);

s16 read_s16(std::ifstream* file);

f32 read_f32(std::ifstream* file);





