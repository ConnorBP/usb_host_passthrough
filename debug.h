// debug macro definitions for easy switching of debug features like printing
#define TRUE 1
#define FALSE 0

#if DEBUG
// debug print macro
#define DBGPRINT(str) Serial.print(str)
#define DBGPRINTLN(str) Serial.println(str)
#else
// no-op any debug prints when not in debug mode
#define DBGPRINT(str)
#define DBGPRINTLN(str)
#endif
