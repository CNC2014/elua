// STM32 interrupt support

// Generic headers
#include <stdio.h>
#include "platform.h"
#include "platform_conf.h"
#include "elua_int.h"
#include "common.h"

// Platform-specific headers
#include "sim3u1xx.h"
#include "sim3u1xx_Types.h"
//#include <stdio.h>

#ifndef VTMR_TIMER_ID
#define VTMR_TIMER_ID         ( -1 )
#endif

// ****************************************************************************
// Interrupt handlers

static SI32_UART_A_Type* const sim3_uart[] = { SI32_UART_0, SI32_UART_1 };
static SI32_USART_A_Type* const sim3_usart[] = { SI32_USART_0, SI32_USART_1 };

// UART IRQ table
static const u8 usart_irq_table[] = { USART0_IRQn, USART1_IRQn };
static const u8 uart_irq_table[] = { UART0_IRQn, UART1_IRQn };


static void all_usart_irqhandler( int resnum )
{
  if( resnum < 2 )
  {
    while( SI32_USART_A_read_rx_fifo_count( sim3_usart[ resnum ] ) > 0 )
      cmn_int_handler( INT_UART_RX, resnum );

    SI32_USART_A_clear_rx_data_request_interrupt(sim3_usart[ resnum ]);
  }
  else
  {
    while( SI32_UART_A_read_rx_fifo_count( sim3_uart[ resnum - 2 ] ) > 0 )
      cmn_int_handler( INT_UART_RX, resnum );

    SI32_UART_A_clear_rx_data_request_interrupt(sim3_uart[ resnum - 2 ]);
  }
}

void USART0_IRQHandler(void)
{
  if (SI32_USART_A_is_rx_data_request_interrupt_pending(SI32_USART_0))
     all_usart_irqhandler( 0 );
}

void USART1_IRQHandler(void)
{
  if (SI32_USART_A_is_rx_data_request_interrupt_pending(SI32_USART_1))
     all_usart_irqhandler( 1 );
}

void UART0_IRQHandler(void)
{
  if (SI32_UART_A_is_rx_data_request_interrupt_pending(SI32_UART_0))
     all_usart_irqhandler( 2 );
}

void UART1_IRQHandler(void)
{
  if (SI32_UART_A_is_rx_data_request_interrupt_pending(SI32_UART_1))
     all_usart_irqhandler( 3 );
}



void I2C0_IRQHandler(void)
{
  if ( SI32_I2C_A_is_timer3_interrupt_pending( SI32_I2C_0 ) &&
       SI32_I2C_A_is_timer3_interrupt_enabled( SI32_I2C_0 ) )
  {
    //printf("SCLOW");
    SI32_I2C_A_reset_module( SI32_I2C_0 );
  }
  if ( SI32_I2C_A_is_arblost_interrupt_pending( SI32_I2C_0 ) &
       SI32_I2C_A_is_arblost_interrupt_enabled( SI32_I2C_0 ) )
  {
    // Just clear arb lost interrupt
    //printf("ALOST");
    SI32_I2C_A_clear_arblost_interrupt( SI32_I2C_0 );
  }
}



// ****************************************************************************
// Interrupt: INT_UART_RX

static int int_uart_rx_get_status( elua_int_resnum resnum )
{
  if( resnum < 2 )
    return ( int )SI32_USART_A_is_rx_data_request_interrupt_enabled( sim3_usart[ resnum ] );
  else
    return ( int )SI32_UART_A_is_rx_data_request_interrupt_enabled( sim3_uart[ resnum - 2 ] );
}

static int int_uart_rx_set_status( elua_int_resnum resnum, int status )
{
  int prev = int_uart_rx_get_status( resnum );

  if( resnum < 2 )
  {
    if( status == PLATFORM_CPU_ENABLE )
    {
      SI32_USART_A_enable_rx_data_request_interrupt( sim3_usart[ resnum ] );
      NVIC_ClearPendingIRQ( usart_irq_table[ resnum ] );
      NVIC_EnableIRQ( usart_irq_table[ resnum ] );
    }
    else
    {
      SI32_USART_A_disable_rx_data_request_interrupt( sim3_usart[ resnum ] );
      NVIC_DisableIRQ( usart_irq_table[ resnum ] );
    }
  }
  else
  {
    resnum = resnum - 2;
    if( status == PLATFORM_CPU_ENABLE )
    {
      SI32_UART_A_enable_rx_data_request_interrupt( sim3_uart[ resnum ] );
      NVIC_ClearPendingIRQ( uart_irq_table[ resnum ] );
      NVIC_EnableIRQ( uart_irq_table[ resnum ] );
    }
    else
    {
      SI32_UART_A_disable_rx_data_request_interrupt( sim3_uart[ resnum ] );
      NVIC_DisableIRQ( uart_irq_table[ resnum ] );
    }
  }
  return prev;
}

static int int_uart_rx_get_flag( elua_int_resnum resnum, int clear )
{
  int status;

  if( resnum < 2 )
  {
    status = ( int )SI32_USART_A_is_rx_data_request_interrupt_pending( sim3_usart[ resnum ] );
    if( clear )
      SI32_USART_A_clear_rx_data_request_interrupt( sim3_usart[ resnum ] );
  }
  else
  {
    resnum = resnum - 2;
    status = ( int )SI32_UART_A_is_rx_data_request_interrupt_pending( sim3_uart[ resnum ] );

    if( clear )
      SI32_UART_A_clear_rx_data_request_interrupt( sim3_uart[ resnum ] );
  }
  return status;
}

// ****************************************************************************
// Interrupt: INT_UART_BUF_FULL

static int int_uart_buf_full_set_status( elua_int_resnum resnum, int status )
{
  return 1;
}

static int int_uart_buf_full_get_status( elua_int_resnum resnum )
{
  return 1;
}

static int int_uart_buf_full_get_flag( elua_int_resnum resnum, int clear )
{
  return 1;
}

// ****************************************************************************
// Interrupt: INT_UART_BUF_MATCH

static int int_uart_buf_match_set_status( elua_int_resnum resnum, int status )
{
  return 1;
}

static int int_uart_buf_match_get_status( elua_int_resnum resnum )
{
  return 1;
}

static int int_uart_buf_match_get_flag( elua_int_resnum resnum, int clear )
{
  return 1;
}

// ****************************************************************************
// Interrupt: INT_SYSTICK

static int int_systick_set_status( elua_int_resnum resnum, int status )
{
  return 1;
}

static int int_systick_get_status( elua_int_resnum resnum )
{
  return 1;
}

static int int_systick_get_flag( elua_int_resnum resnum, int clear )
{
  return 1;
}

// ****************************************************************************
// Interupt: For callbacks that aren't hardware interrupts

static int callback_set_status( elua_int_resnum resnum, int status )
{
  return 1;
}

static int callback_get_status( elua_int_resnum resnum )
{
  return 1;
}

static int callback_get_flag( elua_int_resnum resnum, int clear )
{
  return 1;
}

// ****************************************************************************
// Initialize interrupt subsystem

static SI32_PBSTD_A_Type* const port_std[] = { SI32_PBSTD_0, SI32_PBSTD_1, SI32_PBSTD_2, SI32_PBSTD_3 };

//#define MATCH_PORTNUM1 3
//#define MATCH_PINNUM1  6

//#define MATCH_PORTNUM2 0
//#define MATCH_PINNUM2  1

struct match_port
{
  u8 port;
  u8 pin;
} ;

#if defined( PCB_V7 ) || defined( PCB_V8 )
  #define MATCH_PORTS 6
  static struct match_port match_config[MATCH_PORTS] = { { 1, 14 }, { 1, 15 }, { 0, 0 } , { 2, 2 } , { 2, 3 } , { 2, 4 } };
#else
  #define MATCH_PORTS 3
  static struct match_port match_config[MATCH_PORTS] = { { 3, 6 }, { 0, 1 }, { 0, 0 } };
#endif
void PMATCH_IRQHandler(void)
{
  int i;
  for(i=0;i<MATCH_PORTS;i++)
  {
    if( ( ~( SI32_PBSTD_A_read_pins(port_std[ match_config[i].port ]) ^ port_std[match_config[i].port]->PM.U32) ) & (1<<(match_config[i].pin)) )
    {
      if( SI32_PBSTD_A_read_pins(port_std[ match_config[i].port ]) & (1<<(match_config[i].pin)) )
      {
        port_std[match_config[i].port]->PM_CLR = (1<<(match_config[i].pin));
        //printf("%i:%i DOWN\n",match_config[i].port, match_config[i].pin);
        button_down(match_config[i].port, match_config[i].pin);
      }
      else
      {
        port_std[match_config[i].port]->PM_SET = (1<<(match_config[i].pin));
        //printf("%i:%i UP\n",match_config[i].port, match_config[i].pin);
        button_up(match_config[i].port, match_config[i].pin);
      }
    }

  }
  //printf("Match Bank 0 %x\n", SI32_PBSTD_A_read_pins(port_std[0]));
  //printf("Match Bank 1 %x\n", SI32_PBSTD_A_read_pins(port_std[1]));
  //printf("Match Bank 2 %x\n", SI32_PBSTD_A_read_pins(port_std[2]));
  //printf("Match Bank 3 %x\n", SI32_PBSTD_A_read_pins(port_std[3]));
  // First Toggle
/*  if( ( ~( SI32_PBSTD_A_read_pins(port_std[ MATCH_PORTNUM1 ]) ^ port_std[MATCH_PORTNUM1]->PM.U32) ) & (1<<MATCH_PINNUM1) )
  {
    if( SI32_PBSTD_A_read_pins(port_std[ MATCH_PORTNUM1 ]) & (1<<MATCH_PINNUM1) )
    {
      port_std[MATCH_PORTNUM1]->PM_CLR = (1<<MATCH_PINNUM1);
      printf("POWER DOWN\n");
      button_down(MATCH_PORTNUM1, MATCH_PINNUM1);
    }
    else
    {
      port_std[MATCH_PORTNUM1]->PM_SET = (1<<MATCH_PINNUM1);
      printf("POWER UP\n");
      button_up(MATCH_PORTNUM1, MATCH_PINNUM1);
    }
  }

  // Second Toggle
  if( ( ~( SI32_PBSTD_A_read_pins(port_std[ MATCH_PORTNUM2 ]) ^ port_std[MATCH_PORTNUM2]->PM.U32) ) & (1<<MATCH_PINNUM2) )
  {
    if( SI32_PBSTD_A_read_pins(port_std[ MATCH_PORTNUM2 ]) & (1<<MATCH_PINNUM2) )
    {
      port_std[MATCH_PORTNUM2]->PM_CLR = (1<<MATCH_PINNUM2);
      printf("CHECKIN DOWN\n");
      button_down(MATCH_PORTNUM2, MATCH_PINNUM2);
    }
    else
    {
      port_std[MATCH_PORTNUM2]->PM_SET = (1<<MATCH_PINNUM2);
      printf("CHECKIN UP\n");
      button_up(MATCH_PORTNUM2, MATCH_PINNUM2);
    }
  }*/
}

void platform_int_init()
{
/*  match_config[0].port = 3;
  match_config[0].pin = 6;
  match_config[1].port = 0;
  match_config[1].pin = 1;
  match_config[2].port = 0;
  match_config[2].pin = 0;*/
  int i;
  for(i=0;i<MATCH_PORTS;i++)
  {
    //printf("Setup port %i:%i\n", match_config[i].port, match_config[i].pin);
    port_std[match_config[i].port]->PMEN_SET = (1<<(match_config[i].pin));
    // Setup register based on power-up state
    if( SI32_PBSTD_A_read_pins(port_std[ match_config[i].port ]) & (1<<(match_config[i].pin)) )
    {
      port_std[match_config[i].port]->PM_CLR = (1<<(match_config[i].pin));
      button_down(match_config[i].port, match_config[i].pin);
    }
    else
      port_std[match_config[i].port]->PM_SET = (1<<(match_config[i].pin));
  }
/*  // Set up first match
  port_std[MATCH_PORTNUM1]->PMEN_SET = (1<<MATCH_PINNUM1);
  //port_std[MATCH_PORTNUM1]->PM_SET = (1<<MATCH_PINNUM1);
  // Setup register based on power-up state
  if( SI32_PBSTD_A_read_pins(port_std[ MATCH_PORTNUM1 ]) & (1<<MATCH_PINNUM1) )
  {
    port_std[MATCH_PORTNUM1]->PM_CLR = (1<<MATCH_PINNUM1);
    button_down(MATCH_PORTNUM1, MATCH_PINNUM1);
  }
  else
    port_std[MATCH_PORTNUM1]->PM_SET = (1<<MATCH_PINNUM1);


  // Set up second match
  port_std[MATCH_PORTNUM2]->PMEN_SET = (1<<MATCH_PINNUM2);
  //port_std[MATCH_PORTNUM2]->PM_SET = (1<<MATCH_PINNUM2);
  // Setup register based on power-up state
  if( SI32_PBSTD_A_read_pins(port_std[ MATCH_PORTNUM2 ]) & (1<<MATCH_PINNUM2) )
  {
    port_std[MATCH_PORTNUM2]->PM_CLR = (1<<MATCH_PINNUM2);
    button_down(MATCH_PORTNUM2, MATCH_PINNUM2);
  }
  else
    port_std[MATCH_PORTNUM2]->PM_SET = (1<<MATCH_PINNUM2);*/

  NVIC_ClearPendingIRQ( PMATCH_IRQn );
  NVIC_EnableIRQ( PMATCH_IRQn );
}

// ****************************************************************************
// Interrupt table
// Must have a 1-to-1 correspondence with the interrupt enum in platform_conf.h!

const elua_int_descriptor elua_int_table[ INT_ELUA_LAST ] =
{
  { int_uart_rx_set_status, int_uart_rx_get_status, int_uart_rx_get_flag }, // INT_UART_RX
  { int_uart_buf_full_set_status, int_uart_buf_full_get_status, int_uart_buf_full_get_flag }, // INT_UART_BUF_FULL
  { int_uart_buf_match_set_status, int_uart_buf_match_get_status, int_uart_buf_match_get_flag }, // INT_UART_BUF_MATCH
  { int_systick_set_status, int_systick_get_status, int_systick_get_flag }, // INT_SYSTICK
  { callback_set_status, callback_get_status, callback_get_flag }, // INT_IRIDIUM_SIGNAL
  { callback_set_status, callback_get_status, callback_get_flag }, // INT_IRIDIUM_TX_OK
  { callback_set_status, callback_get_status, callback_get_flag }, // INT_IRIDIUM_TX_FAIL
  { callback_set_status, callback_get_status, callback_get_flag }, // INT_IRIDIUM_TIMEOUT
  { callback_set_status, callback_get_status, callback_get_flag }, // INT_GPS_VALID
  { callback_set_status, callback_get_status, callback_get_flag }, // INT_GPS_TIMEOUT
  { callback_set_status, callback_get_status, callback_get_flag }, // INT_BOOT
  { callback_set_status, callback_get_status, callback_get_flag }, // INT_CONTENTION
  { callback_set_status, callback_get_status, callback_get_flag }, // INT_SYSINIT
  { callback_set_status, callback_get_status, callback_get_flag }, // INT_UART_BUF_HALF_FULL
  { callback_set_status, callback_get_status, callback_get_flag }, // INT_TICKSECOND
};
