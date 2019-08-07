#include <redasm/redasm.h>
#include "esp.h"

// Based on: https://github.com/espressif/esptool/blob/master/esptool.py

REDASM_LOADER("ESP Firmware/ROM", "Dax", "MIT", 1)
REDASM_LOAD { esp.plugin = new ESPLoader(); return true; }
REDASM_UNLOAD { esp.plugin->release(); }
