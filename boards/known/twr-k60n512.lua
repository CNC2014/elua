-- STM32F4-NUCLEO build configuration

return {
  cpu = 'mk60n512vmd100',
  components = {
    sercon = { uart = "3", speed = 115200 },
    romfs = true,
    advanced_shell = true,
    term = { lines = 25, cols = 80 },
    linenoise = { shell_lines = 10, lua_lines = 50 },
    rpc = { uart = "0", speed = 115200 },
    xmodem = true,
  },
  config = {
    egc = { mode = "alloc" },
    vtmr = { num = 4, freq = 10 },
    ram = { internal_rams = 1 },
    clocks = { cpu = 120000000 },
  },
  modules = {
    generic = { 'all', "-i2c", "-net", "-spi", "-adc", "-can" },
    platform = 'all',
  },
}

