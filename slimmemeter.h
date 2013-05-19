/*CRC */

/* Data buffers */
#define OUTBUFFERSIZE 128

volatile uint8_t out_buffer[OUTBUFFERSIZE];
volatile uint8_t out_buffer_ptr;
volatile uint8_t key;
volatile uint8_t state;
volatile uint8_t maj;
volatile uint8_t min;
volatile uint8_t smin;
volatile uint16_t value;


/* State Machine */
volatile uint8_t slimmemeter_state;


/* State Machine states */
#define IDLE 0x00
#define START 0x10
#define CODE 0x20
#define DATA 0x30
#define M3 0x40

//Process serial data
void slimmemeter_receive( uint8_t c );
void do_read(void);

