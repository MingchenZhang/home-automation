/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   util_tools.h
 * Author: mingchen
 *
 * Created on April 13, 2016, 10:25 PM
 */

#ifndef UTIL_TOOLS_H
#define UTIL_TOOLS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int randomInt(int low, int high);

// return positive represent a valid result
// -1 means unable to read from this fd any more (may disconnected)
// -2 means partial info received, not enough to generate a valid result
int getUtillNewLineNonBlock(int fd, int* buffer);

int getUtillNewLine(int fd);

// return positive represent a valid result
// -1 means unable to read from this fd any more (may disconnected)
// -2 means partial info received, not enough to generate a valid result
int getNextWordNonBlock(int fd, int* buffer);

int getNextWord(int fd);

void showNum(int num);

void showPopup(char* format, ...);

void handler_do_nothing();

int getPasswordInput(int fd);

int printFileContent(char* filename);

inline void binToHex(const void* input, size_t inputLen, char* outBuffer);

inline void hexToBin(const void* input, size_t inputLen, uint8_t* outBuffer);

void printMemory(void* mem, size_t size);

long get_nanosecond();

#ifdef __cplusplus
}
#endif

#endif /* UTIL_TOOLS_H */

