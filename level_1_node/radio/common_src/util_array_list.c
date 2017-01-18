/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

#include "util_array_list.h"
#include "util_address_referer.h"

#define ARRAY_LIST_SIZE ((array)[-3])
#define ARRAY_LIST_CAPA ((array)[-2])
#define ARRAY_LIST_ELEMENT_SIZE ((array)[-1])

//static pthread_mutex_t lock;

int arrayList_init(int initialCapacity, int elementSize){
	referer_init();
	
	int* array = malloc(elementSize*initialCapacity + sizeof(int)*3 );
	array[0] = 0;
	array[1] = initialCapacity;
	array[2] = elementSize;
	
	int ref = referer_add(array+3);
	
//	pthread_mutex_init(&lock, NULL);
	
	return ref;
}

void* arrayList_add(void* newElement, int ref){
//	pthread_mutex_lock(&lock);
	
	int* array = referer_getAddr(ref);
	
	if(ARRAY_LIST_SIZE == ARRAY_LIST_CAPA){// new space needed
		array = realloc((array)-3, ARRAY_LIST_ELEMENT_SIZE*(ARRAY_LIST_CAPA*2) + sizeof(int)*3);
		array += 3;
		ARRAY_LIST_CAPA *= 2;
		referer_set(ref, array);
	}
	void* targetAddress = ((uint8_t*)((array))+(ARRAY_LIST_ELEMENT_SIZE*ARRAY_LIST_SIZE));
	memmove(targetAddress, newElement, ARRAY_LIST_ELEMENT_SIZE);
	ARRAY_LIST_SIZE++;
	
//	pthread_mutex_unlock(&lock);
	return targetAddress;
}

void arrayList_addArray(void* newElement, int size, int ref){// todo: slow
//	pthread_mutex_lock(&lock);
	
	uint8_t* elemCursor =  newElement;
	int i;
	for(i=0; i<size; i++){
		arrayList_add(elemCursor, ref);
		int* array = referer_getAddr(ref);
		elemCursor += ARRAY_LIST_ELEMENT_SIZE;
	}
	
//	pthread_mutex_unlock(&lock);
}

void* arrayList_insert(void* newElement, int index, int ref){
//	pthread_mutex_lock(&lock);
	
	int* array = referer_getAddr(ref);
	
	if(index > ARRAY_LIST_SIZE || index < 0) return NULL;
	if(ARRAY_LIST_SIZE == ARRAY_LIST_CAPA){// new space needed
		array = realloc((array)-3, ARRAY_LIST_ELEMENT_SIZE*(ARRAY_LIST_CAPA*2) + sizeof(int)*3);
		array += 3;
		ARRAY_LIST_CAPA *= 2;
		referer_set(ref, array);
	}
	uint8_t* targetAddress = ((uint8_t*)((array))+(ARRAY_LIST_ELEMENT_SIZE*index));
	memmove(targetAddress + ARRAY_LIST_ELEMENT_SIZE, targetAddress, ARRAY_LIST_ELEMENT_SIZE*(ARRAY_LIST_SIZE-index));
	memmove(targetAddress, newElement, ARRAY_LIST_ELEMENT_SIZE);
	ARRAY_LIST_SIZE++;
	
//	pthread_mutex_unlock(&lock);
	return targetAddress;
}

void* arrayList_set(void* newElement, int index, int ref){
//	pthread_mutex_lock(&lock);
	
	int* array = referer_getAddr(ref);
	
	if(index >= ARRAY_LIST_SIZE || index < 0) return NULL;
	void* targetAddress = ((uint8_t*)((array))+(ARRAY_LIST_ELEMENT_SIZE*index));
	memmove(targetAddress, newElement, ARRAY_LIST_ELEMENT_SIZE);
	
//	pthread_mutex_unlock(&lock);
	return targetAddress;
}

void* arrayList_get(int index, int ref){
//	pthread_mutex_lock(&lock);
	
	int* array = referer_getAddr(ref);
	
	if(index >= ARRAY_LIST_SIZE || index < 0) return NULL;
	void* targetAddress = ((uint8_t*)((array))+(ARRAY_LIST_ELEMENT_SIZE*index));
	
//	pthread_mutex_unlock(&lock);
	return targetAddress;
}

void arrayList_remove(int index, int ref){
//	pthread_mutex_lock(&lock);
	
	int* array = referer_getAddr(ref);
	
	if(index >= ARRAY_LIST_SIZE || index < 0) return;
	void* targetAddress = ((uint8_t*)((array))+(ARRAY_LIST_ELEMENT_SIZE*index));
	memmove(targetAddress, ((uint8_t*)(targetAddress))+ARRAY_LIST_ELEMENT_SIZE, ARRAY_LIST_ELEMENT_SIZE*(ARRAY_LIST_SIZE-index-1) );
	ARRAY_LIST_SIZE--;
	
	if(ARRAY_LIST_SIZE<ARRAY_LIST_CAPA/4 && ARRAY_LIST_CAPA>30){
		array = realloc((array)-3, ARRAY_LIST_ELEMENT_SIZE*((ARRAY_LIST_SIZE+5)*2) + sizeof(int)*3);
		array += 3;
		ARRAY_LIST_CAPA = ((ARRAY_LIST_SIZE+5)*2);
		referer_set(ref, array);
	}
	
//	pthread_mutex_unlock(&lock);
}

//void arrayList_chop(int index, int** array){
//	if(index >= ARRAY_LIST_SIZE || index < 0) return;
//	ARRAY_LIST_SIZE = index;
//	// fill a 0 in case of sequential search (like strlen())
//	void* targetAddress = ((uint8_t*)((*array))+(ARRAY_LIST_ELEMENT_SIZE*index));
//	memset(targetAddress, 0, ARRAY_LIST_ELEMENT_SIZE);
//}
//
//void arrayList_fall(int index, int** array){
//	if(index >= ARRAY_LIST_SIZE || index < 0) return;
//	void* targetAddress = ((uint8_t*)((*array))+(ARRAY_LIST_ELEMENT_SIZE*index));
//	memmove(*array, targetAddress, (ARRAY_LIST_SIZE-index)*ARRAY_LIST_ELEMENT_SIZE);
//	ARRAY_LIST_SIZE -= index;
//}

void arrayList_empty(int ref){
//	pthread_mutex_lock(&lock);
	
	int* array = referer_getAddr(ref);
	
ARRAY_LIST_SIZE = 0;
	
	if(ARRAY_LIST_SIZE<ARRAY_LIST_CAPA/4 && ARRAY_LIST_CAPA>30){
		array = realloc((array)-3, ARRAY_LIST_ELEMENT_SIZE*((ARRAY_LIST_SIZE+5)*2) + sizeof(int)*3);
		array += 3;
		ARRAY_LIST_CAPA = ((ARRAY_LIST_SIZE+5)*2);
		referer_set(ref, array);
	}
	
//	pthread_mutex_unlock(&lock);
}

void arrayList_free(int ref){
	int* array = referer_getAddr(ref);
	if(array == NULL) return;
	free((array)-3);
	referer_remove(ref);
}

int arrayList_getSize(int ref){
//	pthread_mutex_lock(&lock);
	int* array = referer_getAddr(ref);
	int ret = ARRAY_LIST_SIZE;
//	pthread_mutex_unlock(&lock);
	return ret;
}