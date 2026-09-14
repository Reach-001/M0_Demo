/**
 * @file    bsp_w25q128.h
 * @brief   W25Q128 SPI Flash 驱动
 */

#ifndef _BSP_W25Q128_H_
#define _BSP_W25Q128_H_

#include "board_config.h"

#define W25Q128_JEDEC_ID        (0xEF4018UL)
#define W25Q128_FLASH_SIZE      (16UL * 1024UL * 1024UL)
#define W25Q128_SECTOR_SIZE     (4096UL)
#define W25Q128_PAGE_SIZE       (256UL)

typedef enum
{
    W25Q128_OK = 0,
    W25Q128_ERROR = 1,
    W25Q128_TIMEOUT = 2,
    W25Q128_PARAM_ERROR = 3,
} w25q128_status_t;

void                w25q128_init(void);
uint32              w25q128_read_jedec_id(void);
uint8               w25q128_read_status1(void);
w25q128_status_t    w25q128_wait_busy(uint32 timeout);
w25q128_status_t    w25q128_read(uint32 addr, uint8 *buf, uint32 len);
w25q128_status_t    w25q128_page_program(uint32 addr, const uint8 *buf, uint32 len);
w25q128_status_t    w25q128_write(uint32 addr, const uint8 *buf, uint32 len);
w25q128_status_t    w25q128_sector_erase(uint32 addr);
w25q128_status_t    w25q128_block_erase_32k(uint32 addr);
w25q128_status_t    w25q128_block_erase_64k(uint32 addr);
w25q128_status_t    w25q128_chip_erase(void);

#endif /* _BSP_W25Q128_H_ */
