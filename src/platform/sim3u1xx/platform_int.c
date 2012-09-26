// STM32 interrupt support

// Generic headers
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

#define MATCH_PORTNUM1 3
#define MATCH_PINNUM1  8

#define MATCH_PORTNUM2 0
#define MATCH_PINNUM2  1


void PMATCH_IRQHandler(void) 
{
  // First Toggle
  if( ( ~( SI32_PBSTD_A_read_pins(port_std[ MATCH_PORTNUM1 ]) ^ port_std[MATCH_PORTNUM1]->PM.U32) ) & (1<<MATCH_PINNUM1) )
  {
    if( SI32_PBSTD_A_read_pins(port_std[ MATCH_PORTNUM1 ]) & (1<<MATCH_PINNUM1) )
    {
      port_std[MATCH_PORTNUM1]->PM_CLR = (1<<MATCH_PINNUM1);

      // Do something on high transition for first
    }
    else
    {
      port_std[MATCH_PORTNUM1]->PM_SET = (1<<MATCH_PINNUM1);

      // Do something on low transition for first
      //printf("PMATCH LOW 1\n");
    }
  }


  // Second Toggle
  if( ( ~( SI32_PBSTD_A_read_pins(port_std[ MATCH_PORTNUM2 ]) ^ port_std[MATCH_PORTNUM2]->PM.U32) ) & (1<<MATCH_PINNUM2) )
  {
    if( SI32_PBSTD_A_read_pins(port_std[ MATCH_PORTNUM2 ]) & (1<<MATCH_PINNUM2) )
    {
      port_std[MATCH_PORTNUM2]->PM_CLR = (1<<MATCH_PINNUM2);

      // Do something on high transition for second
    }
    else
    {
      port_std[MATCH_PORTNUM2]->PM_SET = (1<<MATCH_PINNUM2);

      // Do something on low transition for second
      //printf("PMATCH LOW 2\n");

    }
  }
}

void platform_int_init()
{
  // Set up first match
  port_std[MATCH_PORTNUM1]->PMEN_SET = (1<<MATCH_PINNUM1);
  port_std[MATCH_PORTNUM1]->PM_SET = (1<<MATCH_PINNUM1);

  // Set up second match
  port_std[MATCH_PORTNUM2]->PMEN_SET = (1<<MATCH_PINNUM2);
  port_std[MATCH_PORTNUM2]->PM_SET = (1<<MATCH_PINNUM2);

  NVIC_ClearPendingIRQ( PMATCH_IRQn );
  NVIC_EnableIRQ( PMATCH_IRQn );
}

// ****************************************************************************
// Interrupt table
// Must have a 1-to-1 correspondence with the interrupt enum in platform_conf.h!

const elua_int_descriptor elua_int_table[ INT_ELUA_LAST ] =
{
  { int_uart_rx_set_status, int_uart_rx_get_status, int_uart_rx_get_flag },
  { int_uart_buf_full_set_status, int_uart_buf_full_get_status, int_uart_buf_full_get_flag },
  { int_uart_buf_match_set_status, int_uart_buf_match_get_status, int_uart_buf_match_get_flag },
  { int_systick_set_status, int_systick_get_status, int_systick_get_flag },
  { callback_set_status, callback_get_status, callback_get_flag },
  { callback_set_status, callback_get_status, callback_get_flag },
  { callback_set_status, callback_get_status, callback_get_flag }
};
