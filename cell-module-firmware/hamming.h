#ifndef HAMMING_H_INCLUDED
#define HAMMING_H_INCLUDED

#include <stdint.h>

#define DEC_RES_OK 0
#define DEC_RES_ERR (-1)

#ifdef __cplusplus
extern "C"
{
#endif

uint16_t hamming_encode(uint16_t d);
int8_t hamming_decode(uint16_t c, uint16_t* d);

#ifdef __cplusplus
}
#endif
#endif // HAMMING_H_INCLUDED
