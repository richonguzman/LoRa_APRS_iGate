#define DCD_ON            0x03

#define FEND              0xC0
#define FESC              0xDB
#define TFEND             0xDC
#define TFESC             0xDD

#define CMD_UNKNOWN       0xFE
#define CMD_DATA          0x00
#define CMD_HARDWARE      0x06

#define HW_RSSI           0x21

#define CMD_ERROR         0x90
#define ERROR_INITRADIO   0x01
#define ERROR_TXFAILED    0x02
#define ERROR_QUEUE_FULL  0x04

#define APRS_CONTROL_FIELD 0x03
#define APRS_INFORMATION_FIELD 0xf0

#define HAS_BEEN_DIGIPITED_MASK 0b10000000
#define IS_LAST_ADDRESS_POSITION_MASK 0b1