#ifndef __SCOKET_REDIS_SENTINEL_COMMON_H__
#define __SCOKET_REDIS_SENTINEL_COMMON_H__


#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <stdarg.h>
#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <sstream>



#ifndef  _MSC_VER

#define __STDC_FORMAT_MACROS
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>

#else	// NOT _MSC_VER
#ifdef	    __GNUC__
/* FreeBSD has these C99 int types defined in /sys/inttypes.h already */
#ifndef     _SYS_INTTYPES_H_
typedef signed char int8_t;
typedef signed short int16_t;
//typedef signed int int32_t;
typedef signed long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
#endif /* !_SYS_INTTYPES_H_ */
#elif	    defined(_MSC_VER)
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed __int64 int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;
#endif

#endif	// _MSC_VER END




#define URI_TYPE int

/* Even in pure C, we still need a standard boolean typedef */
#ifndef     __cplusplus
typedef unsigned char bool;
#define     true    1
#define     false   0
#endif	    /* !__cplusplus */



enum LogLevel{
	Fatal = 0,	//0
	Error = 3,	//3
	Warn = 4,	//4
	Notice = 5,//5
	Info = 6,	//6
	Debug = 7,	//7
};



#if !defined(__foreach) && !defined(__const_iterator_def)
#define __const_iterator_def(Iterator, Initializer) __typeof(Initializer) Iterator=(Initializer)
#define __foreach(Iterator,Container) for(__const_iterator_def(Iterator,(Container).begin()); Iterator!=(Container).end(); ++Iterator)
#endif







#endif


