/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   util_address_referer.h
 * Author: mingchen
 *
 * Created on April 9, 2016, 4:08 PM
 */

#ifndef UTIL_ADDRESS_REFERER_H
#define UTIL_ADDRESS_REFERER_H

#ifdef __cplusplus
extern "C" {
#endif

void referer_init();

int referer_add(void* addr);

void* referer_getAddr(int reference);

void referer_set(int reference, void* addr);

void referer_remove(int reference);

void referer_cleanup();

#ifdef __cplusplus
}
#endif

#endif /* UTIL_ADDRESS_REFERER_H */

