#include "atmega.h"
#include "SST39SF020A.h"

#include <stdlib.h>
#include <string.h>

// serial commands
#define CMD_DUMP 'd'
#define CMD_RANDOM_READ 'r'
#define CMD_FULL_ERASE 'f'
#define CMD_SECTOR_ERASE 's'
#define CMD_READ_DEVICE_ID 'i'
#define CMD_READ_MANUFACTURER_ID 'm'
#define CMD_WRITE 'w'

// delimit arguments in received serial string
#define DELIMITER ((char)0x20)

#define MIN(x,  y)   (((x) < (y)) ? (x) : (y))

// enable log messages
#define DEBUG 1
#define DISABLED

// redirect stdout to the serial port
static FILE uart_stdout = FDEV_SETUP_STREAM(put_char, NULL, _FDEV_SETUP_WRITE);

// function for search for the delimiter in a string
int findChar(const char* string, uint8_t start)
{
    if (start < 0)
    {
        return -1;
    }

    char* cur = (char*)string;
    cur += start;

    while (cur++)
    {
        if (*cur == DELIMITER)
        {
            return (cur - string);
        }
    }

    return -1;

}

// read from eeprom and write to serial port
void flash_read(const uint32_t start, const uint32_t length)
{
    if (length > ADDR_MASK || start > ADDR_MASK)
    {
        //printf("ERROR\n");
        return;
    }

    uint32_t end = start + length;

    // prevent this going outside of the maximum address
    if (end > ADDR_MASK)
    {
        end = ADDR_MASK + 1;
    }


    for (uint32_t addr = start; addr < end; addr++)
    {
        uint8_t data = SST39SF020A_readData(addr);
        #ifdef DEBUG
        printf("# address=0x%08lx, read=0x%02x\n", addr, data);
        #else
        printf("%02x\n", data);
        #endif
    }
}

// read from serial port and write to eeprom
void flash_write(const uint32_t start, const uint32_t length)
{
    if (length > ADDR_MASK || start > ADDR_MASK)
    {
        //printf("ERROR");
        return;
    }

    uint32_t end = start + length;
    // prevent this going outside of the maximum address
    if (end > ADDR_MASK)
    {
        end = ADDR_MASK + 1;
    }

    char buf[3] = {0};
    unsigned int data = 0;

    for (uint32_t addr = start; addr < end; addr++)
    {
        UART_readString(buf, sizeof(buf)/sizeof(char));
        buf[2] = 0;
        //should read two hex digits
        data = strtoul(buf, NULL, 16);

        SST39SF020A_writeData(addr, data);

        #if DEBUG
        printf("# address=0x%08lx, write=0x%02x\n", addr, (uint8_t)data);
        #else
        //print ok to let the computer know this has accepted the byte
        printf("OK\n");
        #endif // DEBUG
    }

}


int main(void)
{
    stdout = &uart_stdout;

    SST39SF020A_init_pins();

    UART_setup(BAUD);

    SST39SF020A_setChipEnable(FALSE);
    SST39SF020A_setOutputEnable(TRUE);
    SST39SF020A_setWriteEnable(FALSE);

    delay_us(1); //Recommended System Power-up Timing

    // somewhere to read serial parameters
    char cmd[32] = {0};
    char arg[2][16] = {0};

    int index, index2;
    #if DEBUG
    printf("\n# Welcome!\n");
    #endif // DEBUG

    // main loop
    while (1)
    {
        #if DEBUG
        printf("# enter command: \n");
        #endif

        memset(cmd, 0, sizeof(cmd));
        UART_readString(cmd, sizeof(cmd));

        /* command format
        read: r start length\n
        dump: d\n
        write: w start length\n
        man id: m\n
        dev id: i\n
        sector erase: s sector\n
        full erase: f\n
        */

        if (cmd[0] == CMD_DUMP)
        {
            #ifdef DEBUG
            printf("# Dumping chip...\n");
            #endif // DEBUG
            flash_read(0, ADDR_MASK);
        }

        #ifndef DISABLED
        //Disabled due to faulty implementation
        else if (cmd[0] == CMD_READ_DEVICE_ID)
        {
            uint8_t data = SST39SF020A_readDeviceID();
            #if DEBUG
            printf("# device id=0x%02x\n", data);
            #else
            printf("%02x\n", data);
            #endif // DEBUG
        }
        else if (cmd[0] == CMD_READ_MANUFACTURER_ID)
        {
            uint8_t data = SST39SF020A_readManufacturerID();
            #if DEBUG
            printf("# vendor id=0x%02x\n", data);
            #else
            printf("%02x\n", data);
            #endif // DEBUG
        }
        #endif

        else if (cmd[0] == CMD_FULL_ERASE)
        {
            #ifdef DEBUG
            printf("# Erasing chip...\n");
            #endif // DEBUG
            SST39SF020A_chipErase();
            #ifdef DEBUG
            printf("DONE\n");
            #endif
        }
        else
        {
            //commands that require arguments

            memset(arg, 0, sizeof(arg));

            index = findChar(cmd, 0);
            if (index < 0)
            {
                continue;
            }

            index++; // jump over the space char

            index2 = findChar(cmd, index);
            index2++; // jump over the space char

            strncpy(arg[0], cmd + index, MIN(index2 - index, sizeof(arg[0])));

            strncpy(arg[1], cmd + index2, MIN(strlen(cmd) - index2, sizeof(arg[1])));

            if (cmd[0] == CMD_RANDOM_READ)
            {
                #if DEBUG
                printf("# random read\n");
                #endif

                if (!index2)
                {
                    continue;
                }

                uint32_t addr = 0;
                addr = strtoul(arg[0], NULL, 16);

                uint32_t length = 0;
                length = strtoul(arg[1], NULL, 10);

                #if DEBUG
                printf("# addr=0x%lx, len=0x%lx\n", addr, length);
                #endif

                flash_read(addr, length);
            }
            else if (cmd[0] == CMD_SECTOR_ERASE)
            {
                unsigned int sector = 0;
                sector = strtoul(arg[0], NULL, 10);

                #ifdef DEBUG
                printf("# Erasing sector %u...\n", sector);
                #endif // DEBUG

                if (sector < SST39SF020A_NUMSECTORS)
                {
                    SST39SF020A_sectorErase((uint8_t)sector);
                    printf("DONE\n"); //success code
                }
                else
                {
                   printf("ERROR\n"); //success code
                }
            }
            else if (cmd[0] == CMD_WRITE)
            {
                uint32_t addr = 0;
                uint32_t length = 0;

                #if DEBUG
                printf("# write mode\n");
                #endif // DEBUG

                if (!index2 || !strlen(arg[1]))
                {
                    length = ADDR_MASK;
                }
                else
                {
                    length = strtoul(arg[1], NULL, 16);
                }

                addr = strtoul(arg[0], NULL, 16);


                #if DEBUG
                printf("# addr=0x%lx, len=0x%lx\n", addr, length);
                #endif

                flash_write(addr, length);
            }


        }
    }
}

