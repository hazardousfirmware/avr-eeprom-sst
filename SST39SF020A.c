#include "SST39SF020A.h"

// pin toggle functions
static inline void chipEnable(void)
{
    // set CE low
    CONTROL_LINES &= ~CHIP_ENABLE;
}

static inline void chipDisable(void)
{
    // set CE high
    CONTROL_LINES |= CHIP_ENABLE;
}

static inline void outputEnable(void)
{
    // set OE low
    CONTROL_LINES &= ~OUTPUT_ENABLE;
}

static inline void outputDisable(void)
{
    // set OE high
    CONTROL_LINES |= OUTPUT_ENABLE;
}

static inline void writeEnable(void)
{
    // set WE low
    CONTROL_LINES &= ~WRITE_ENABLE;
}

static inline void writeDisable(void)
{
    // set WE high
    CONTROL_LINES |= WRITE_ENABLE;
}

// control the data bus
static inline void dataBusWrite(uint_fast8_t data)
{
    // Write data to the bus (PORTB by default)
    DATA_BUS_WRITE = data;
}

static inline void busClear(void)
{
    DATA_BUS_WRITE = 0x00;
    ADDR_LOW = 0x00;
    ADDR_HIGH = 0x00;

    ADDR_HIGH2 = ADDR_HIGH2 & ~(ADDR_A16 | ADDR_A17);
}

static inline void dataBusDirIn(void)
{

    DATA_BUS_DIR = 0x00; // set input direction
    DATA_BUS_WRITE = 0xff; //enable pull ups
}

static inline void dataBusDirOut(void)
{

    DATA_BUS_DIR = 0xff;
    DATA_BUS_WRITE = 0x00;
}

// Perform 1st 3 bus write sequences (common for write, sector erase, chip erase, software ID mode)
static inline void startSoftwareModeSequence(uint_fast8_t data)
{
    ADDR_HIGH2 &= ~(ADDR_A16 | ADDR_A17); //Most significant address bits are not needed yet

    chipEnable();
    outputDisable();

    // 1st bus write cycle
    ADDR_LOW = 0x55;
    ADDR_HIGH = 0x55;
    writeEnable(); //latch the address
    dataBusWrite(0xaa);
    CLOCK_DELAY;
    writeDisable(); //latch the data


    // 2nd bus write cycle
    ADDR_LOW = 0xaa;
    ADDR_HIGH = 0x2a;
    writeEnable(); //latch the address
    dataBusWrite(0x55);
    CLOCK_DELAY;
    CLOCK_DELAY;
    writeDisable(); //latch the data


    // 3rd bus write cycle
    ADDR_LOW = 0x55;
    ADDR_HIGH = 0x55;
    CLOCK_DELAY;
    writeEnable(); //latch the address
    dataBusWrite(data);
    CLOCK_DELAY;
    writeDisable(); //latch the data
}


// Initialize the output pins (control and address lines, data is only output when writing).
void SST39SF020A_init_pins(void)
{
    DDRA = 0xff; // Address low pins are outputs
    DDRC = 0xff; // Address high pins are outputs
    dataBusDirIn(); // read mode is default
    DDRD = 0xf8; // Additional address pins and control pins are outputs, pins PD0-1 used for UART, PD2 unused

    //default to standby
    chipDisable();
    outputEnable();
    writeDisable(); //default to read mode
}


// set the CE pin (low acting)
void SST39SF020A_setChipEnable(int status)
{
    if (!status)
    {
        chipDisable();
    }
    else
    {
        chipEnable();
    }
}

// set the OE pin (low acting)
void SST39SF020A_setOutputEnable(int status)
{
    if (!status)
    {
        outputDisable();
    }
    else
    {
        outputEnable();
    }
}

// set the WE pin (low acting)
void SST39SF020A_setWriteEnable(int status)
{
    if (!status)
    {
        writeDisable();
    }
    else
    {
        writeEnable();
    }
}


// function to read from an address
uint8_t SST39SF020A_readData(uint32_t address)
{
    busClear();

    address &= ADDR_MASK; //18bit address space

    const uint8_t addr_low = (uint8_t)(address & 0x00ff);
    const uint8_t addr_high = (uint8_t)((address & 0xff00) >> 8);
    const uint8_t addr_high2 = (uint8_t)((address & 0x30000) >> 10);

    dataBusDirIn();
    writeDisable();

    ADDR_LOW = addr_low;
    ADDR_HIGH = addr_high;
    ADDR_HIGH2 &= ~(ADDR_A16 | ADDR_A17);
    ADDR_HIGH2 |= addr_high2;

    chipEnable();
    outputEnable();

    // account for propagation delay
    delay_us(1);

    uint8_t result = DATA_BUS_READ;

    CLOCK_DELAY;

    chipDisable();
    outputDisable();

    return result;
}


uint8_t SST39SF020A_readManufacturerID(void)
{
    return 0xbf;
    // TODO: implement this, once dev id is working.
}

uint8_t SST39SF020A_readDeviceID(void)
{
    //TODO: dev id always reads address 0 of the rom, never enters software mode successfully

    chipEnable();

    startSoftwareModeSequence(BUS_CMD_SOFT_ENTRY);
    chipDisable();
    outputDisable();
    writeDisable();

    ADDR_HIGH = 0x00;
    ADDR_LOW = 0x01;

    dataBusDirIn();

    CLOCK_DELAY;
    CLOCK_DELAY;

    outputEnable();
    chipEnable();

    uint8_t result = DATA_BUS_READ;
    //SST39SF020A should always return 0xb6

    CLOCK_DELAY;
    CLOCK_DELAY;

    startSoftwareModeSequence(BUS_CMD_SOFT_EXIT);
    chipDisable();
    outputEnable();

    writeDisable();

    return result;
}


void SST39SF020A_writeData(uint32_t address, uint8_t data)
{
    // prepare the address. These calculations are slow on a dumb 8bit micro-controller.
    address &= ADDR_MASK; //18bit address space

    // Store the address values for now and access the pre-computed values when needed.
    const uint8_t addr_low = (uint8_t)(address & 0x00ff);
    const uint8_t addr_high = (uint8_t)((address & 0xff00) >> 8);

    uint8_t addr_high2 = (uint8_t)((address & 0x30000) >> 10);
    addr_high2 |= OUTPUT_ENABLE;
    addr_high2 &= ~(CHIP_ENABLE | WRITE_ENABLE);
    // when the most significant bits of the address are used, WE is low, CE is low and OE is high

    // prepare to write to the ROM
    dataBusDirOut();
    busClear();

    startSoftwareModeSequence(BUS_CMD_WRITE);

    // 4th bus write cycle, programs the byte
    ADDR_LOW = addr_low;
    ADDR_HIGH = addr_high;
    ADDR_HIGH2 = addr_high2; //its a bit awkward here as the same GPIO (PORTD) is shared with control pins
    CLOCK_DELAY;
    CLOCK_DELAY;
    CLOCK_DELAY;
    writeEnable(); //latch the address
    dataBusWrite(data);
    writeDisable(); //latch the data

    CLOCK_DELAY;
    CLOCK_DELAY;
    outputEnable();


    // Wait for byte program operation to complete (should take 20us)
    delay_us(15);
    waitForToggleBit();
    waitForDataPoll(data);

    chipDisable();

    busClear();
}

void SST39SF020A_sectorErase(uint8_t sector)
{
    // prepare the address to fill with sector to erase
    sector &= 0x3f; //6bit address (A17-A12)
    const uint8_t sector_low = ((sector & 0x0f) << 4);
    uint8_t sector_high = ((sector & 0x30) << 2);
    sector_high |= OUTPUT_ENABLE;
    sector_high &= ~(CHIP_ENABLE | WRITE_ENABLE);
    // when the most significant bits of the address are to be used, WE is low, CE is low and OE is high


    dataBusDirOut();
    busClear();

    startSoftwareModeSequence(BUS_CMD_ERASE);

    // 4th bus write cycle
    ADDR_LOW = 0x55;
    ADDR_HIGH = 0x55;
    CLOCK_DELAY;
    writeEnable(); //latch the address
    dataBusWrite(0xaa);
    CLOCK_DELAY;
    CLOCK_DELAY;
    writeDisable(); //latch the data


    // 5th bus write cycle
    ADDR_LOW = 0xaa;
    ADDR_HIGH = 0x2a;
    CLOCK_DELAY;
    writeEnable(); //latch the address
    dataBusWrite(0x55);
    CLOCK_DELAY;
    CLOCK_DELAY;
    writeDisable(); //latch the data


    // 6th bus cycle
    ADDR_LOW = 0x00;
    ADDR_HIGH = sector_low;
    ADDR_HIGH2 = sector_high;
    CLOCK_DELAY;
    CLOCK_DELAY;
    CLOCK_DELAY;
    writeEnable(); //latch the address
    dataBusWrite(0x30);
    writeDisable(); //latch the data

    //delay_us(1);
    CLOCK_DELAY;
    CLOCK_DELAY;
    outputEnable();

    // Wait for sector erase to complete (should take 25ms)
    delay_ms(20);
    waitForToggleBit();

    chipDisable();

    busClear();
}

void SST39SF020A_chipErase(void)
{
    dataBusDirOut();
    busClear();

    startSoftwareModeSequence(BUS_CMD_ERASE);


    // 4th bus write cycle
    ADDR_LOW = 0x55;
    ADDR_HIGH = 0x55;
    CLOCK_DELAY;
    writeEnable(); //latch the address
    dataBusWrite(0xaa);
    CLOCK_DELAY;
    CLOCK_DELAY;
    writeDisable(); //latch the data


    // 5th bus write cycle
    ADDR_LOW = 0xaa;
    ADDR_HIGH = 0x2a;
    CLOCK_DELAY;
    writeEnable(); //latch the address
    dataBusWrite(0x55);
    CLOCK_DELAY;
    CLOCK_DELAY;
    writeDisable(); //latch the data


    // 6th bus cycle
    ADDR_LOW = 0x55;
    ADDR_HIGH = 0x55;
    CLOCK_DELAY;
    writeEnable(); //latch the address
    dataBusWrite(0x10);
    CLOCK_DELAY;
    writeDisable(); //latch the data

    //delay_us(1);
    CLOCK_DELAY;
    CLOCK_DELAY;
    outputEnable();

    // wait for chip erase to complete (should take 100ms)
    delay_ms(95);
    waitForToggleBit();

    chipDisable();

    busClear();
}

void waitForToggleBit(void)
{
    dataBusDirIn();

    // Compare consecutive toggle bit reads. They will alternate if still erasing.
    uint_fast8_t last, curr = 0;
    do
    {
        outputEnable();
        last = DATA_BUS_READ & ~TOGGLE_BIT;
        outputDisable();

        CLOCK_DELAY;
        CLOCK_DELAY;

        outputEnable();
        curr = DATA_BUS_READ & ~TOGGLE_BIT;
        outputDisable();
    }
    while (last != curr);

    // The erase has completed after 25ms for sector or 100ms for chip
}

/* Wait using data poll
    read data should be 0 during erase, 1 when done.
    or read data is complement of actual data when programming byte */
void waitForDataPoll(uint_fast8_t data)
{
    data &= DATA_POLL_BIT;

    dataBusDirIn();

    // Compare if DQ7 is the complement of the actual data
    uint_fast8_t read = 0;
    do
    {
        outputEnable();
        read = DATA_BUS_READ & DATA_POLL_BIT;
        outputDisable();
    }
    while (data != read);
}
