#include "atmega.h"
#include <avr/interrupt.h>

// microsecond delay
void delay_us(unsigned int time)
{
    while (time--)
    {
        //_delay_us(1);

        /* avr's delay function is faulty
         so I'll make my own.
         at 12MHz clock, 1 clock happens every 83ns.
         we need 12 clocks to take 1 us.
         a few of the clocks are taken by the comparison of values (x2), int decrements (x2) and the while loop (a branch or jump)
         */

         __asm__ ("\tnop\n\t"
                  "nop\n\t"
                  "nop\n\t"
                  "nop\n\t"
                  "nop\n\t"
                  "nop\n\t"
                  "nop\n");
    }
}

// millisecond delay
void delay_ms(unsigned int time)
{
    while (time--)
    {
        delay_us(1000);
    }
}


// configure the UART hardware
void UART_setup(uint32_t baudrate)
{
    /* Set frame format: 8data, 1stop bit  */
    UCSRC = (1<<URSEL)|(3<<UCSZ0);

    //set the baud rate
    UBRRL = BAUDRATE_LOW(baudrate);
    UBRRH = BAUDRATE_HIGH(baudrate);

    //enable tx and rx
    UCSRB = (1<<RXEN)|(1<<TXEN);

    #ifdef USE_ISR
    UCSRB |= (1<<RXCIE); // Enable the USART Receive Complete interrupt (USART_RXC)
	sei(); // Enable the Global Interrupt Enable flag so that interrupts can be processed
	#endif
}

// uart communications
static inline void UART_Transmit(unsigned char data)
{
    /* Wait for empty transmit buffer */
    while ( !( UCSRA & (1<<UDRE)) );

    /* Put data into buffer, sends the data */
    UDR = data;
}

static inline unsigned char UART_Receive(void)
{
    /* Wait for data to be received */
    while ( !(UCSRA & (1<<RXC)) );

    /* Get and return received data from buffer */
    return UDR;
}


// helper for printf
int put_char(char c, FILE* stream)
{
    UART_Transmit(c);

    return 0;
}



// my own fgets like function (read up to maxlength bytes from the circular buffer)
// length = number of expected chars + null terminator
void UART_readString(char* buf, uint8_t length)
{
    char data = 0;
    char* cur = buf;
    //length--;

    for (uint8_t i = 0; i < length; i++)
    {
        data = UART_Receive();
        UART_Transmit(data); //echo back to PC

        if (data < 0x20)// || data > 0x7e)
        {
            // The first invalid char will terminate the string early
            break;
        }
        else
        {
            *cur = data;
        }
        cur++;
    }
    *cur = 0;
}

// interrupt handler
#ifdef USE_ISR
ISR(USART_RXC_vect)
{
    uint8_t data = UART_Receive();
    UART_Transmit(data); //echo back to PC
}
#endif // USE_ISR
