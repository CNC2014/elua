// Platform-dependent functions

#include "platform.h"
#include "type.h"
#include "devman.h"
#include "genstd.h"
#include "stacks.h"
#include <reent.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "utils.h"
#include "common.h"
#include "platform_conf.h"
#include "buf.h"
#include "tsi.h"
#include "lrotable.h"

// Platform includes
#include "MK60N512VMD100.h"
#include "sysinit.h"
#include "uart.h"
#include "arm_cm4.h"

extern int core_clk_mhz;
extern int core_clk_khz;
extern int periph_clk_khz;

static void uarts_init();
static void timers_init();
static void gpios_init();
static void pwms_init();
static void tsi_init();

// ****************************************************************************
// Platform initialization

int platform_init()
{
  sysinit();
  
  gpios_init();
  uarts_init();
  timers_init();
  pwms_init();
  tsi_init();
  
  // Common platform initialization code
  cmn_platform_init();

  return PLATFORM_OK;
} 

// *****************************************************************************
// PIO functions

static GPIO_MemMapPtr const pgpios[ NUM_PIO ] = { PTA_BASE_PTR, PTB_BASE_PTR, PTC_BASE_PTR, PTD_BASE_PTR, PTE_BASE_PTR };
static PORT_MemMapPtr const ports[ NUM_PIO ] = { PORTA_BASE_PTR, PORTB_BASE_PTR, PORTC_BASE_PTR, PORTD_BASE_PTR, PORTE_BASE_PTR };

void gpios_init()
{
  unsigned i, j;
  PORT_MemMapPtr port;
  
  SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK;  
  SIM_SCGC5 |= SIM_SCGC5_PORTC_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTD_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;
  // Set all pins to GPIO
  for( i = 0; i < NUM_PIO; i ++ )
  {
    port = ports[ i ];
    for( j = 0; j < 32; j ++ )
      PORT_PCR_REG( port, j ) |= PORT_PCR_MUX( 1 );
  }    
}

pio_type platform_pio_op( unsigned port, pio_type pinmask, int op )
{
  pio_type retval = 1;
  GPIO_MemMapPtr base = pgpios[ port ];

  switch( op )
  {
    case PLATFORM_IO_PORT_SET_VALUE:
      GPIO_PDOR_REG( base ) = pinmask;
      break;

    case PLATFORM_IO_PIN_SET:
      GPIO_PSOR_REG( base ) = pinmask;
      break;

    case PLATFORM_IO_PIN_CLEAR:
      GPIO_PCOR_REG( base ) = pinmask;
      break;

    case PLATFORM_IO_PORT_DIR_INPUT:
      GPIO_PDDR_REG( base ) = 0;
      break;
      
    case PLATFORM_IO_PIN_DIR_INPUT:
      GPIO_PDDR_REG( base ) &= ~pinmask;
      break;

    case PLATFORM_IO_PORT_DIR_OUTPUT:
      GPIO_PDDR_REG( base ) = 0xFFFFFFFF;
      break;
      
    case PLATFORM_IO_PIN_DIR_OUTPUT:
      GPIO_PDDR_REG( base ) |= pinmask;
      break;

    case PLATFORM_IO_PORT_GET_VALUE:
      retval = GPIO_PDIR_REG( base );
      break;

    case PLATFORM_IO_PIN_GET:
      retval = ( GPIO_PDIR_REG( base ) & pinmask ) ? 1 : 0;
      break;

    default:
      retval = 0;
      break;
  }
  return retval;
}

// *****************************************************************************
// UART functions

static UART_MemMapPtr const puarts[ NUM_UART ] = { UART0_BASE_PTR, UART1_BASE_PTR, UART2_BASE_PTR, UART3_BASE_PTR, UART4_BASE_PTR, UART5_BASE_PTR };
static const u8 uart_ports[ NUM_UART ] = { 3, 2, 3, 2, 4 };
static const u8 uart_tx_pins[ NUM_UART ] = { 6, 4, 3, 17, 8 };
static const u8 uart_rx_pins[ NUM_UART ] = { 7, 3, 2, 16, 9 }; 

// Initialize UART clocks
static void uarts_init()
{
  SIM_SCGC4 |= SIM_SCGC4_UART0_MASK;
  SIM_SCGC4 |= SIM_SCGC4_UART1_MASK;
  SIM_SCGC4 |= SIM_SCGC4_UART2_MASK;
  SIM_SCGC4 |= SIM_SCGC4_UART3_MASK;
  SIM_SCGC1 |= SIM_SCGC1_UART4_MASK;
  SIM_SCGC1 |= SIM_SCGC1_UART5_MASK;
}

u32 platform_uart_setup( unsigned id, u32 baud, int databits, int parity, int stopbits )
{
  PORT_MemMapPtr uart_port = ports[ uart_ports[ id ] ]; 
  UART_MemMapPtr puart = puarts[ id ];
  u16 ubd;
  int sysclk = id == 0 || id == 1 ? core_clk_khz * 1000 : periph_clk_khz * 1000;
  
  // Setup pins first
  uart_port->PCR[ uart_tx_pins[ id ] ] = PORT_PCR_MUX( 0x03 );
  uart_port->PCR[ uart_rx_pins[ id ] ] = PORT_PCR_MUX( 0x03 );
      
  // Disable receiver and transmitter
  UART_C2_REG( puart ) &= ~( UART_C2_TE_MASK | UART_C2_RE_MASK );

  // Configure the UART for 8-bit mode, no parity
  UART_C1_REG( puart ) = 0;
  
  // Check parity
  if( parity != PLATFORM_UART_PARITY_NONE )
  {
    UART_C1_REG( puart ) |= UART_C1_PE_MASK;
    if( parity == PLATFORM_UART_PARITY_ODD )
      UART_C1_REG( puart ) |= UART_C1_PT_MASK;
  }
  
  // Check data bits
  if( databits == 9 )
    UART_C1_REG( puart ) |= UART_C1_M_MASK;
    
  // Calculate baud settings
  ubd = ( u16 )( sysclk / ( baud * 16 ) );   
  UART_BDH_REG( puart ) |= UART_BDH_SBR( ( ubd & 0x1F00 ) >> 8 );
  UART_BDL_REG( puart ) = ( u8 )( ubd & UART_BDL_SBR_MASK );

  // Enable receiver and transmitter
	UART_C2_REG( puart ) |= ( UART_C2_TE_MASK | UART_C2_RE_MASK );    
      
  return baud;
}

void platform_s_uart_send( unsigned id, u8 data )
{
  uart_putchar( puarts[ id ], data );
}

int platform_s_uart_recv( unsigned id, timer_data_type timeout )
{
  UART_MemMapPtr uart = puarts[ id ];
  
  if( timeout == 0 )
    return uart_getchar_present( uart ) ? uart_getchar( uart ) : -1;
  return uart_getchar( uart );
}

int platform_s_uart_set_flow_control( unsigned id, int type )
{
  return PLATFORM_ERR;
}

// *****************************************************************************
// Timer operations
// On Kinetis these are implemented using the PITs, not the FTMs

static void timers_init()
{
  SIM_SCGC6 |= SIM_SCGC6_PIT_MASK;
  PIT_MCR_REG( PIT_BASE_PTR ) &= ~PIT_MCR_MDIS_MASK;
}

void platform_s_timer_delay( unsigned id, timer_data_type delay_us )
{
  // Disable timer first
  PIT_TCTRL_REG( PIT_BASE_PTR, id ) &= ~PIT_TCTRL_TEN_MASK;
  // Clear interrupt flag
  PIT_TFLG_REG( PIT_BASE_PTR, id ) |= PIT_TFLG_TIF_MASK;
  // Compute timeout and load it into LDVAL
  PIT_LDVAL_REG( PIT_BASE_PTR, id ) = ( ( u64 )delay_us * periph_clk_khz ) / 1000;
  // Enable timer and wait for interrupt flag
  PIT_TCTRL_REG( PIT_BASE_PTR, id ) |= PIT_TCTRL_TEN_MASK;
  while( ( PIT_TFLG_REG( PIT_BASE_PTR, id ) & PIT_TFLG_TIF_MASK ) == 0 );  
}

timer_data_type platform_s_timer_op( unsigned id, int op, timer_data_type data )
{
  u32 res = 0;

  data = data;
  switch( op )
  {
    case PLATFORM_TIMER_OP_START:
      PIT_TCTRL_REG( PIT_BASE_PTR, id ) &= ~PIT_TCTRL_TEN_MASK;
      PIT_LDVAL_REG( PIT_BASE_PTR, id ) = 0xFFFFFFFF;
      PIT_TCTRL_REG( PIT_BASE_PTR, id ) |= PIT_TCTRL_TEN_MASK;
      res = 0xFFFFFFFF;
      break;

    case PLATFORM_TIMER_OP_READ:
      res = PIT_CVAL_REG( PIT_BASE_PTR, id );
      break;

    case PLATFORM_TIMER_OP_GET_MAX_DELAY:
      res = platform_timer_get_diff_us( id, 0, 0xFFFFFFFF );
      break;

    case PLATFORM_TIMER_OP_GET_MIN_DELAY:
      res = platform_timer_get_diff_us( id, 0, 1 );    
      break;

    case PLATFORM_TIMER_OP_SET_CLOCK:
    case PLATFORM_TIMER_OP_GET_CLOCK:    
      res = periph_clk_khz * 1000;
      break;

    case PLATFORM_TIMER_OP_GET_MAX_CNT:
      res = 0xFFFFFFFF;
      break;
  }
  return res;
}

int platform_s_timer_set_match_int( unsigned id, timer_data_type period_us, int type )
{
  return PLATFORM_TIMER_INT_INVALID_ID;
}

// u64 platform_timer_sys_raw_read()
// {
//   return SysTick->LOAD - SysTick->VAL;
// }

// void platform_timer_sys_disable_int()
// {
//   SysTick->CTRL &= ~( 1 << SysTick_CTRL_TICKINT_Pos );
// }

// void platform_timer_sys_enable_int()
// {
//   SysTick->CTRL |= 1 << SysTick_CTRL_TICKINT_Pos;
// }

// timer_data_type platform_timer_read_sys()
// {
//   return cmn_systimer_get();
// }

// *****************************************************************************
// PWM functions
// The current PWM channels are allocated as follows:
// 2 channels on FTM0 - 0 (PA3) and 1 (PA4)
// 2 channels on FTM1 - 0 (PA8) and 1 (PA9)
// 2 channels on FTM2 - 0 (PA10) and 1 (PA11)
// This is almost arbitrary and it might change in future versions

#define PWM_PORT              0
static FTM_MemMapPtr const pwms[] = { FTM0_BASE_PTR, FTM1_BASE_PTR, FTM2_BASE_PTR };
static const u8 pwm_pins[] = { 3, 4, 8, 9, 10, 11 };

static void pwms_init()
{
  FTM_MemMapPtr tmr;
  unsigned i;
  
  // Enable clocks
  SIM_SCGC3 |= SIM_SCGC3_FTM2_MASK;
  SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK;
  SIM_SCGC6 |= SIM_SCGC6_FTM1_MASK;
  // Set FTM clocks to system clock
  for( i = 0; i < sizeof( pwms ) / sizeof( FTM_MemMapPtr ); i ++ )
  {
    tmr = pwms[ i ];
    FTM_SC_REG( tmr ) &= ~FTM_SC_CLKS_MASK;
    FTM_SC_REG( tmr ) |= 1 << FTM_SC_CLKS_SHIFT;
    FTM_MODE_REG( tmr ) |= FTM_MODE_FTMEN_MASK;
    FTM_CnSC_REG( tmr, 0 ) |= FTM_CnSC_MSB_MASK;
    FTM_CnSC_REG( tmr, 1 ) |= FTM_CnSC_MSB_MASK;    
  }
}

// Helper function: return the PWM clock
u32 platform_pwm_get_clock( unsigned id )
{
  FTM_MemMapPtr tmr = pwms[ id >> 1 ];
  
  return ( periph_clk_khz * 1000 ) / ( 1 << ( FTM_SC_REG( tmr ) & FTM_SC_PS_MASK ) );
}

// Helper function: set the PWM clock
u32 platform_pwm_set_clock( unsigned id, u32 clock )
{
  FTM_MemMapPtr tmr = pwms[ id >> 1 ];
  u32 baseclk = periph_clk_khz * 1000;
  unsigned i, mini;
  
  for( i = 1, mini = 0; i < 8; i ++ )
    if( ABSDIFF( baseclk / ( 1 << i ), clock ) < ABSDIFF( baseclk / ( 1 << mini ), clock ) )
      mini = i;
  FTM_SC_REG( tmr ) &= ~FTM_SC_PS_MASK;
  FTM_SC_REG( tmr ) |= mini;        
  return platform_pwm_get_clock( id );
}

u32 platform_pwm_setup( unsigned id, u32 frequency, unsigned duty )
{
  FTM_MemMapPtr tmr = pwms[ id >> 1 ];
  int ch = id & 1;
  u32 basefreq = platform_pwm_get_clock( id );
  u32 modval;
  
  // Stop timer while the updates are made
  FTM_SC_REG( tmr ) &= ~FTM_SC_CLKS_MASK;
  modval = basefreq / frequency;
  FTM_MOD_REG( tmr ) = modval - 1;
  FTM_CnV_REG( tmr, ch ) = ( duty * modval ) / 100;
  FTM_CNT_REG( tmr ) = 0; 
  // Restart timer 
  FTM_SC_REG( tmr ) |= 1 << FTM_SC_CLKS_SHIFT;  
  return basefreq / modval;
}

void platform_pwm_start( unsigned id )
{
  FTM_MemMapPtr tmr = pwms[ id >> 1 ];
  int ch = id & 1;
  PORT_MemMapPtr pwm_port = ports[ PWM_PORT ];   
  pwm_port->PCR[ pwm_pins[ id ] ] = PORT_PCR_MUX( 0x03 );
  FTM_CnSC_REG( tmr, ch ) |= FTM_CnSC_ELSB_MASK;
}

void platform_pwm_stop( unsigned id )
{
  FTM_MemMapPtr tmr = pwms[ id >> 1 ];
  int ch = id & 1;
  PORT_MemMapPtr pwm_port = ports[ PWM_PORT ];   
  FTM_CnSC_REG( tmr, ch ) &= ~FTM_CnSC_ELSB_MASK;    
  pwm_port->PCR[ pwm_pins[ id ] ] = PORT_PCR_MUX( 0x01 );   
}

// *****************************************************************************
// CPU specific functions
 
u32 platform_s_cpu_get_frequency()
{
  return core_clk_mhz * 1000000;
}

// *****************************************************************************
// TSI specific functions

static const u8 tsi_pins[] =  { 0, 0, 1, 2, 3, 4, 1, 2, 3, 16, 17, 18, 19, 0, 1, 2 };
static const u8 tsi_ports[] = { 1, 0, 0, 0, 0, 0, 1, 1, 1, 1,  1,  1,  1, 2, 2, 2 };


static void tsi_init()
{
  // Enable clock
  SIM_SCGC5 |= SIM_SCGC5_TSI_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;
}


void kin_tsi_init( unsigned id )
{


  //

}


u16 kin_tsi_read( unsigned id )
{
  u16 res = 0;
  u32 i;

  // Disable TSI
  TSI0_GENCS &= ~TSI_GENCS_TSIEN_MASK;
  
  //Make sure no pending scans are running
  while( ( TSI0_GENCS & TSI_GENCS_SCNIP_MASK ) );

  // Set electrode pin to analog
  PORT_PCR_REG( ports[ tsi_ports[ id ] ], tsi_pins[ id ] ) = PORT_PCR_MUX( 0 );
  
  
  // 4 uA external current, 32 uA ref current, 600mV delta
  TSI_SCANC_REG(TSI0_BASE_PTR) |= ( TSI_SCANC_EXTCHRG( 3 ) | TSI_SCANC_REFCHRG( 31 ) |
                                    TSI_SCANC_DELVOL( 7 )  | TSI_SCANC_SMOD( 0 ) | 
                                    TSI_SCANC_AMPSC( 0 ) );

  TSI_GENCS_REG(TSI0_BASE_PTR) |= TSI_GENCS_PS( 2 ) | TSI_GENCS_NSCN( 10 );

  // enable electrode pin
  TSI0_PEN = ( ( u32 )1 ) << id;

  TSI_THRESHLD_REG(TSI0_BASE_PTR, id) = TSI_THRESHLD_HTHH(0xFF) | TSI_THRESHLD_LTHH(0x00);

  // Enable TSI
  TSI0_GENCS |=  TSI_GENCS_TSIEN_MASK;

  // Start scan & wait for completion
  TSI0_GENCS |=  TSI_GENCS_SWTS_MASK;

  //TSI0_GENCS |= TSI_GENCS_STM_MASK;
  while( ! ( TSI0_GENCS & TSI_GENCS_EOSF_MASK ) );

  // Errata for 0M33Z parts: EOSF flag may get set 0.25 ms too early
  for(i=250000;i;i--);

  //printf ("TSI0_STATUS = %x\n", (*((volatile unsigned  *)(0x4004500C))));
  //printf ("TSI0_GENCS = %x\n", (*((volatile unsigned  *)(0x40045000))));

  res = *(( u16* )&TSI_CNTR1_REG( TSI0_BASE_PTR ) + id);

  TSI0_STATUS = 0xFFFFFFFF;

  // Disable TSI
  TSI0_GENCS &= ~TSI_GENCS_TSIEN_MASK;


  return res;
}


/*
u32 platform_tsi_op( unsigned id, int op, u32 data )
{
  u32 res = 0;

  data = data;
  switch( op )
  {
    case PLATFORM_TSI_OP_ENABLE:
      PORT_PCR_REG( PORTA_BASE_PTR, tsi_pins[ id ] ) |= PORT_PCR_MUX( MUX_ALT0 );
      TSI_PEN_REG( TSI0_BASE_PTR ) |= 1 << id;
      break;

    case PLATFORM_TSI_OP_DISABLE:
      TSI_PEN_REG( TSI0_BASE_PTR ) &= ~(1 << ( id & 0xF ));
      break;

    case PLATFORM_TSI_OP_SET_THRESHOLD:
      TSI_THRESHLD_REG(TSI0_BASE_PTR, id) = data


    case PLATFORM_TIMER_OP_READ:
      res = PIT_CVAL_REG( PIT_BASE_PTR, id );
      break;

    case PLATFORM_TIMER_OP_GET_MAX_DELAY:
      res = platform_timer_get_diff_us( id, 0, 0xFFFFFFFF );
      break;

    case PLATFORM_TIMER_OP_GET_MIN_DELAY:
      res = platform_timer_get_diff_us( id, 0, 1 );    
      break;

    case PLATFORM_TIMER_OP_SET_CLOCK:
    case PLATFORM_TIMER_OP_GET_CLOCK:    
      res = periph_clk_khz * 1000;
      break;
  }
  return res;
  }*/


#ifdef ENABLE_TSI

#define MIN_OPT_LEVEL 2
#include "lrodefs.h"
extern const LUA_REG_TYPE tsi_map[];

const LUA_REG_TYPE platform_map[] =
{
#if LUA_OPTIMIZE_MEMORY > 0
  { LSTRKEY( "tsi" ), LROVAL( tsi_map ) },
#endif
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_platform( lua_State *L )
{
#if LUA_OPTIMIZE_MEMORY > 0
  return 0;
#else // #if LUA_OPTIMIZE_MEMORY > 0
  luaL_register( L, PS_LIB_TABLE_NAME, platform_map );

  // Setup the new tables inside platform table
  lua_newtable( L );
  luaL_register( L, NULL, enc_map );
  lua_setfield( L, -2, "tsi" );

  return 1;
#endif // #if LUA_OPTIMIZE_MEMORY > 0
}

#else // #ifdef ENABLE_TSI

LUALIB_API int luaopen_platform( lua_State *L )
{
  return 0;
}

#endif // #ifdef ENABLE_TSI
