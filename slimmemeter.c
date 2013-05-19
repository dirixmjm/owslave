/*
 *  Copyright © 2010, Matthias Urlichs <matthias@urlichs.de>
 *  Copyright © 2012, Marc Dirix <marc@dirix.nu>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License (included; see the file LICENSE)
 *  for more details.
 */

/* This code implements (some of) the DS2502 memory code.
 */

#include "onewire.h"
#include "slimmemeter.h" 

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>

#define C_READ_DATA      0xC3 
#define C_STATUS      0xAA 

#define UART0_RECEIVE_INTERRUPT   USART_RX_vect
#define UART0_TRANSMIT_INTERRUPT  USART_UDRE_vect
#define UART0_STATUS   UCSRA
#define UART0_CONTROL  UCSRB
#define UART0_DATA     UDR
#define UART0_UDRIE    UDRIE
#define BAUD 9600
#define BAUDRATE (F_CPU / 16 / BAUD )-1

extern volatile uint8_t out_buffer[OUTBUFFERSIZE];

void do_read(void)
{
	uint8_t crc = 0;
	uint16_t adr;
	uint8_t b;

	recv_byte();
	crc = crc8(crc,0xC3); //FIXME, maybe predefine?
	b = recv_byte_in();
	recv_byte();
	crc = crc8(crc,b);
	adr = b;
	b = recv_byte_in();
	crc = crc8(crc,b);
	adr |= b<<8;
	xmit_byte(crc);
        crc = 0;
#define XMIT(val) do {                                     \
		xmit_byte(val);                            \
		crc = crc8(crc,val);                      \
	} while(0)
        
	while(1)
        {
           if( adr < OUTBUFFERSIZE )
              XMIT(out_buffer[adr]);
           else
	      XMIT(0xFF);
           adr++;
           if((adr%0x20) == 0 )
           {
              xmit_byte(crc);
              crc = 0;
           }
        }
        while(1)
           xmit_bit(1);
}

void do_command(uint8_t cmd)
{
        switch(cmd)
        {
           case C_READ_DATA: 
              do_read();
              break;
           default:           
		DBG_P("?CI");
		DBG_X(cmd);
		set_idle();
        }
}

void update_idle(uint8_t bits)
{
}

void init_state(void)
{
//Init UART for slimmemeter operation this is not for debugging.
    UBRRH = (unsigned char)(BAUDRATE>>8);
    UBRRL = (unsigned char) BAUDRATE;
    //UART0_CONTROL = _BV(RXCIE)|(1<<RXEN)|(1<<TXEN);
    UART0_CONTROL = _BV(RXCIE)| _BV(RXEN);
    // Set frame format: asynchronous, 8data, no parity, 1stop bit 
    UCSRC = _BV(UCSZ1) | _BV(UPM1);

    //PD5 output and pullup
    DDRD |= _BV(PD5);
    PORTD |= _BV(PD5);
}

SIGNAL(UART0_RECEIVE_INTERRUPT)
{
   unsigned char data;
   data = UART0_DATA;
   slimmemeter_receive(data);
}

void slimmemeter_receive( uint8_t c )
{
   if (  c == 0x0A )
   {
      if ( key == M3 )
      {
         state = M3;
         key = 0;
         return;
      }
   
      maj=0;min=0;smin=0;key=0;
      state=START;
      return;
   }

   switch(state & 0xF0)
   {
      case IDLE:
         return;
         break;
      case START:
         if( state == 0x10 )
         {
            if ( c == 0x30 || c == 0x31 )
              state++;
            else
              state=IDLE;
            //Synchrosize start of new message on "/KMP5"
//            if( c == 0x2f || c == 0x21 )
//              out_buffer_ptr = 0;
            
         }
         else if( state == 0x11 )
         {
            if ( c == 0x2d )
               state++;
            else
               state=IDLE;
         }
         else if( state == 0x12 )
         {
            if ( c == 0x30 || c == 0x31 )
              state++;
            else
              state=IDLE;
         }
         else if( state == 0x13 )
         {
            if( c == 0x3a )
               state=CODE;
            else
              state=IDLE;
         }
         break;
      case CODE:
         if( state == 0x20)
         {
            if( c == 0x2e )
              state++;
            else
              maj=10*maj+(c-0x30);
         }
         else if( state == 0x21 )
         {
            if( c == 0x2e )
              state++;
            else
              min=10*min+(c-0x30);
         }
         else if( state == 0x22 )
         {
            if( c != 0x28 )
              smin=10*smin+(c-0x30);
            else
            {
               if( maj == 24 && min == 3 && smin == 0 )
               {
                  out_buffer_ptr = 0x60;
                  key = M3;
                  state = IDLE;
               }
               else if( maj == 1 && min == 8 && smin == 1)
               {
                  state=DATA;
                  out_buffer_ptr = 0x00;
               }
               else if ( maj ==1 && min == 8 && smin == 2)
               {
                  state=DATA;
                  out_buffer_ptr = 0x10;
               }
               else if ( maj ==2 && min == 8 && smin == 1)
               {
                  state=DATA;
                  out_buffer_ptr = 0x20;
               }
               else if ( maj ==2 && min == 8 && smin == 2)
               {
                  state=DATA;
                  out_buffer_ptr = 0x30;
               }
               else if ( maj ==1 && min == 7 && smin == 0)
               {
                  state=DATA;
                  out_buffer_ptr = 0x40;
               }
               else if ( maj ==2 && min == 7 && smin == 0)
               {
                  state=DATA;
                  out_buffer_ptr = 0x50;
               }
               else
               {
                  state=IDLE;
                  return;
               }
               out_buffer[out_buffer_ptr++] = maj;
               out_buffer[out_buffer_ptr++] = min;
               out_buffer[out_buffer_ptr++] = smin;
               out_buffer[out_buffer_ptr++] = 0xFF;

            }
         }
         break;
      case DATA:
         if ( min == 7 || min == 8)
         {
            if ( c == 0x2a )
            {
//               out_buffer[out_buffer_ptr++] = (maj*min*smin)&0xFF;
               state=IDLE;
            }
            else if ( c != 0x2e )
            {
               //value = 10*value + (c-0x30);
               out_buffer[out_buffer_ptr++] = (c-0x30)&0xFF;
            }
            else 
               out_buffer[out_buffer_ptr++] = c;
         }
         break;
      case M3:
         if( state == 0x40 )
         {
            if( c == 0x28 )
               state++;
            else
               state=IDLE;
         }
         else if( state == 0x41 )
         {
            if ( c == 0x29 )
            {
//               out_buffer[out_buffer_ptr] = (maj*min*smin)&0xFF;
               state=IDLE;
               out_buffer_ptr = 0;
            }
            else if ( c != 0x2e )
            {
               out_buffer[out_buffer_ptr++] = (c-0x30)&0xFF;
            }
            else 
               out_buffer[out_buffer_ptr++] = c;
         }
         break;
   }
}

