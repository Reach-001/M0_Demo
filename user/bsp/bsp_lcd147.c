/**
 * @file    bsp_lcd147.c
 * @brief   1.47 inch 172x320 SPI LCD driver (ST7789)
 *
 * 使用硬件 SPI（MSPM0 SPI1），初始化序列匹配 ST7789 参考驱动。
 *
 * 引脚连接（LCKFB TMX-MSPM0G3507 标准接线）：
 *   SCK  = B9    MOSI = B8    RST = B10
 *   DC   = B11   CS   = B14   BLK = B26
 */

#include "bsp_lcd147.h"

#include "zf_common_font.h"
#include "zf_driver_delay.h"
#include "zf_driver_gpio.h"
#include "zf_driver_spi.h"

/*===========================================================================
 * 控制引脚宏
 *===========================================================================*/
#define LCD147_DC(x)    ((x) ? gpio_high(LCD147_DC_PIN) : gpio_low(LCD147_DC_PIN))
#define LCD147_RST(x)   ((x) ? gpio_high(LCD147_RST_PIN) : gpio_low(LCD147_RST_PIN))
#define LCD147_CS(x)    ((x) ? gpio_high(LCD147_CS_PIN) : gpio_low(LCD147_CS_PIN))
#define LCD147_BL(x)    ((x) ? gpio_high(LCD147_BL_PIN) : gpio_low(LCD147_BL_PIN))

#define LCD147_BYTES_PER_ROW  ((LCD147_WIDTH + 7U) / 8U)

uint8 lcd147_framebuffer[LCD147_FRAMEBUFFER_SIZE];

/*===========================================================================
 * 6×8 ASCII 字模
 *===========================================================================*/
static const uint8 font_6x8[][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00,0x00},
    {0x00,0x07,0x00,0x07,0x00,0x00}, {0x14,0x7F,0x14,0x7F,0x14,0x00},
    {0x24,0x2A,0x7F,0x2A,0x12,0x00}, {0x23,0x13,0x08,0x64,0x62,0x00},
    {0x36,0x49,0x55,0x22,0x50,0x00}, {0x00,0x05,0x03,0x00,0x00,0x00},
    {0x00,0x1C,0x22,0x41,0x00,0x00}, {0x00,0x41,0x22,0x1C,0x00,0x00},
    {0x08,0x2A,0x1C,0x2A,0x08,0x00}, {0x08,0x08,0x3E,0x08,0x08,0x00},
    {0x00,0x50,0x30,0x00,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08,0x00},
    {0x00,0x60,0x60,0x00,0x00,0x00}, {0x20,0x10,0x08,0x04,0x02,0x00},
    {0x3E,0x51,0x49,0x45,0x3E,0x00}, {0x00,0x42,0x7F,0x40,0x00,0x00},
    {0x42,0x61,0x51,0x49,0x46,0x00}, {0x21,0x41,0x45,0x4B,0x31,0x00},
    {0x18,0x14,0x12,0x7F,0x10,0x00}, {0x27,0x45,0x45,0x45,0x39,0x00},
    {0x3C,0x4A,0x49,0x49,0x30,0x00}, {0x01,0x71,0x09,0x05,0x03,0x00},
    {0x36,0x49,0x49,0x49,0x36,0x00}, {0x06,0x49,0x49,0x29,0x1E,0x00},
    {0x00,0x36,0x36,0x00,0x00,0x00}, {0x00,0x56,0x36,0x00,0x00,0x00},
    {0x00,0x08,0x14,0x22,0x41,0x00}, {0x14,0x14,0x14,0x14,0x14,0x00},
    {0x41,0x22,0x14,0x08,0x00,0x00}, {0x02,0x01,0x51,0x09,0x06,0x00},
    {0x32,0x49,0x79,0x41,0x3E,0x00}, {0x7E,0x11,0x11,0x11,0x7E,0x00},
    {0x7F,0x49,0x49,0x49,0x36,0x00}, {0x3E,0x41,0x41,0x41,0x22,0x00},
    {0x7F,0x41,0x41,0x22,0x1C,0x00}, {0x7F,0x49,0x49,0x49,0x41,0x00},
    {0x7F,0x09,0x09,0x01,0x01,0x00}, {0x3E,0x41,0x41,0x51,0x32,0x00},
    {0x7F,0x08,0x08,0x08,0x7F,0x00}, {0x00,0x41,0x7F,0x41,0x00,0x00},
    {0x20,0x40,0x41,0x3F,0x01,0x00}, {0x7F,0x08,0x14,0x22,0x41,0x00},
    {0x7F,0x40,0x40,0x40,0x40,0x00}, {0x7F,0x02,0x04,0x02,0x7F,0x00},
    {0x7F,0x04,0x08,0x10,0x7F,0x00}, {0x3E,0x41,0x41,0x41,0x3E,0x00},
    {0x7F,0x09,0x09,0x09,0x06,0x00}, {0x3E,0x41,0x51,0x21,0x5E,0x00},
    {0x7F,0x09,0x19,0x29,0x46,0x00}, {0x46,0x49,0x49,0x49,0x31,0x00},
    {0x01,0x01,0x7F,0x01,0x01,0x00}, {0x3F,0x40,0x40,0x40,0x3F,0x00},
    {0x1F,0x20,0x40,0x20,0x1F,0x00}, {0x7F,0x20,0x18,0x20,0x7F,0x00},
    {0x63,0x14,0x08,0x14,0x63,0x00}, {0x03,0x04,0x78,0x04,0x03,0x00},
    {0x61,0x51,0x49,0x45,0x43,0x00}, {0x00,0x00,0x7F,0x41,0x41,0x00},
    {0x02,0x04,0x08,0x10,0x20,0x00}, {0x41,0x41,0x7F,0x00,0x00,0x00},
    {0x04,0x02,0x01,0x02,0x04,0x00}, {0x40,0x40,0x40,0x40,0x40,0x00},
    {0x00,0x01,0x02,0x04,0x00,0x00}, {0x20,0x54,0x54,0x54,0x78,0x00},
    {0x7F,0x48,0x44,0x44,0x38,0x00}, {0x38,0x44,0x44,0x44,0x20,0x00},
    {0x38,0x44,0x44,0x48,0x7F,0x00}, {0x38,0x54,0x54,0x54,0x18,0x00},
    {0x08,0x7E,0x09,0x01,0x02,0x00}, {0x08,0x14,0x54,0x54,0x3C,0x00},
    {0x7F,0x08,0x04,0x04,0x78,0x00}, {0x00,0x44,0x7D,0x40,0x00,0x00},
    {0x20,0x40,0x44,0x3D,0x00,0x00}, {0x00,0x7F,0x10,0x28,0x44,0x00},
    {0x00,0x41,0x7F,0x40,0x00,0x00}, {0x7C,0x04,0x18,0x04,0x78,0x00},
    {0x7C,0x08,0x04,0x04,0x78,0x00}, {0x38,0x44,0x44,0x44,0x38,0x00},
    {0x7C,0x14,0x14,0x14,0x08,0x00}, {0x08,0x14,0x14,0x18,0x7C,0x00},
    {0x7C,0x08,0x04,0x04,0x08,0x00}, {0x48,0x54,0x54,0x54,0x20,0x00},
    {0x04,0x3F,0x44,0x40,0x20,0x00}, {0x3C,0x40,0x40,0x20,0x7C,0x00},
    {0x1C,0x20,0x40,0x20,0x1C,0x00}, {0x3C,0x40,0x30,0x40,0x3C,0x00},
    {0x44,0x28,0x10,0x28,0x44,0x00}, {0x0C,0x50,0x50,0x50,0x3C,0x00},
    {0x44,0x64,0x54,0x4C,0x44,0x00}, {0x00,0x08,0x36,0x41,0x00,0x00},
    {0x00,0x00,0x7F,0x00,0x00,0x00}, {0x00,0x41,0x36,0x08,0x00,0x00},
    {0x08,0x04,0x08,0x10,0x08,0x00},
};

/*===========================================================================
 * LCD 命令/数据写入（硬件 SPI）
 *===========================================================================*/
static void lcd147_write_command(uint8 command)
{
    LCD147_DC(0);
    spi_write_8bit(LCD147_SPI_INDEX, command);
    LCD147_DC(1);
}

static void lcd147_write_data8(uint8 data)
{
    spi_write_8bit(LCD147_SPI_INDEX, data);
}

static void lcd147_write_data16(uint16 data)
{
    lcd147_write_data8((uint8)(data >> 8));
    lcd147_write_data8((uint8)data);
}

/*===========================================================================
 * 设置显示窗口
 *===========================================================================*/
static void lcd147_set_window(uint16 x1, uint16 y1, uint16 x2, uint16 y2)
{
    zf_assert(x1 < LCD147_WIDTH);
    zf_assert(x2 < LCD147_WIDTH);
    zf_assert(y1 < LCD147_HEIGHT);
    zf_assert(y2 < LCD147_HEIGHT);

    lcd147_write_command(0x2A);
    if (LCD147_USE_HORIZONTAL == 0 || LCD147_USE_HORIZONTAL == 1)
    {
        lcd147_write_data16(x1 + LCD147_COLUMN_OFFSET);
        lcd147_write_data16(x2 + LCD147_COLUMN_OFFSET);
    }
    else
    {
        lcd147_write_data16(x1);
        lcd147_write_data16(x2);
    }

    lcd147_write_command(0x2B);
    if (LCD147_USE_HORIZONTAL == 0 || LCD147_USE_HORIZONTAL == 1)
    {
        lcd147_write_data16(y1 + LCD147_LINE_OFFSET);
        lcd147_write_data16(y2 + LCD147_LINE_OFFSET);
    }
    else
    {
        lcd147_write_data16(y1 + LCD147_COLUMN_OFFSET);
        lcd147_write_data16(y2 + LCD147_COLUMN_OFFSET);
    }

    lcd147_write_command(0x2C);
}

/*===========================================================================
 * 背光控制
 *===========================================================================*/
void bsp_lcd147_set_backlight(uint8 on)
{
    LCD147_BL(on ? 1U : 0U);
}

/*===========================================================================
 * LCD 初始化（硬件 SPI + ST7789 参考驱动初始化序列）
 *
 * 注意：
 *   1. 已删除 0x21 INVON（反色命令），该屏实测不需要
 *   2. Gamma 表完全匹配 STM32 参考驱动
 *   3. 背光在初始化序列结束后打开
 *===========================================================================*/
void bsp_lcd147_init(void)
{
    /* ---- 硬件 SPI 初始化 ---- */
    spi_init(LCD147_SPI_INDEX, SPI_MODE0, LCD147_SPI_BAUD,
             LCD147_SCK_PIN, LCD147_MOSI_PIN, LCD147_MISO_PIN, SPI_CS_NULL);

    /* ---- 控制引脚初始化（CS 由软件控制，不经 SPI 硬件） ---- */
    gpio_init(LCD147_DC_PIN,  GPO, GPIO_LOW,  GPO_PUSH_PULL);
    gpio_init(LCD147_RST_PIN, GPO, GPIO_HIGH, GPO_PUSH_PULL);
    gpio_init(LCD147_CS_PIN,  GPO, GPIO_HIGH, GPO_PUSH_PULL);
    gpio_init(LCD147_BL_PIN,  GPO, GPIO_LOW,  GPO_PUSH_PULL);

    /* ---- 硬件复位 ---- */
    LCD147_RST(0);
    system_delay_ms(100);
    LCD147_RST(1);
    system_delay_ms(100);

    LCD147_CS(0);

    /* ---- ST7789 初始化序列（匹配 STM32 参考驱动） ---- */
    lcd147_write_command(0x11);           /* Sleep Out */
    /* 参考驱动在初始化序列末尾统一延时 */

    lcd147_write_command(0x36);           /* MADCTL */
    if (LCD147_USE_HORIZONTAL == 0)      lcd147_write_data8(0x00);
    else if (LCD147_USE_HORIZONTAL == 1) lcd147_write_data8(0xC0);
    else if (LCD147_USE_HORIZONTAL == 2) lcd147_write_data8(0x70);
    else                                 lcd147_write_data8(0xA0);

    lcd147_write_command(0x3A);           /* COLMOD: 16-bit/pixel RGB565 */
    lcd147_write_data8(0x05);

    lcd147_write_command(0xB2);           /* PORCTRL */
    lcd147_write_data8(0x0C);
    lcd147_write_data8(0x0C);
    lcd147_write_data8(0x00);
    lcd147_write_data8(0x33);
    lcd147_write_data8(0x33);

    lcd147_write_command(0xB7);           /* GCTRL */
    lcd147_write_data8(0x35);

    lcd147_write_command(0xBB);           /* VCOMS */
    lcd147_write_data8(0x35);

    lcd147_write_command(0xC0);           /* LCMCTRL */
    lcd147_write_data8(0x2C);

    lcd147_write_command(0xC2);           /* VDVVRHEN */
    lcd147_write_data8(0x01);

    lcd147_write_command(0xC3);           /* VRHS */
    lcd147_write_data8(0x13);

    lcd147_write_command(0xC4);           /* VDVS */
    lcd147_write_data8(0x20);

    lcd147_write_command(0xC6);           /* FRCTRL2 */
    lcd147_write_data8(0x0F);

    lcd147_write_command(0xD0);           /* PWCTRL1 */
    lcd147_write_data8(0xA4);
    lcd147_write_data8(0xA1);

    lcd147_write_command(0xD6);
    lcd147_write_data8(0xA1);

    /* 正极性 Gamma 校正（14 字节） */
    lcd147_write_command(0xE0);
    lcd147_write_data8(0xF0); lcd147_write_data8(0x00); lcd147_write_data8(0x04); lcd147_write_data8(0x04);
    lcd147_write_data8(0x04); lcd147_write_data8(0x05); lcd147_write_data8(0x29); lcd147_write_data8(0x33);
    lcd147_write_data8(0x3E); lcd147_write_data8(0x38); lcd147_write_data8(0x12); lcd147_write_data8(0x12);
    lcd147_write_data8(0x28); lcd147_write_data8(0x30);

    /* 负极性 Gamma 校正（14 字节） */
    lcd147_write_command(0xE1);
    lcd147_write_data8(0xF0); lcd147_write_data8(0x07); lcd147_write_data8(0x0A); lcd147_write_data8(0x0D);
    lcd147_write_data8(0x0B); lcd147_write_data8(0x07); lcd147_write_data8(0x28); lcd147_write_data8(0x33);
    lcd147_write_data8(0x3E); lcd147_write_data8(0x36); lcd147_write_data8(0x14); lcd147_write_data8(0x14);
    lcd147_write_data8(0x29); lcd147_write_data8(0x32);

    /* 已删除 0x21 INVON — 该屏不需要反色 */

    /* Sleep Out + Display On */
    lcd147_write_command(0x11);
    system_delay_ms(120);
    lcd147_write_command(0x29);           /* Display On */

    LCD147_CS(1);

    /* 清屏 + 开背光 */
    bsp_lcd147_clear();
    bsp_lcd147_refresh();
    LCD147_BL(1);
}

/*===========================================================================
 * 直接写全屏纯色（绕过帧缓冲，硬件 SPI ~88ms 完成）
 * 用于 LCD 硬件自检
 *===========================================================================*/
void bsp_lcd147_fill_direct(uint16 color)
{
    LCD147_CS(0);
    lcd147_set_window(0, 0, LCD147_WIDTH - 1U, LCD147_HEIGHT - 1U);

    for (uint32 i = 0; i < (uint32)LCD147_WIDTH * LCD147_HEIGHT; i++)
    {
        lcd147_write_data16(color);
    }

    LCD147_CS(1);
}

/*===========================================================================
 * 帧缓冲操作
 *===========================================================================*/
void bsp_lcd147_clear(void)
{
    memset(lcd147_framebuffer, 0, sizeof(lcd147_framebuffer));
}

void bsp_lcd147_fill(uint8 color)
{
    memset(lcd147_framebuffer, color ? 0xFF : 0x00, sizeof(lcd147_framebuffer));
}

void bsp_lcd147_draw_point(uint16 x, uint16 y, uint8 color)
{
    uint32 idx;
    uint8 mask;

    if (x >= LCD147_WIDTH || y >= LCD147_HEIGHT) return;

    idx = (uint32)y * LCD147_BYTES_PER_ROW + (uint32)(x >> 3);
    mask = (uint8)(0x80U >> (x & 0x07U));

    if (color)
        lcd147_framebuffer[idx] |= mask;
    else
        lcd147_framebuffer[idx] &= (uint8)~mask;
}

uint8 bsp_lcd147_get_point(uint16 x, uint16 y)
{
    uint32 idx;
    uint8 mask;

    if (x >= LCD147_WIDTH || y >= LCD147_HEIGHT) return 0;

    idx = (uint32)y * LCD147_BYTES_PER_ROW + (uint32)(x >> 3);
    mask = (uint8)(0x80U >> (x & 0x07U));
    return (lcd147_framebuffer[idx] & mask) ? 1U : 0U;
}

uint8 bsp_lcd147_get_ascii_column(char ch, uint8 column)
{
    if (ch < 0x20 || ch > 0x7E || column >= 6U) return 0;
    return font_6x8[(uint8)ch - 0x20U][column];
}

/*===========================================================================
 * 全屏刷新（帧缓冲 → 硬件 SPI → LCD GRAM，约 88ms）
 *===========================================================================*/
void bsp_lcd147_refresh(void)
{
    const uint16 fg = RGB565_WHITE;
    const uint16 bg = RGB565_BLACK;

    LCD147_CS(0);
    lcd147_set_window(0, 0, LCD147_WIDTH - 1U, LCD147_HEIGHT - 1U);

    for (uint16 y = 0; y < LCD147_HEIGHT; y++)
    {
        const uint8 *row = &lcd147_framebuffer[(uint32)y * LCD147_BYTES_PER_ROW];
        for (uint16 x = 0; x < LCD147_WIDTH; x++)
        {
            lcd147_write_data16((row[x >> 3] & (0x80U >> (x & 0x07U))) ? fg : bg);
        }
    }

    LCD147_CS(1);
}

/*===========================================================================
 * 文字显示
 *===========================================================================*/
static void lcd147_draw_char(uint16 x, uint16 line, char ch)
{
    const uint8 *bitmap;
    uint16 y = line * 8U;

    if (ch < 0x20 || ch > 0x7E) return;
    if ((x + 6U) > LCD147_WIDTH) return;
    if ((y + 8U) > LCD147_HEIGHT) return;

    bitmap = font_6x8[(uint8)ch - 0x20U];
    for (uint8 col = 0; col < 6U; col++)
    {
        uint8 bits = bitmap[col];
        for (uint8 row = 0; row < 8U; row++)
        {
            bsp_lcd147_draw_point(x + col, y + row, (bits >> row) & 0x01U);
        }
    }
}

void bsp_lcd147_show_string(uint16 x, uint16 line, const char *str)
{
    if (!str) return;

    while (*str)
    {
        lcd147_draw_char(x, line, *str);
        x += 6U;
        str++;
        if (x >= LCD147_WIDTH) break;
    }
}

void bsp_lcd147_show_string_scaled(uint16 x, uint16 y, const char *str, uint8 scale)
{
    if (!str || scale == 0U) return;

    while (*str)
    {
        for (uint8 col = 0; col < 6U; col++)
        {
            uint8 bits = bsp_lcd147_get_ascii_column(*str, col);
            for (uint8 row = 0; row < 8U; row++)
            {
                uint8 color = (bits >> row) & 0x01U;
                for (uint8 sy = 0; sy < scale; sy++)
                    for (uint8 sx = 0; sx < scale; sx++)
                        bsp_lcd147_draw_point(x + col * scale + sx,
                                             y + row * scale + sy, color);
            }
        }

        x += 6U * scale;
        str++;
        if (x >= LCD147_WIDTH) break;
    }
}

void bsp_lcd147_show_int(uint16 x, uint16 line, int32 val, uint8 len)
{
    char buf[16];
    (void)snprintf(buf, sizeof(buf), "%*ld", len, (long)val);
    bsp_lcd147_show_string(x, line, buf);
}

void bsp_lcd147_show_uint(uint16 x, uint16 line, uint32 val, uint8 len)
{
    char buf[16];
    (void)snprintf(buf, sizeof(buf), "%*lu", len, (unsigned long)val);
    bsp_lcd147_show_string(x, line, buf);
}

void bsp_lcd147_show_float(uint16 x, uint16 line, double val, uint8 total, uint8 decimal)
{
    char fmt[16];
    char buf[24];
    (void)snprintf(fmt, sizeof(fmt), "%%%u.%uf", total, decimal);
    (void)snprintf(buf, sizeof(buf), fmt, val);
    bsp_lcd147_show_string(x, line, buf);
}
