/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   util_array_list.h
 * Author: mingchen
 *
 * Created on March 15, 2016, 11:10 PM
 */

#ifndef UTIL_ARRAY_LIST_H
#define UTIL_ARRAY_LIST_H

int arrayList_init(int initialCapacity, int elementSize);

void* arrayList_add(void* newElement, int ref);

void arrayList_addArray(void* newElement, int size, int ref);

void* arrayList_insert(void* newElement, int index, int ref);

void* arrayList_set(void* newElement, int index, int ref);

void* arrayList_get(int index, int ref);

void arrayList_remove(int index, int ref);

//void arrayList_chop(int index, int** array);
//
//void arrayList_fall(int index, int** array);

void arrayList_empty(int ref);

void arrayList_free(int ref);

int arrayList_getSize(int ref);

#endif /* UTIL_ARRAY_LIST_H */

