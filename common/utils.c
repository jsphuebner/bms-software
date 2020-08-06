/**
 * @file
 * @brief Virtual BMU device that governs all local physical devices (MMU, CMU etc.)
 * @author Johannes Huebner <dev@johanneshuebner.com>
 * @date 07.11.2015
 */
#include "utils.h"
#ifdef __WITH_AVRLIBC__
#include <stdlib.h>
#else
#include "stub/stubs.h"
#endif

static char* append(char* buf, const char* str)
{
   for (; *str != 0; str++, buf++)
      *buf = *str;
   *buf = 0;
   return buf;
}

/**
 * @brief Formats software version number as A.B.C.D
 * @param buf Buffer where formatted version number is written to
 * @param version Version information structure
 * @return buf for convenience
 */
char* FormatSwVersion(char* buf, const struct version* version)
{
   char *p;
   p = append(buf, itoa(version->swVersion[0], buf, 10));
   p = append(p, ".");
   p = append(p, itoa(version->swVersion[1], p, 10));
   p = append(p, ".");
   p = append(p, itoa(version->swVersion[2], p, 10));
   p = append(p, ".");
   *p++ = version->swVersion[3];
   *p = 0;

   return buf;
}

/**
 * @brief Formats hardware version number as A.B
 * @param buf Buffer where formatted version number is written to
 * @param version Version information structure
 * @return buf for convenience
 */
char* FormatHwVersion(char* buf, const struct version* version)
{
   char *p;
   p = append(buf, itoa(version->hwVersion[0], buf, 10));
   p = append(p, ".");
   *p++ = version->hwVersion[1];
   *p = 0;

   return buf;
}
