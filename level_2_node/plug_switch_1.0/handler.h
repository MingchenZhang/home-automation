#ifndef HANDLER_H
#define HANDLER_H

#include "config.h"
#include <Timer.h>

extern Timer* loop_timer;

#ifdef __cplusplus
extern "C" {
#endif

void program_init();

void program_loop();

extern void* receiveBuffer;
extern int connection_check_timer;
#ifdef POWER_SAVING_MODE
extern int power_saving_interval_timer;
extern int sleep_disabled;
void sleep_handler();
void disable_sleep();
void enable_sleep();
#endif

void radioReceiveCheck();


#ifdef __cplusplus
}
#endif

#endif /* HANDLER_H */
