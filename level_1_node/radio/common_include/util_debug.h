/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   util_debug.h
 * Author: mingchen
 *
 * Created on April 9, 2016, 4:27 PM
 */

#ifndef DEBUG_H
#define DEBUG_H

#include "stdio.h"

#include "util_tools.h"

// Colors
#ifdef COLOR
	#define KNRM  "\x1B[0m"
    #define KDIM  "\x1B[2m"
	#define KRED  "\x1B[1;31m"
	#define KGRN  "\x1B[1;32m"
	#define KYEL  "\x1B[1;33m"
	#define KBLU  "\x1B[1;34m"
	#define KMAG  "\x1B[1;35m"
	#define KCYN  "\x1B[1;36m"
	#define KWHT  "\x1B[1;37m"
	#define KBWN  "\x1B[0;33m"
#else
	/* Color was either not defined or Terminal did not support */
	#define KNRM
	#define KRED
	#define KGRN
	#define KYEL
	#define KBLU
	#define KMAG
	#define KCYN
	#define KWHT
	#define KBWN
#endif

#ifdef DEBUG
	#define debug(S, ...)   fprintf(stderr, KMAG "DEBUG: %s:%s:%d(%ld) " KNRM S, __FILE__, __FUNCTION__, __LINE__, get_nanosecond(), ##__VA_ARGS__);fflush(stderr)
	#define error(S, ...)   fprintf(stderr, KRED "ERROR: %s:%s:%d(%ld) " KNRM S, __FILE__, __FUNCTION__, __LINE__, get_nanosecond(), ##__VA_ARGS__);fflush(stderr)
	#define warn(S, ...)    fprintf(stderr, KYEL "WARN: %s:%s:%d(%ld) " KNRM S, __FILE__, __FUNCTION__, __LINE__, get_nanosecond(), ##__VA_ARGS__);fflush(stderr)
	#define info(S, ...)    fprintf(stdout, KBLU "INFO: %s:%s:%d(%ld) " KNRM S, __FILE__, __FUNCTION__, __LINE__, get_nanosecond(), ##__VA_ARGS__);fflush(stdout)
	#define success(S, ...) fprintf(stdout, KGRN "SUCCESS: %s:%s:%d(%ld) " KNRM S, __FILE__, __FUNCTION__, __LINE__, get_nanosecond(), ##__VA_ARGS__);fflush(stdout)
#else
	#define debug(S, ...)
	#define error(S, ...)   fprintf(stderr, KRED "ERROR: " KNRM S, ##__VA_ARGS__);fflush(stderr)
	#define warn(S, ...)    fprintf(stderr, KYEL "WARN: " KNRM S, ##__VA_ARGS__);fflush(stderr)
	#define info(S, ...)    fprintf(stdout, KBLU "INFO: " KNRM S, ##__VA_ARGS__);fflush(stdout)
	#define success(S, ...) fprintf(stdout, KGRN "SUCCESS: " KNRM S, ##__VA_ARGS__);fflush(stdout)
#endif

#endif

