/* This is an ISO C 9X header file. We omit this copy to the include
   directory if the local platform does not have inttypes.h, it contains
   [u]int[8,16,32]_t fixed width types */

#include <config.h>

/* Generate the fixed bit size types */
#if SIZEOF_INT == 4
  typedef int int32_t;
  typedef unsigned int uint32_t;
#else
# if SIZEOF_LONG == 4
  typedef long int32_t;
  typedef unsigned long uint32_t;
# else
#  if SIZEOF_SHORT == 4
    typedef short int32_t;
    typedef unsigned short uint32_t;
#  else
#   error Must have a form of 32-bit integer
#  endif
# endif
#endif

#if SIZEOF_INT == 2
  typedef int int16_t;
  typedef unsigned int uint16_t;
#else
# if SIZEOF_LONG == 2
   typedef long int16_t;
   typedef unsigned long uint16_t;
# else
#  if SIZEOF_SHORT == 2
    typedef short int16_t;
    typedef unsigned short uint16_t;
#  else
#   error Must have a form of 16-bit integer
#  endif
# endif
#endif

typedef signed char int8_t;
typedef unsigned char uint8_t;
