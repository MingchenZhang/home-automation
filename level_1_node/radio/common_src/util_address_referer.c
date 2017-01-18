/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include "util_address_referer.h"
#include "util_debug.h"

static void** head = NULL;
static int listSize = 0;
static int listCapa = 5;
static pthread_mutex_t lock;

void referer_init(){
	if(head!= NULL) return;
	pthread_mutex_init(&lock, NULL);
	head = malloc(listCapa * sizeof(void*));
}

int referer_add(void* addr){
	pthread_mutex_lock(&lock);
	if(head == NULL){
		warn("referer not initialized\n");
		exit(EXIT_FAILURE);
	}
	
	if(listSize == listCapa){
		listCapa *= 2;
		head = realloc(head, listCapa * sizeof(void*));
	}
	
	int i=0;
	for(;i<listSize; i++){
		if(head[i] == NULL){
			head[i] = addr;
			break;
		}
	}
	if(i==listSize) head[listSize++] = addr;
	
	int ret = i;
	pthread_mutex_unlock(&lock);
	return ret;
}

void* referer_getAddr(int reference){
	pthread_mutex_lock(&lock);
	if(reference>=0 && reference<listSize && head!=NULL){
		void* ret = head[reference];
		//if(ret == NULL) warn("referer cannot find address\n");
		pthread_mutex_unlock(&lock);
		return ret;
	}else{
		//warn("referer cannot find address\n");
		pthread_mutex_unlock(&lock);
		return NULL;
	}
}

void referer_set(int reference, void* addr){
	pthread_mutex_lock(&lock);
	if(reference>=0 && reference<listSize && head!=NULL){
		head[reference] = addr;
	}else{
		warn("referer cannot find address\n");
	}
	pthread_mutex_unlock(&lock);
}

void referer_remove(int reference){
	pthread_mutex_lock(&lock);
	if(reference>=0 && reference<listSize && head!=NULL){
		head[reference] = NULL;
	}else{
		warn("referer cannot find address\n");
	}
	pthread_mutex_unlock(&lock);
}

void referer_cleanup(){
	if(head != NULL) free(head);
	head = NULL;
	listSize = 0;
	listCapa = 5;
	pthread_mutex_destroy(&lock);
}