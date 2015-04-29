// eLua Module for WS2812 support

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "lrotable.h"
#include "platform_conf.h"
#include "auxmods.h"
#include "utils.h"
#include "stm32f4xx_conf.h"

#define WS2812_CLOCK 50000000
#define WS2812_SCALE_NS ( 1000000000 / WS2812_CLOCK )

GPIO_TypeDef * const pio_port[] = { GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH, GPIOI };
const TIM_TypeDef * const timer[] = {
  TIM1,   // ID: 0
  TIM2,   // ID: 1
  TIM3,   // ID: 2
  TIM4,   // ID: 3
  TIM5,   // ID: 4
  TIM8,   // ID: 5
  TIM9,   // ID: 6
  TIM10,  // ID: 7
  TIM11,  // ID: 8
  TIM12,  // ID: 9
  TIM13,  // ID: 10
  TIM14   // ID: 11
};

int tmr_id = -1;

#define platform_timer_get_diff_ns_crt( id, v )  platform_timer_get_diff_ns( id, v, timer[id]->CNT )

//Lua: init(tmr_id)
static int ws2812_init( lua_State *L )
{
  unsigned id;
  
  id = luaL_checkinteger( L, 1 );
  MOD_CHECK_ID( timer, id );
  tmr_id = id;
  platform_timer_op( tmr_id, PLATFORM_TIMER_OP_SET_CLOCK, WS2812_CLOCK );

  return 0;
}

timer_data_type platform_timer_get_diff_ns( unsigned id, timer_data_type start, timer_data_type end )
{
  start = ( end - start ) * WS2812_SCALE_NS;
  return UMIN( start, PLATFORM_TIMER_SYS_MAX );
}

static void ws2812_write_0( int port, int pin )
{
  timer_data_type start_time = TIM2->CNT;
  //platform_pio_op( port, ( ( u32 ) 1 << pin ), PLATFORM_IO_PIN_SET );
  pio_port[port]->BSRRL = ( u32 ) 1 << pin;
  while( ( timer[tmr_id]->CNT - start_time ) < ( 350 / WS2812_SCALE_NS ) );
  //platform_pio_op( port, ( ( u32 ) 1 << pin ), PLATFORM_IO_PIN_CLEAR );
  pio_port[port]->BSRRH = ( u32 ) 1 << pin;
  while(( timer[tmr_id]->CNT - start_time ) < ( 1250 / WS2812_SCALE_NS ) );
  //printf("end_ns_0: %llu, %lu\n", start_time, (u32)platform_timer_get_diff_ns_crt( tmr_id, start_time));
}

static void ws2812_write_1( int port, int pin )
{
  timer_data_type start_time = TIM2->CNT;
  //timer_data_type first_wait;
  //first_wait = platform_timer_get_diff_ns_crt( tmr_id, start_time);
  //platform_pio_op( port, ( ( u32 ) 1 << pin ), PLATFORM_IO_PIN_SET );
  pio_port[port]->BSRRL = ( u32 ) 1 << pin;
  while( ( timer[tmr_id]->CNT - start_time ) < ( 700 / WS2812_SCALE_NS ) );
  //platform_pio_op( port, ( ( u32 ) 1 << pin ), PLATFORM_IO_PIN_CLEAR );
  pio_port[port]->BSRRH = ( u32 ) 1 << pin;
  while( ( timer[tmr_id]->CNT - start_time ) < ( 1250 / WS2812_SCALE_NS ) );
  //printf("end_ns_1: %llu, %lu\n", start_time, (u32)platform_timer_get_diff_ns_crt( tmr_id, start_time));
}

//Lua: writergb(pin, string)
static int ws2812_writergb( lua_State *L )
{
  size_t len;
  int pos;
  int pos_tmp;
  u8 mask;
  int code = ( int )luaL_checkinteger( L, 1 );
  int port = PLATFORM_IO_GET_PORT( code );
  int pin  = PLATFORM_IO_GET_PIN( code );

  if( PLATFORM_IO_IS_PORT( code ) || !platform_pio_has_port( port ) || !platform_pio_has_pin( port, pin ) )
    return luaL_error( L, "invalid pin" );

  if( tmr_id < 0 )
    return luaL_error( L, "timer not configured" );

  platform_timer_op( tmr_id, PLATFORM_TIMER_OP_START, 0 );

  platform_pio_op( port, ( ( u32 ) 1 << pin ), PLATFORM_IO_PIN_DIR_OUTPUT );

  const char *grb_str = luaL_checklstring( L, 2, &len );

  int old_status;
  old_status = platform_cpu_set_global_interrupts( PLATFORM_CPU_DISABLE );

  for( pos = 0; pos < len; pos++ )
  {
    pos_tmp = pos;
    if( pos % 3 == 0 )
      pos_tmp++;
    else if( pos % 3 == 1 )
      pos_tmp--;

    for( mask = 0x80; mask; mask >>= 1 )
    {
      ( grb_str[ pos_tmp ] & mask ) ? ws2812_write_1( port, pin ) : ws2812_write_0( port, pin );
    }
  }

  platform_cpu_set_global_interrupts( old_status );

  return 0;
}


#define MIN_OPT_LEVEL 2
#include "lrodefs.h"  

// Module function map
const LUA_REG_TYPE ws2812_map[] =
{ 
  { LSTRKEY( "init" ),  LFUNCVAL( ws2812_init ) },
  { LSTRKEY( "writergb" ),  LFUNCVAL( ws2812_writergb ) },
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_ws2812( lua_State *L )
{
  LREGISTER( L, AUXLIB_WS2812, ws2812_map );
}  

