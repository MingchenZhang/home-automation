#ifndef CONFIG_H
#define CONFIG_H

//#define DEBUG_PRINT

#define CHANNEL_USED 126
#define DEVICE_NO 20

#define LIVE_PULSE_INTERVAL 5000
#define MESSAGE_CHECK_INTERVAL 1

#define CONNECTION_INDICATOR 6 //must support analogWrite
#define ERROR_INDICATOR  5

#define STATUS_CHECK_INTERVAL 0 // set to 0 to disable it


// Define POWER_SAVING_MODE will allow unit to enter power saving mode every POWER_SAVING_LIVE_TIME second. 
// Each sleep last 2 second and radio will be disabled during sleep (no receiving).
// Sleeping mode is on by default if POWER_SAVING_MODE is defined. 
// Calling disable_sleep() can disable sleep until enable_sleep() is called. 
#define POWER_SAVING_MODE
#ifdef POWER_SAVING_MODE
	#define POWER_SAVING_LIVE_TIME 40
	#define POWER_SAVING_LIVE_PULSE_INTERVAL 4
#endif

#ifdef DEBUG_PRINT
	#define debug(S)	Serial.print(S)
	#define DEBUG_S(S) S
#else
	#define debug(S)
	#define DEBUG_S(S) 
#endif

#define p(S)	Serial.print(S)
#define pl(S)	Serial.println(S)

#endif /* CONFIG_H */
