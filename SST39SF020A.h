#ifndef SST39SF020A_H_INCLUDED
#define SST39SF020A_H_INCLUDED

#include "atmega.h"

// EEPROM chip address bus (18bit)
#define ADDR_LOW PORTA //low 8bits A0-7
#define ADDR_HIGH PORTC //high 8bit A8-15
#define ADDR_HIGH2 PORTD //highest 2bits A16-17
#define ADDR_A16 (1<<6)
#define ADDR_A17 (1<<7)

#define ADDR_MASK (0x3ffff)

// EEPROM Data bus
#define DATA_BUS_WRITE PORTB
#define DATA_BUS_READ PINB
#define DATA_BUS_DIR DDRB


// EEPROM control lines
#define CHIP_ENABLE (1<<5) // Chip enable - PD5
#define OUTPUT_ENABLE (1<<4) // Output enable - PD4
#define WRITE_ENABLE (1<<3)// Write enable - PD3
#define CONTROL_LINES PORTD

// Flash/write verification mode bits
#define TOGGLE_BIT (1<<6)
#define DATA_POLL_BIT (1<<7)


// Status of control lines
enum PIN_STATUS {FALSE = 0, TRUE = 1};


// Commands used in 3rd bus write sequence of software mode
#define BUS_CMD_WRITE (uint8_t)0xa0
#define BUS_CMD_ERASE (uint8_t)0x80
#define BUS_CMD_SOFT_ENTRY (uint8_t)0x90
#define BUS_CMD_SOFT_EXIT (uint8_t)0xf0

// Control
void SST39SF020A_init_pins(void);
void SST39SF020A_setChipEnable(int status);
void SST39SF020A_setOutputEnable(int status);
void SST39SF020A_setWriteEnable(int status);

// Read
uint8_t SST39SF020A_readData(uint32_t address);

// Information
uint8_t SST39SF020A_readManufacturerID(void);
uint8_t SST39SF020A_readDeviceID(void);

// Program
void SST39SF020A_writeData(uint32_t address, uint8_t data);
void SST39SF020A_sectorErase(uint8_t sector);
void SST39SF020A_chipErase(void);

// Verify
void waitForToggleBit(void);
void waitForDataPoll(uint_fast8_t data);

#define SST39SF020A_NUMSECTORS 64

#endif // SST39SF020A_H_INCLUDED

