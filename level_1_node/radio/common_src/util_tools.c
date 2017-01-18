/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <termios.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include "util_array_list.h"
#include "util_address_referer.h"
#include "util_debug.h"

int randomInt(int low, int high){
	srand(time(NULL));
	return (rand() % (high-low) + low);
}

#define IsLineDelim(c) (c == '\n')
int getUtillNewLineNonBlock(int fd, int* buffer){
	if(referer_getAddr(*buffer) == NULL){
		*buffer = arrayList_init(20, sizeof(char));
	}
	int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
		arrayList_add("\0", *buffer);
		int result = *buffer;
		*buffer = -1;
		return result;
	}
	assert(fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0);
	
	while(1){
		char newChar;
		int status;
		if((status = read(fd, &newChar, 1)) <0 && errno == EAGAIN){
			// reach current read end
			return -2;
		}else if(status >0){
			// new character
			if(IsLineDelim(newChar)){
				arrayList_add("\0", *buffer);
				int result = *buffer;
				*buffer = -1;
				return result;
			}else{
				arrayList_add(&newChar, *buffer);
			}
		}else{
			// fd may have closed
			if(arrayList_getSize(*buffer)>0){
				arrayList_add("\0", *buffer);
				int result = *buffer;
				*buffer = -1;
				return result;
			}else
				return -1;
		}
	}
	return -1;
}
#undef IsLineDelim

int getUtillNewLine(int fd){
	int strArray = arrayList_init(10, sizeof(char));
	while(1){
		char temp;
		if(read(fd, &temp, 1) < 1) {
			arrayList_free(strArray);
			return -1;
		}
		if(temp == '\0' || temp == '\n') break;
		arrayList_add(&temp, strArray);
	}
	arrayList_add("\0", strArray);
	return (strArray);
}

#define IsWordDelim(c) (c == ' ' || c == '\r' || c == '\n' || c == '\0')
int getNextWordNonBlock(int fd, int* buffer){
	if(referer_getAddr(*buffer) == NULL){
		*buffer = arrayList_init(10, sizeof(char));
	}
	int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
		if(arrayList_getSize(*buffer)>0){
			arrayList_add("\0", *buffer);
			int result = *buffer;
			*buffer = -1;
			return result;
		}else{
			return -1;
		}
	}
	if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) error("cannot set fd to O_NONBLOCK\n");
	
	while(1){
		char newChar;
		int status;
		if((status = read(fd, &newChar, 1)) <0 && errno == EAGAIN){
			// reach current read end
			return -2;
		}else if(status >0){
			// new character
			if(IsWordDelim(newChar)){
				if(arrayList_getSize(*buffer)>0){
					arrayList_add("\0", *buffer);
					int result = *buffer;
					*buffer = -1;
					return result;
				}
			}else{
				arrayList_add(&newChar, *buffer);
			}
		}else{
			// fd may have closed
			if(arrayList_getSize(*buffer)>0){
				arrayList_add("\0", *buffer);
				int result = *buffer;
				*buffer = -1;
				return result;
			}else{
				if(*buffer!=-1) arrayList_free(*buffer);
				return -1;
			}
		}
	}
	if(*buffer!=-1) arrayList_free(*buffer);
	return -1;
}

int getNextWord(int fd){
	int strArray = arrayList_init(10, sizeof(char));
	while(1){
		char temp;
		if(read(fd, &temp, 1) < 1) {
			arrayList_free(strArray);
			return -1;
		}
		if(IsWordDelim(temp)) {
			if(arrayList_getSize(strArray)==0){
				continue;
			}else{
				break;
			}
		}
		arrayList_add(&temp, strArray);
	}
	arrayList_add("\0", strArray);
	return (strArray);
}
#undef IsWordDelim

void showNum(int num){
	char buffer[30];
	sprintf(buffer, "notify-send \"number: %d\"", num);
	system(buffer);
}

void showPopup(char* format, ...){
	char* newFormat;
	asprintf(&newFormat, "notify-send \"%s\"", format);
	va_list args;
	va_start(args, format);
	char* command;
	vasprintf(&command, newFormat, args);
	va_end(args);
	system(command);
	free(newFormat);
	free(command);
}

void handler_do_nothing(){
	
}

int getPasswordInput(int fd){
	int termChanged = 0;
	struct termios term, oldTerm;
	if(tcgetattr(fd, &oldTerm)==0){
		term = oldTerm;
		term.c_lflag &= ~ECHO;
		if(tcsetattr(fd, TCSANOW, &term)>=0) termChanged = 1;
	}
	
	int result = getUtillNewLine(fd);
	
	if(termChanged){
		tcsetattr(fd, TCSANOW, &oldTerm);
	}
	return result;
}

int printFileContent(char* filename){
	int fd = open(filename, O_RDONLY);
	if(fd < 0) return -1;
	char buffer[128];
	while(1){
		int readed = read(fd, buffer, 128);
		if(readed<0) break;
		int writen = write(STDOUT_FILENO, buffer, readed);
		if(readed<128 || writen<128) break;
	}
	close(fd);
	return 0;
}

inline static char NtoH(uint8_t num){
	if(num<10) return (char)(num+'0');
	else if(num<16) return (char)(num-10+'A');
	else return 'F';
}
inline void binToHex(const void* input, size_t inputLen, char* outBuffer){
	size_t i;
	for(i=0; i<inputLen; i++){
		uint8_t byte_ = ((uint8_t*)input)[i];
		*outBuffer = NtoH(byte_>>4 & 0xF);
		outBuffer++;
		*outBuffer = NtoH(byte_ & 0xF);
		outBuffer++;
	}
	*outBuffer = '\0';
}
inline static uint8_t HtoN(char c){
	if('0'<=c && c<='9') return (uint8_t)(c-'0');
	else if('A'<=c && c<='F') return (uint8_t)(c-'A'+10);
	else if('a'<=c && c<='f') return (uint8_t)(c-'a'+10);
	else return '0';
}
inline void hexToBin(const void* input, size_t inputLen, uint8_t* outBuffer){
	size_t i;
	for(i=0; i<inputLen; i+=2){
		char upper = ((char*)input)[i];
		char lower = ((char*)input)[i+1];
		*outBuffer = (HtoN(upper)<<4 & 0xF0) | (HtoN(lower) & 0xF);
		outBuffer++;
	}
}

void printMemory(void* mem, size_t size){
	int i;
	for(i=0;i<size;i++)
	{
		char* address;
		address = ((char*)mem+i);
		printf("%.2X ", *address);                                        
	}
}

long get_nanosecond(){
	long ns; // Milliseconds
	struct timespec spec;
	clock_gettime(CLOCK_REALTIME, &spec);
	ns = round(spec.tv_nsec / 1.0e3); // Convert nanoseconds to milliseconds
	return ns;
}