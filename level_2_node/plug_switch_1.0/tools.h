#ifndef TOOLS_H
#define TOOLS_H

#ifdef __cplusplus
extern "C" {
#endif

void sleep_2_seconds();

#ifdef POWER_SAVING_MODE
#endif

void int_to_big(int num, char* address_out);
int big_to_int(char* address_in);

#ifdef __cplusplus
}
#endif

#endif /* TOOLS_H */