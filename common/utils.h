/**
 * @file utils.h
 * @brief Utility functions
 * @author Johannes Huebner <dev@johanneshuebner.com>
 * @date 05.04.2016
 */
#include "bms_shared.h"

char* FormatSwVersion(char* buf, const struct version* version);
char* FormatHwVersion(char* buf, const struct version* version);
