#include <avr/io.h>
#include <stdio.h>
#include "slimmemeter.h"

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

