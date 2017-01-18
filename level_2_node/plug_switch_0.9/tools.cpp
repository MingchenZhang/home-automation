#include <Arduino.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include "tools.h"


extern volatile unsigned long timer0_millis;
void sleep_2_seconds(){
	// disable ADC
	ADCSRA = 0;  
	// clear various "reset" flags
	MCUSR = 0;
	// allow changes, disable reset
	WDTCSR = bit(WDCE) | bit(WDE);
	// set interrupt mode and an interval
	WDTCSR = bit(WDIE) | bit(WDP2) | bit(WDP1) | bit(WDP0);// set WDIE, and 2 seconds delay
	//WDTCSR = bit(WDIE) | bit(WDP3);// set WDIE, and 4 seconds delay
	wdt_reset();  // pat the dog
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);  
	noInterrupts();           // timed sequence follows
	sleep_enable();
	// turn off brown-out enable in software
	MCUCR = bit(BODS) | bit(BODSE);
	MCUCR = bit(BODS); 
	interrupts();             // guarantees next instruction executed
	sleep_cpu();

	// cancel sleep as a precaution
	sleep_disable();
	
	timer0_millis += 2000; // set timer in arduino IDE
}
ISR (WDT_vect){
   wdt_disable();  // disable watchdog
}

void int_to_big(int num, char* address_out){
	int i;
	for(i=sizeof(int); i>=1; i--) {address_out[i-1] = ( num >> (8*(sizeof(int)-i)) ) & 0xFF;}
}

int big_to_int(char* address_in){
	int num = 0;
	int i;
	for(i=sizeof(int); i>=1; i--) {num += ((uint64_t)(address_in[i-1] & 0xFF) << (8*(sizeof(int)-i)));}
	return num;
}

#ifdef POWER_SAVING_MODE
#endif