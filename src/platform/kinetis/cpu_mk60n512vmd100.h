// CPU definition file for STM32F411RE

#ifndef __CPU_MK60N512VMD100_H__
#define __CPU_MK60N512VMD100_H__

#include "type.h"
#include "stacks.h"
// #include "platform_ints.h"

// Number of resources (0 if not available/not implemented)
#define NUM_PIO               5
#define NUM_SPI               0
#define NUM_UART              6
#define NUM_TIMER             4
#define NUM_PHYS_TIMER        4
#define NUM_PWM               6
#define NUM_ADC               0
#define NUM_CAN               0

#define ADC_BIT_RESOLUTION    12

u32 platform_s_cpu_get_frequency();
#define CPU_FREQUENCY         platform_s_cpu_get_frequency()

// PIO prefix ('0' for P0, P1, ... or 'A' for PA, PB, ...)
#define PIO_PREFIX            'A'
// Pins per port configuration:
// #define PIO_PINS_PER_PORT (n) if each port has the same number of pins, or
// #define PIO_PIN_ARRAY { n1, n2, ... } to define pins per port in an array
// Use #define PIO_PINS_PER_PORT 0 if this isn't needed
#define PIO_PINS_PER_PORT     32

// Internal memory data
#define INTERNAL_SRAM_BASE    0x1FFF0000
#define INTERNAL_SRAM_SIZE    ( 128 * 1024 )
#define INTERNAL_RAM1_FIRST_FREE        end
#define INTERNAL_RAM1_LAST_FREE         ( INTERNAL_SRAM_BASE + INTERNAL_SRAM_SIZE - STACK_SIZE_TOTAL -1 )

// Internal Flash data
// #define INTERNAL_FLASH_SIZE             ( 512 * 1024 )
// #define INTERNAL_FLASH_SECTOR_ARRAY     { 16384, 16384, 16384, 16384, 65536, 131072, 131072, 131072 }
// #define INTERNAL_FLASH_START_ADDRESS    0x00000000

// Interrupt list for this CPU


#endif // #ifndef __CPU_MK60N512VMD100_H__

