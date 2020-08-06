#ifndef BMS_SHARED_H_INCLUDED
#define BMS_SHARED_H_INCLUDED

#include <stdint.h>

/** Command code to request data */
#define OP_GETDATA   0x0
/** Command code to set address */
#define OP_SETADDR   0x1
/** Command code to enable shunt */
#define OP_SHUNTON   0x2
/** Command code to request version and serial information */
#define OP_VERSION   0x3
/** Command code to enter address mode */
#define OP_ADDRMODE  0x5
/** Command code to send a break signal for baud rate calibration */
#define OP_BREAK     0x6
/** Command code to jump to bootloader */
#define OP_BOOT      0x7

/** Number of bits per command */
#define NUM_CMD_BITS 16
/** Number of bits in a value reply */
#define NUM_DATA_BITS (8 + NUM_VALUES * 16 + 16)
/** Number of bits in a param request */
#define NUM_PARAM_BITS (NUM_CMD_BITS + 16)

/** Number external analog inputs */
#define NUM_INPUTS        4
/** Number of temperature inputs */
#define NUM_TEMP          1
/** Total number of measured values */
#define NUM_VALUES        (NUM_INPUTS + NUM_TEMP)
/** Index of temperature in value array */
#define TEMP_IDX          NUM_INPUTS
/** Calibration command has 6 bits for selecting the channel to
 * be calibrated. 0-7 is single ended channels, 8-15 is differential
 * channels and 16 is temperature channel */
#define CALIB_CHAN_BITS   5
#define CALIB_CHAN_MASK   ((1 << CALIB_CHAN_BITS) - 1)
#define CALIB_VAL_BITS    6
#define CALIB_VAL_MASK    (((1 << CALIB_VAL_BITS) - 1) << CALIB_CHAN_BITS)
#define CALIB_VAL_SHIFTL  (16 - CALIB_VAL_BITS - CALIB_CHAN_BITS)
#define CALIB_VAL_SHIFTR  (CALIB_VAL_SHIFTL + CALIB_CHAN_BITS)
#define CALIB_DIFF_FLAG   0x8
#define CALIB_TEMP_FLAG   0x10
#define CALIB_LOCK_MAGIC  0x1f

/** Since we only have 11 bits for the shunt voltage setpoint we add a fixed offset */
#define SHUNT_VTG_OFFSET  3000

/** Page size of CMU flash memory */
#define PAGE_WORDS 32

/** Maximum pages for application software of CMU */
#define ATTINY_MAX_APPLICATION_PAGES 56

/** Maximum pages for application software of MMU and CSU */
#define ATMEGA_MAX_APPLICATION_PAGES 118

/** Macro for setting version number for software and hardware
 * @param n  name of version information constant to be created
 * @param sa SW Version API
 * @param sb SW Version Major
 * @param sc SW Version Minor
 * @param ha HW Version Major
 * @param hb HW Revision
 * */
#define VERSION(n,sa,sb,sc,sd,ha,hb) static const struct version n = { { sa, sb, sc, sd }, { ha, hb } }

struct BatValues
{
   uint8_t adr;
   uint16_t values[NUM_VALUES];
   uint16_t crc;
} __attribute__((packed));

struct cmd
{
   uint8_t addr;
   uint8_t op; //only 3 bits used
   uint16_t arg; //only use 11 bits
};

struct version 
{
   uint8_t swVersion[4];
   uint8_t hwVersion[2];
} __attribute__((packed));

struct versionComm
{
   struct version version;
   uint32_t serialNumber;
   uint16_t crc;
} __attribute__((packed));

/** Data structure for transmitting a flash page to the boot loader */
struct PageBuf
{
   uint8_t pageNum;          /**< Page number 0..50 */
   uint16_t buf[PAGE_WORDS]; /**< Page content */
   uint16_t crc;             /**< CRC16 xmodem over pageNum and buf */
} __attribute__((packed));

#endif // BMS_SHARED_H_INCLUDED
