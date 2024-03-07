// some handy util functions we don't need to stay in the main file
// only defined once upon import, and gets inlined

#ifdef __cplusplus
 extern "C" {
#endif
#ifndef UTILS_DEFINITION
#define UTILS_DEFINITION


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

// toggles on sleep mode
inline void enter_sleep() {
  sleeping = true;
  set_arm_clock(SLEEP_FREQUENCY);
}

inline void exit_sleep() {
  sleeping = false;
  set_arm_clock(INITIAL_CPU_SPEED);
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


#endif
#ifdef __cplusplus
}
#endif
