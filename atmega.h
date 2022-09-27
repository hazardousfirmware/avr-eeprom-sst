#ifndef ATMEGA_H_INCLUDED
#define ATMEGA_H_INCLUDED

#ifndef F_CPU
#define F_CPU 12000000UL // or whatever may be your frequency
#endif

#include <avr/io.h>

// Serial port baud rate
#define BAUD 57600

// Calculate the baud rate and split into high and low registers (8 + 4 bit)
#define BAUDRATE_LOW(A) ((F_CPU)/(A*16UL)-1)
#define BAUDRATE_HIGH(B) (BAUDRATE_LOW(B)>>8)

void delay_us(unsigned int time);
void delay_ms(unsigned int time);

#define CLOCK_DELAY __asm__("nop")

void UART_setup(uint32_t baudrate);
//void UART_Transmit(unsigned char data);
//unsigned char UART_Receive(void);

#ifdef USE_ISR
ISR(USART_RXC_vect);
#endif

// for printf functions
#include <stdio.h>
int put_char(char c, FILE *stream);


// serial reading
#define BUFFER_SIZE 256
void UART_readString(char* buf, uint8_t maxlength);

#endif // ATMEGA_H_INCLUDED

