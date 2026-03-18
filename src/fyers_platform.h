/**
 * @file fyers_platform.h
 * @brief Platform compatibility (Windows vs POSIX) for sleep, usleep, htonl/ntohl
 */
 #ifndef FYERS_PLATFORM_H
 #define FYERS_PLATFORM_H
 
 #ifdef _WIN32
 #  include <winsock2.h>
 #  include <windows.h>
 #  include <string.h>
 /* MSVC uses strtok_s instead of POSIX strtok_r (same signature) */
 #  ifndef strtok_r
 #    define strtok_r strtok_s
 #  endif
 
 /* sleep(seconds) -> Sleep(milliseconds) */
 static inline void fyers_sleep(unsigned int seconds) {
     Sleep(seconds * 1000);
 }
 #  define sleep(sec) fyers_sleep((unsigned int)(sec))
 
 /* usleep(microseconds) -> Sleep(milliseconds) */
 static inline void fyers_usleep(unsigned int usec) {
     Sleep(usec / 1000);
 }
 #  define usleep(usec) fyers_usleep((unsigned int)(usec))
 
 #else
 #  include <unistd.h>
 #  include <arpa/inet.h>
 #endif
 
 #endif /* FYERS_PLATFORM_H */