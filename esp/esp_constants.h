#pragma once

#define ESP_IMAGE_MAGIC 0xE9 // First byte of the application image
#define IMAGE_V2_MAGIC  0xEA // First byte of the "v2" application image

#define SPI_FLASH_INTERFACE_QIO  0
#define SPI_FLASH_INTERFACE_QOUT 1
#define SPI_FLASH_INTERFACE_DIO  2
#define SPI_FLASH_INTERFACE_DOUT 3

#define FLASH_SIZE_512K  0
#define FLASH_SIZE_256K  1
#define FLASH_SIZE_1M    2
#define FLASH_SIZE_2M    3
#define FLASH_SIZE_4M    4

#define FLASH_FREQ_40MHZ 0
#define FLASH_FREQ_26MHZ 1
#define FLASH_FREQ_20MHZ 2
#define FLASH_FREQ_80MHZ 0xF
