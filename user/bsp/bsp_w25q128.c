/**
 * @file    bsp_w25q128.c
 * @brief   W25Q128 SPI Flash 驱动实现
 *
 * 当前适配 Winbond W25Q128，容量 16MB，页大小 256B，扇区大小 4KB。
 */

#include "bsp_w25q128.h"

#define W25Q128_CMD_WRITE_ENABLE        0x06
#define W25Q128_CMD_READ_STATUS1        0x05
#define W25Q128_CMD_READ_DATA           0x03
#define W25Q128_CMD_PAGE_PROGRAM        0x02
#define W25Q128_CMD_SECTOR_ERASE        0x20
#define W25Q128_CMD_BLOCK_ERASE_32K     0x52
#define W25Q128_CMD_BLOCK_ERASE_64K     0xD8
#define W25Q128_CMD_CHIP_ERASE          0xC7
#define W25Q128_CMD_READ_JEDEC_ID       0x9F

#define W25Q128_STATUS1_BUSY            0x01
#define W25Q128_DEFAULT_TIMEOUT         0x00FFFFFFUL

static void w25q128_cs_low(void)
{
    gpio_low(W25Q128_CS_PIN);
}

static void w25q128_cs_high(void)
{
    gpio_high(W25Q128_CS_PIN);
}

static uint8 w25q128_transfer_byte(uint8 data)
{
    uint8 rx = 0;
    spi_transfer_8bit(W25Q128_SPI_INDEX, &data, &rx, 1);
    return rx;
}

static void w25q128_write_enable(void)
{
    w25q128_cs_low();
    w25q128_transfer_byte(W25Q128_CMD_WRITE_ENABLE);
    w25q128_cs_high();
}

static void w25q128_send_addr(uint32 addr)
{
    w25q128_transfer_byte((uint8)(addr >> 16));
    w25q128_transfer_byte((uint8)(addr >> 8));
    w25q128_transfer_byte((uint8)addr);
}

static w25q128_status_t w25q128_check_range(uint32 addr, uint32 len)
{
    if ((0 == len) || (addr >= W25Q128_FLASH_SIZE) || (len > (W25Q128_FLASH_SIZE - addr)))
    {
        return W25Q128_PARAM_ERROR;
    }

    return W25Q128_OK;
}

static w25q128_status_t w25q128_erase(uint8 cmd, uint32 addr, uint32 align_size)
{
    if ((addr >= W25Q128_FLASH_SIZE) || (0 != (addr % align_size)))
    {
        return W25Q128_PARAM_ERROR;
    }

    if (W25Q128_OK != w25q128_wait_busy(W25Q128_DEFAULT_TIMEOUT))
    {
        return W25Q128_TIMEOUT;
    }

    w25q128_write_enable();

    w25q128_cs_low();
    w25q128_transfer_byte(cmd);
    w25q128_send_addr(addr);
    w25q128_cs_high();

    return w25q128_wait_busy(W25Q128_DEFAULT_TIMEOUT);
}

void w25q128_init(void)
{
    /* CS 先拉高，避免 SPI 复用初始化期间被 Flash 误识别为命令。 */
    gpio_init(W25Q128_CS_PIN, GPO, GPIO_HIGH, GPO_PUSH_PULL);

    spi_init(W25Q128_SPI_INDEX,
             W25Q128_SPI_MODE,
             W25Q128_SPI_BAUD,
             W25Q128_SCK_PIN,
             W25Q128_MOSI_PIN,
             W25Q128_MISO_PIN,
             SPI_CS_NULL);

    /* 上电后先留出一点稳定时间，再开始读写命令。 */
    system_delay_ms(10);
}

uint32 w25q128_read_jedec_id(void)
{
    uint32 id = 0;

    w25q128_cs_low();
    w25q128_transfer_byte(W25Q128_CMD_READ_JEDEC_ID);
    id |= ((uint32)w25q128_transfer_byte(0xFF) << 16);
    id |= ((uint32)w25q128_transfer_byte(0xFF) << 8);
    id |= (uint32)w25q128_transfer_byte(0xFF);
    w25q128_cs_high();

    return id;
}

uint8 w25q128_read_status1(void)
{
    uint8 status = 0;

    w25q128_cs_low();
    w25q128_transfer_byte(W25Q128_CMD_READ_STATUS1);
    status = w25q128_transfer_byte(0xFF);
    w25q128_cs_high();

    return status;
}

w25q128_status_t w25q128_wait_busy(uint32 timeout)
{
    while (timeout--)
    {
        if (0 == (w25q128_read_status1() & W25Q128_STATUS1_BUSY))
        {
            return W25Q128_OK;
        }
    }

    return W25Q128_TIMEOUT;
}

w25q128_status_t w25q128_read(uint32 addr, uint8 *buf, uint32 len)
{
    if ((NULL == buf) || (W25Q128_OK != w25q128_check_range(addr, len)))
    {
        return W25Q128_PARAM_ERROR;
    }

    if (W25Q128_OK != w25q128_wait_busy(W25Q128_DEFAULT_TIMEOUT))
    {
        return W25Q128_TIMEOUT;
    }

    w25q128_cs_low();
    w25q128_transfer_byte(W25Q128_CMD_READ_DATA);
    w25q128_send_addr(addr);

    while (len--)
    {
        *buf++ = w25q128_transfer_byte(0xFF);
    }

    w25q128_cs_high();

    return W25Q128_OK;
}

w25q128_status_t w25q128_page_program(uint32 addr, const uint8 *buf, uint32 len)
{
    uint32 page_remain = W25Q128_PAGE_SIZE - (addr % W25Q128_PAGE_SIZE);

    if ((NULL == buf) || (W25Q128_OK != w25q128_check_range(addr, len)) || (len > page_remain))
    {
        return W25Q128_PARAM_ERROR;
    }

    if (W25Q128_OK != w25q128_wait_busy(W25Q128_DEFAULT_TIMEOUT))
    {
        return W25Q128_TIMEOUT;
    }

    w25q128_write_enable();

    w25q128_cs_low();
    w25q128_transfer_byte(W25Q128_CMD_PAGE_PROGRAM);
    w25q128_send_addr(addr);

    while (len--)
    {
        w25q128_transfer_byte(*buf++);
    }

    w25q128_cs_high();

    return w25q128_wait_busy(W25Q128_DEFAULT_TIMEOUT);
}

w25q128_status_t w25q128_write(uint32 addr, const uint8 *buf, uint32 len)
{
    w25q128_status_t status;
    uint32 page_remain;
    uint32 write_len;

    if ((NULL == buf) || (W25Q128_OK != w25q128_check_range(addr, len)))
    {
        return W25Q128_PARAM_ERROR;
    }

    while (len)
    {
        page_remain = W25Q128_PAGE_SIZE - (addr % W25Q128_PAGE_SIZE);
        write_len = (len < page_remain) ? len : page_remain;

        status = w25q128_page_program(addr, buf, write_len);
        if (W25Q128_OK != status)
        {
            return status;
        }

        addr += write_len;
        buf += write_len;
        len -= write_len;
    }

    return W25Q128_OK;
}

w25q128_status_t w25q128_sector_erase(uint32 addr)
{
    return w25q128_erase(W25Q128_CMD_SECTOR_ERASE, addr, W25Q128_SECTOR_SIZE);
}

w25q128_status_t w25q128_block_erase_32k(uint32 addr)
{
    return w25q128_erase(W25Q128_CMD_BLOCK_ERASE_32K, addr, 32UL * 1024UL);
}

w25q128_status_t w25q128_block_erase_64k(uint32 addr)
{
    return w25q128_erase(W25Q128_CMD_BLOCK_ERASE_64K, addr, 64UL * 1024UL);
}

w25q128_status_t w25q128_chip_erase(void)
{
    if (W25Q128_OK != w25q128_wait_busy(W25Q128_DEFAULT_TIMEOUT))
    {
        return W25Q128_TIMEOUT;
    }

    w25q128_write_enable();

    w25q128_cs_low();
    w25q128_transfer_byte(W25Q128_CMD_CHIP_ERASE);
    w25q128_cs_high();

    return w25q128_wait_busy(W25Q128_DEFAULT_TIMEOUT);
}
