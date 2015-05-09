// eLua Module for WS2812 support
// Platform-specific implementation for STM32 (in-lines GPIO & timer register modification)
//
// Reference:
//   http://wp.josh.com/2014/05/13/ws2812-neopixels-are-not-so-finicky-once-you-get-to-know-them/
//   https://github.com/nodemcu/nodemcu-firmware/blob/master/app/modules/ws2812.c

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "platform.h"
#include "lrotable.h"
#include "platform_conf.h"
#include "auxmods.h"
#include "utils.h"
#include "stm32f4xx_conf.h"

#define WS2812_CLOCK 10000000
#define WS2812_TOH 200
#define WS2812_T1H 550
#define WS2812_TLD 450
#define WS2812_TLL 6000


extern TIM_TypeDef *const timer[];
extern GPIO_TypeDef * const pio_port[];
extern const u8 timer_width[];

int tmr_id = -1;
s32 tmr_scale_ns = -1;

//Lua: init(tmr_id)
static int ws2812_init( lua_State *L )
{
  unsigned id;
  
  id = luaL_checkinteger( L, 1 );
  MOD_CHECK_ID( timer, id );
  
  if( timer_width[ id ] != 32 )
    return luaL_error( L, "32-bit timer required" );

  tmr_id = id;
  tmr_scale_ns = 1000000000 / platform_timer_op( tmr_id, PLATFORM_TIMER_OP_SET_CLOCK, WS2812_CLOCK );

  return 0;
}

// TODO: Resolve portability issue(s):
//       - timer is read before setting the pin state which doesn't guarantee minimum timings, 
//         when done after, however, we aren't able to get T0H short enough
//       - support 16-bit timers in addition to 32-bit
static void ws2812_write_0( int port, int pin )
{
  u32 start_time = timer[tmr_id]->CNT;

  pio_port[ port ]->BSRRL = ( u32 ) 1 << pin;

  while( ( timer[ tmr_id ]->CNT - start_time ) < ( WS2812_TOH / tmr_scale_ns ) );

  pio_port[ port ]->BSRRH = ( u32 ) 1 << pin;

  while( ( timer[ tmr_id ]->CNT - start_time ) < ( ( WS2812_TOH  + WS2812_TLD ) / tmr_scale_ns ) );
}

static void ws2812_write_1( int port, int pin )
{
  u32 start_time = timer[ tmr_id ]->CNT;

  pio_port[ port ]->BSRRL = ( u32 ) 1 << pin;

  while( ( timer[ tmr_id ]->CNT - start_time ) < ( WS2812_T1H / tmr_scale_ns ) );

  pio_port[ port ]->BSRRH = ( u32 ) 1 << pin;

  while( ( timer[ tmr_id ]->CNT - start_time ) < ( ( WS2812_T1H + WS2812_TLD ) / tmr_scale_ns ) );
}

//Lua: writergb(pin, string)
static int ws2812_writergb( lua_State *L )
{
  size_t len;
  int pos, pos_tmp, old_status;
  u8 mask;
  u32 start_time;
  
  int code = ( int )luaL_checkinteger( L, 1 );
  int port = PLATFORM_IO_GET_PORT( code );
  int pin  = PLATFORM_IO_GET_PIN( code );
  const char *rgb_str = luaL_checklstring( L, 2, &len );

  if( PLATFORM_IO_IS_PORT( code ) || !platform_pio_has_port( port ) || !platform_pio_has_pin( port, pin ) )
    return luaL_error( L, "invalid pin" );

  if( tmr_id < 0 || tmr_scale_ns < 0 )
    return luaL_error( L, "timer not configured" );

  if( timer_width[ tmr_id ] != 32 )
    return luaL_error( L, "32-bit timer required" );

  platform_timer_op( tmr_id, PLATFORM_TIMER_OP_START, 0 );

  // Set up pin
  platform_pio_op( port, ( ( u32 ) 1 << pin ), PLATFORM_IO_PIN_DIR_OUTPUT );
  platform_pio_op( port, ( ( u32 ) 1 << pin ), PLATFORM_IO_PIN_CLEAR );

  // Disable other interrupts
  old_status = platform_cpu_set_global_interrupts( PLATFORM_CPU_DISABLE );

  for( pos = 0; pos < len; pos++ )
  {
    // Swap first & second index since WS2812 expects GRB ordering
    switch( pos % 3 )
    {
      case 0: // Get G first
        pos_tmp = pos + 1;
        break;
      case 1: // Get R second
        pos_tmp = pos - 1;
        break;
      default:
        pos_tmp = pos;
        break;
    }

    // Iterate through bits for each byte
    for( mask = 0x80; mask; mask >>= 1 )
    {
      ( rgb_str[ pos_tmp ] & mask ) ? ws2812_write_1( port, pin ) : ws2812_write_0( port, pin );
    }
  }

  // Re-enable interrupts
  platform_cpu_set_global_interrupts( old_status );

  start_time = timer[ tmr_id ]->CNT;
  while( ( timer[ tmr_id ]->CNT - start_time ) < ( WS2812_TLL ) );

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

