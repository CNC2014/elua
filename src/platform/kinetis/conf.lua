local utils = require "utils.utils"
local sf = string.format

ldscript = "mk60n512md100.ld"

-- Configuration file for the kinetis microcontroller
specific_files = "startup_kinetis.c platform.c cpu/arm_cm4.c cpu/sysinit.c cpu/kinetis_it.c drivers/uart/uart.c drivers/mcg/mcg.c drivers/wdog/wdog.c tsi.c"

addi( sf( 'src/platform/%s/cpu', platform ) )
addi( sf( 'src/platform/%s/cpu/headers', platform ) )
addi( sf( 'src/platform/%s/platforms', platform ) )
addi( sf( 'src/platform/%s/drivers/mcg', platform ) )
addi( sf( 'src/platform/%s/drivers/uart', platform ) )
addi( sf( 'src/platform/%s/drivers/wdog', platform ) )

-- Prepend with path
specific_files = utils.prepend_path( specific_files, sf( "src/platform/%s", platform) )
specific_files = specific_files .. " src/platform/cortex_utils.s src/platform/arm_cortex_interrupts.c"
ldscript = sf( "src/platform/%s/%s", platform, ldscript )

addm{ "FOR" .. comp.cpu:upper(), 'gcc', 'CORTEX_M4', 'TOWER', 'TWR_K60N512' }

-- Standard GCC flags
addcf{ '-ffunction-sections', '-fdata-sections', '-fno-strict-aliasing', '-Wall' }
addlf{ '-nostartfiles', '-nostdlib', '-T', ldscript, '-Wl,--gc-sections', '-Wl,--allow-multiple-definition' }
addaf{ '-x', 'assembler-with-cpp', '-Wall' }
addlib{ 'c','gcc','m' }

local target_flags =  {'-mcpu=cortex-m4','-mthumb' }

-- Configure general flags for target
addcf{ target_flags, '-mlittle-endian' }
addlf{ target_flags, '-Wl,-e,Reset_Handler', '-Wl,-static' }
addaf( target_flags )

-- Toolset data
tools.kinetis = {}

-- Array of file names that will be checked against the 'prog' target; their absence will force a rebuild
tools.kinetis.prog_flist = { output .. ".bin" }

-- We use 'gcc' as the assembler
toolset.asm = toolset.compile

