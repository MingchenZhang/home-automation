#ifndef CONFIG_H
#define CONFIG_H

#define LIVE_PULSE_INTERVAL 10000
#define MESSAGE_CHECK_INTERVAL 1

#define CONNECTION_INDICATOR 3 //must support analogWrite 
//#define DEBUG_PRINT


#ifdef DEBUG_PRINT
	#define debug(S)	Serial.print(S)
	#define DEBUG_S(S) S
#else
	#define debug(S)
	#define DEBUG_S(S) 
#endif

#define p(S)	Serial.print(S)


#endif /* CONFIG_H */
