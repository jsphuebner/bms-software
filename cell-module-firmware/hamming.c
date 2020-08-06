/** @file
  * @brief Implementation of Extended Hamming 15/11 code (http://de.wikipedia.org/wiki/Hamming-Code)
  *
*/

#include "hamming.h"

#define EXTENDED_CODE_BITS 16
#define HAMMING_BITS 15
#define DATA_BITS 11
#define PARITY_BITS (HAMMING_BITS - DATA_BITS)
#define FIRST_CODE_DATA_BIT 2
#define DATA_BIT_MSB (DATA_BITS - 1)
#define PARITY_BIT_LSB HAMMING_BITS

#define NOT_PARITY_BIT_POS(cbit) ((cbit != 0) && (cbit != 1) && (cbit != 3)  && (cbit != 7))

static uint16_t data_from_code(uint16_t c);
static uint8_t calc_syndrome(uint16_t c);
static uint8_t calc_parity(uint16_t c);
static uint16_t data_to_code(uint16_t d);
static uint16_t calc_hamming_parity(uint16_t c);

/** Decode an extended Hamming 16/11 code word
 * @param[in] c received code word
 * @param[out] d decoded data
 * @retval DEC_RES_OK data in d is valid
 * @retval DEC_RES_ERR data in d is invalid
 */
int8_t hamming_decode(uint16_t c, uint16_t* d)
{
   uint8_t syndrome = calc_syndrome(c);
   uint8_t p = calc_parity(c);
   int8_t derr = DEC_RES_OK;

   if (syndrome == 0 && p == 0) //No error
   {
   }
   else if (syndrome > 0 && p == 1) //Correctable error
   {
      c ^= 1 << (syndrome - 1); //Toggle errornous bit
   }
   else if (syndrome == 0 && p == 1) //Uncorrectable error
   {
      derr = DEC_RES_ERR;
   }
   else if (syndrome > 0 && p == 0) //Uncorrectable error
   {
      derr = DEC_RES_ERR;
   }

   *d = data_from_code(c);

   return derr;
}

/** Encode data with extended Hamming 16/11 code
 * @param[in] d data to be encoded
 * @return code word
 */
uint16_t hamming_encode(uint16_t d)
{
   uint16_t c = data_to_code(d);

   c = calc_hamming_parity(c);

   //Calculate extended parity bit
   c |= calc_parity(c) << PARITY_BIT_LSB;

   return c;
}

/************** Helper functions ***************/
static uint8_t calc_parity(uint16_t c)
{
   uint8_t p = 0;
   for (uint8_t cbit = 0; cbit < EXTENDED_CODE_BITS; cbit++)
   {
      p ^= (c >> cbit) & 1;
   }
   return p;
}

static uint8_t calc_syndrome(uint16_t c)
{
   uint8_t syndrome = 0;
   for (uint8_t sbit = 0; sbit < PARITY_BITS; sbit++)
   {
      for (uint8_t cbit = 0; cbit < HAMMING_BITS; cbit++)
      {
         if (((cbit + 1) & (1 << sbit)) > 0)
         {
            syndrome ^= ((c >> cbit) << sbit) & (1 << sbit);
         }
      }
   }
   return syndrome;
}

static uint16_t data_from_code(uint16_t c)
{
   uint16_t d = 0;
   for (uint8_t cbit = 0, dbit = DATA_BIT_MSB; cbit < HAMMING_BITS; cbit++)
   {
      if (NOT_PARITY_BIT_POS(cbit))
      {
         d |= ((c >> cbit) & 1) << dbit;
         dbit--;
      }
   }
   return d;
}

static uint16_t data_to_code(uint16_t d)
{
   uint16_t c = 0;

   //Place data in code word reversed
   for (uint8_t cbit = 0, dbit = DATA_BIT_MSB; cbit < HAMMING_BITS; cbit++)
   {
      if (NOT_PARITY_BIT_POS(cbit))
      {
         c |= ((d >> dbit) & 1) << cbit;
         dbit--;
      }
   }
   return c;
}

static uint16_t calc_hamming_parity(uint16_t c)
{
   //Calculate parity bits and place in code word
   for (uint8_t pbit = 0; pbit < PARITY_BITS; pbit++)
   {
      for (uint8_t dbit = FIRST_CODE_DATA_BIT; dbit < HAMMING_BITS; dbit++)
      {
         if (((dbit + 1) & (1 << pbit)) > 0)
         {
            c ^= ((c >> dbit) & 1) << ((1 << pbit) - 1);
         }
      }
   }
   return c;
}
