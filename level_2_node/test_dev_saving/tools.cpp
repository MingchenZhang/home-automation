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

#ifdef POWER_SAVING_MODE
#endif