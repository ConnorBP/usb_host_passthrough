// some handy util functions we don't need to stay in the main file
// only defined once upon import, and gets inlined
//#include <Arduino.h>

#ifdef __cplusplus
 extern "C" {
#endif
#ifndef UTILS_DEFINITION
#define UTILS_DEFINITION

// constant char len string compare to help avoid buffer injection
#define STRCMP(str1,str2) strcmp(str1,str2)
#define STRNCMP(str1,str2) strncmp(str1,str2,strlen(str2))
#define STREQ(str1,str2) STRCMP(str1,str2) == 0

// mouse clicking macros

#define PRESS_LEFTM serial_buttons_value |= MOUSE_LEFT
#define RELEASE_LEFTM serial_buttons_value &= ~MOUSE_LEFT
#define PRESS_RIGHTM serial_buttons_value |= MOUSE_RIGHT
#define RELEASE_RIGHTM serial_buttons_value &= ~MOUSE_RIGHT

const uint32_t INITIAL_CPU_SPEED = F_CPU_ACTUAL;
// if this is set, then the board is in a lower power mode
// any extra stuff that is not passthrough or serial queries should be skipped
// in the event that this is set to true
static volatile bool sleeping = false;
#define SLEEP_FREQUENCY 150000000


extern float tempmonGetTemp(void);
#if defined(__IMXRT1062__)
extern "C" uint32_t set_arm_clock(uint32_t frequency);
#endif

// A small error message helper. Locks the controller upon error
inline void error(const __FlashStringHelper*err) {
  Serial.print("ENCOUNTERED CRITICAL ERROR: ");
  Serial.println(err);
  while (1);
}

// toggles on sleep mode
inline void enter_sleep() {
  sleeping = true;
  set_arm_clock(SLEEP_FREQUENCY);
}

inline void exit_sleep() {
  sleeping = false;
  set_arm_clock(INITIAL_CPU_SPEED);
}
extern uint8_t serial_buttons_value;
inline void clear_mouse() {
  serial_buttons_value = 0;
}

inline bool prefix(const char *pre, const char *str)
{
  return strncmp(pre, str, strlen(pre)) == 0;
}

inline void printtemp(void) {
  float temp = tempmonGetTemp();
  Serial.printf("Tempurature: %f\n", temp);
}

void printcpu(void) {
  Serial.printf("CPU CLK %dMHz ORIG %dMHz\n",F_CPU_ACTUAL/1000000, INITIAL_CPU_SPEED/1000000);
}

// casts four bytes from a byte array into an int
inline int cast_int32_parts(const char *str, int startByte)
{
  int i = (int) ((str[startByte] << 24) | (str[startByte+1] << 16) | str[startByte+2] << 8) | (str[startByte+3]);
  return i;
}

#endif
#ifdef __cplusplus
}
#endif
