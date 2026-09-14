/**
 * @file    bsp_oled.c
 * @brief   I2C OLED 驱动实现 — SSD1306 128x64 + 逐飞库 soft_iic
 * @author  Reach
 * @version v1.0
 *
 * 显存策略：完整帧缓冲区（128 × 64 ÷ 8 = 1024 字节）
 * 显示流程：用户调用画点/字函数修改显存 → bsp_oled_refresh() 整屏刷新
 * 刷新方式：按页（8行/页 × 128列）逐页写入 SSD1306
 */

#include "bsp_oled.h"
#include "zf_driver_soft_iic.h"
#include "zf_driver_delay.h"

#ifndef OLED_USE
#define OLED_USE 0
#endif

#if OLED_USE

/* ==========================================================================
 * SSD1306 I2C 协议常量
 * ========================================================================== */
#define OLED_CMD_MODE        0x00     /* 控制字节：命令模式 */
#define OLED_DATA_MODE       0x40     /* 控制字节：数据模式 */
#define OLED_PAGE_COUNT      8        /* 128×64 共 8 个页 */

/* ==========================================================================
 * 6×8 ASCII 字模表（字符 0x20 ~ 0x7E，每个字符 6 字节）
 * 用 const 放在 Flash 中不占 SRAM
 * ========================================================================== */
static const uint8 font_6x8[][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00}, /* SPACE */
    {0x00,0x00,0x5F,0x00,0x00,0x00}, /* ! */
    {0x00,0x07,0x00,0x07,0x00,0x00}, /* " */
    {0x14,0x7F,0x14,0x7F,0x14,0x00}, /* # */
    {0x24,0x2A,0x7F,0x2A,0x12,0x00}, /* $ */
    {0x23,0x13,0x08,0x64,0x62,0x00}, /* % */
    {0x36,0x49,0x55,0x22,0x50,0x00}, /* & */
    {0x00,0x05,0x03,0x00,0x00,0x00}, /* ' */
    {0x00,0x1C,0x22,0x41,0x00,0x00}, /* ( */
    {0x00,0x41,0x22,0x1C,0x00,0x00}, /* ) */
    {0x08,0x2A,0x1C,0x2A,0x08,0x00}, /* * */
    {0x08,0x08,0x3E,0x08,0x08,0x00}, /* + */
    {0x00,0x50,0x30,0x00,0x00,0x00}, /* , */
    {0x08,0x08,0x08,0x08,0x08,0x00}, /* - */
    {0x00,0x60,0x60,0x00,0x00,0x00}, /* . */
    {0x20,0x10,0x08,0x04,0x02,0x00}, /* / */
    {0x3E,0x51,0x49,0x45,0x3E,0x00}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46,0x00}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31,0x00}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10,0x00}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39,0x00}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30,0x00}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03,0x00}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36,0x00}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E,0x00}, /* 9 */
    {0x00,0x36,0x36,0x00,0x00,0x00}, /* : */
    {0x00,0x56,0x36,0x00,0x00,0x00}, /* ; */
    {0x00,0x08,0x14,0x22,0x41,0x00}, /* < */
    {0x14,0x14,0x14,0x14,0x14,0x00}, /* = */
    {0x41,0x22,0x14,0x08,0x00,0x00}, /* > */
    {0x02,0x01,0x51,0x09,0x06,0x00}, /* ? */
    {0x32,0x49,0x79,0x41,0x3E,0x00}, /* @ */
    {0x7E,0x11,0x11,0x11,0x7E,0x00}, /* A */
    {0x7F,0x49,0x49,0x49,0x36,0x00}, /* B */
    {0x3E,0x41,0x41,0x41,0x22,0x00}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C,0x00}, /* D */
    {0x7F,0x49,0x49,0x49,0x41,0x00}, /* E */
    {0x7F,0x09,0x09,0x01,0x01,0x00}, /* F */
    {0x3E,0x41,0x41,0x51,0x32,0x00}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F,0x00}, /* H */
    {0x00,0x41,0x7F,0x41,0x00,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01,0x00}, /* J */
    {0x7F,0x08,0x14,0x22,0x41,0x00}, /* K */
    {0x7F,0x40,0x40,0x40,0x40,0x00}, /* L */
    {0x7F,0x02,0x04,0x02,0x7F,0x00}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F,0x00}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E,0x00}, /* O */
    {0x7F,0x09,0x09,0x09,0x06,0x00}, /* P */
    {0x3E,0x41,0x51,0x21,0x5E,0x00}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46,0x00}, /* R */
    {0x46,0x49,0x49,0x49,0x31,0x00}, /* S */
    {0x01,0x01,0x7F,0x01,0x01,0x00}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F,0x00}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F,0x00}, /* V */
    {0x7F,0x20,0x18,0x20,0x7F,0x00}, /* W */
    {0x63,0x14,0x08,0x14,0x63,0x00}, /* X */
    {0x03,0x04,0x78,0x04,0x03,0x00}, /* Y */
    {0x61,0x51,0x49,0x45,0x43,0x00}, /* Z */
    {0x00,0x00,0x7F,0x41,0x41,0x00}, /* [ */
    {0x02,0x04,0x08,0x10,0x20,0x00}, /* \ */
    {0x41,0x41,0x7F,0x00,0x00,0x00}, /* ] */
    {0x04,0x02,0x01,0x02,0x04,0x00}, /* ^ */
    {0x40,0x40,0x40,0x40,0x40,0x00}, /* _ */
    {0x00,0x01,0x02,0x04,0x00,0x00}, /* ` */
    {0x20,0x54,0x54,0x54,0x78,0x00}, /* a */
    {0x7F,0x48,0x44,0x44,0x38,0x00}, /* b */
    {0x38,0x44,0x44,0x44,0x20,0x00}, /* c */
    {0x38,0x44,0x44,0x48,0x7F,0x00}, /* d */
    {0x38,0x54,0x54,0x54,0x18,0x00}, /* e */
    {0x08,0x7E,0x09,0x01,0x02,0x00}, /* f */
    {0x08,0x14,0x54,0x54,0x3C,0x00}, /* g */
    {0x7F,0x08,0x04,0x04,0x78,0x00}, /* h */
    {0x00,0x44,0x7D,0x40,0x00,0x00}, /* i */
    {0x20,0x40,0x44,0x3D,0x00,0x00}, /* j */
    {0x00,0x7F,0x10,0x28,0x44,0x00}, /* k */
    {0x00,0x41,0x7F,0x40,0x00,0x00}, /* l */
    {0x7C,0x04,0x18,0x04,0x78,0x00}, /* m */
    {0x7C,0x08,0x04,0x04,0x78,0x00}, /* n */
    {0x38,0x44,0x44,0x44,0x38,0x00}, /* o */
    {0x7C,0x14,0x14,0x14,0x08,0x00}, /* p */
    {0x08,0x14,0x14,0x18,0x7C,0x00}, /* q */
    {0x7C,0x08,0x04,0x04,0x08,0x00}, /* r */
    {0x48,0x54,0x54,0x54,0x20,0x00}, /* s */
    {0x04,0x3F,0x44,0x40,0x20,0x00}, /* t */
    {0x3C,0x40,0x40,0x20,0x7C,0x00}, /* u */
    {0x1C,0x20,0x40,0x20,0x1C,0x00}, /* v */
    {0x3C,0x40,0x30,0x40,0x3C,0x00}, /* w */
    {0x44,0x28,0x10,0x28,0x44,0x00}, /* x */
    {0x0C,0x50,0x50,0x50,0x3C,0x00}, /* y */
    {0x44,0x64,0x54,0x4C,0x44,0x00}, /* z */
    {0x00,0x08,0x36,0x41,0x00,0x00}, /* { */
    {0x00,0x00,0x7F,0x00,0x00,0x00}, /* | */
    {0x00,0x41,0x36,0x08,0x00,0x00}, /* } */
    {0x08,0x04,0x08,0x10,0x08,0x00}, /* ~ */
};

/* ==========================================================================
 * 模块内部变量
 * ========================================================================== */
static soft_iic_info_struct s_oled_iic;    /* I2C 设备句柄 */
uint8 oled_framebuffer[OLED_PAGE_COUNT * OLED_MAX_X];  /* 显存 1024B，供 Astra UI 读取 */

/* ==========================================================================
 * 内部辅助函数
 * ========================================================================== */

/**
 * @brief   发送 SSD1306 命令字节
 * @note    I2C 格式: START + addr(W) + 0x00(CMD) + cmd_byte + STOP
 */
static void bsp_oled_write_cmd(uint8 cmd)
{
    soft_iic_write_8bit_register(&s_oled_iic, OLED_CMD_MODE, cmd);
}

/**
 * @brief   批量发送显存数据（一页 128 字节）
 * @note    用 soft_iic_write_8bit_registers 实现连续传输：
 *          START + addr(W) + 0x40(DATA) + data[0..127] + STOP
 */
static void bsp_oled_write_page(uint8 page)
{
    uint8 *buf = oled_framebuffer + (page * OLED_MAX_X);
    soft_iic_write_8bit_registers(&s_oled_iic, OLED_DATA_MODE, buf, OLED_MAX_X);
}

/**
 * @brief   设置 SSD1306 光标位置（页地址 + 列地址）
 * @param   page    页号 (0~7)
 * @param   col     列号 (0~127)
 */
static void bsp_oled_set_pos(uint8 page, uint8 col)
{
    bsp_oled_write_cmd(0xB0 | (page & 0x07));     // 页地址 (0xB0 ~ 0xB7)
    bsp_oled_write_cmd(0x00 | (col  & 0x0F));      // 列低4位
    bsp_oled_write_cmd(0x10 | ((col >> 4) & 0x0F)); // 列高4位
}

/* ==========================================================================
 * 公共函数
 * ========================================================================== */

/**
 * @brief   OLED 初始化
 * @note    完成后屏幕处于显示开启状态，需手动清屏
 */
void bsp_oled_init(void)
{
    /* 1. 初始化软件 I2C 模块 */
    soft_iic_init(&s_oled_iic, OLED_I2C_ADDR, OLED_I2C_DELAY,
                  OLED_SCL_PIN, OLED_SDA_PIN);

    /* 2. SSD1306 初始化序列（参考数据手册） */
    bsp_oled_write_cmd(0xAE);   // 关闭显示

    bsp_oled_write_cmd(0xD5);   // 设置时钟分频
    bsp_oled_write_cmd(0x80);   // 分频比=1, 频率=8

    bsp_oled_write_cmd(0xA8);   // 设置多路复用
    bsp_oled_write_cmd(0x3F);   // 64 行 (128×64)

    bsp_oled_write_cmd(0xD3);   // 设置显示偏移
    bsp_oled_write_cmd(0x00);   // 偏移=0

    bsp_oled_write_cmd(0x40);   // 设置起始行=0

    bsp_oled_write_cmd(0x8D);   // 电荷泵配置
    bsp_oled_write_cmd(0x14);   // 使能电荷泵（3.3V 供电必须开）

    bsp_oled_write_cmd(0x20);   // 内存寻址模式
    bsp_oled_write_cmd(0x00);   // 水平寻址

    bsp_oled_write_cmd(0xA1);   // 段重映射（左右镜像，A0=不镜像, A1=镜像）

    bsp_oled_write_cmd(0xC8);   // COM 扫描方向（C0=正常, C8=上下镜像）

    bsp_oled_write_cmd(0xDA);   // COM 引脚配置
    bsp_oled_write_cmd(0x12);   // 交替排列

    bsp_oled_write_cmd(0x81);   // 对比度
    bsp_oled_write_cmd(0xCF);   // 对比度值 (0~255)

    bsp_oled_write_cmd(0xD9);   // 预充电周期
    bsp_oled_write_cmd(0xF1);   // 周期=1, 相位=15

    bsp_oled_write_cmd(0xDB);   // VCOMH 电压
    bsp_oled_write_cmd(0x40);   // ~0.77 × VCC

    bsp_oled_write_cmd(0xA4);   // 正常显示模式（不反白）
    bsp_oled_write_cmd(0xA6);   // 正常显示（不反转）

    bsp_oled_write_cmd(0xAF);   // 开启显示

    /* 3. 清屏 */
    bsp_oled_clear();
    bsp_oled_refresh();
}

/**
 * @brief   清屏
 */
void bsp_oled_clear(void)
{
    for (uint16 i = 0; i < sizeof(oled_framebuffer); i++)
    {
        oled_framebuffer[i] = 0x00;
    }
}

/**
 * @brief   全屏填充
 */
void bsp_oled_fill(uint8 color)
{
    uint8 val = color ? 0xFF : 0x00;
    for (uint16 i = 0; i < sizeof(oled_framebuffer); i++)
    {
        oled_framebuffer[i] = val;
    }
}

/**
 * @brief   画点 (x: 0~127, y: 0~63)
 */
void bsp_oled_draw_point(uint8 x, uint8 y, uint8 color)
{
    if (x >= OLED_MAX_X || y >= OLED_MAX_Y) return;

    uint16 idx = (y / 8) * OLED_MAX_X + x;  /* 页偏移 + 列偏移 */
    if (color)
        oled_framebuffer[idx] |=  (1 << (y % 8));
    else
        oled_framebuffer[idx] &= ~(1 << (y % 8));
}

/**
 * @brief   刷新全屏（将显存写入 SSD1306）
 * @note    逐页写入，每页 128 字节，共 8 页
 */
void bsp_oled_refresh(void)
{
    for (uint8 page = 0; page < OLED_PAGE_COUNT; page++)
    {
        bsp_oled_set_pos(page, 0);
        bsp_oled_write_page(page);
    }
}

/**
 * @brief   在指定位置画一个 6×8 字符
 * @param   ch      字符
 * @param   col     列（像素）
 * @param   line    行号（0~7，对应 page）
 */
static void bsp_oled_draw_char(uint8 col, uint8 line, char ch)
{
    if (ch < 0x20 || ch > 0x7E) return;  /* 不可打印字符 */
    if (col + 6 > OLED_MAX_X)   return;  /* 超出屏幕右边界 */

    const uint8 *bitmap = font_6x8[ch - 0x20];

    for (uint8 i = 0; i < 6; i++)
    {
        uint8 data = bitmap[i];
        uint16 idx = line * OLED_MAX_X + col + i;

        oled_framebuffer[idx] = data;
    }
}

/**
 * @brief   显示字符串
 */
void bsp_oled_show_string(uint8 x, uint8 line, const char *str)
{
    if (line >= 8) return;  /* OLED 共 8 行 */

    while (*str)
    {
        if (x + 6 > OLED_MAX_X) break;  /* 超出屏幕自动换行 */
        bsp_oled_draw_char(x, line, *str);
        x += 6;
        str++;
    }
}

/**
 * @brief   显示有符号整数
 */
void bsp_oled_show_int(uint8 x, uint8 line, int32 val, uint8 len)
{
    char buf[12];
    uint8 i = 0;
    uint8 negative = 0;

    if (val < 0)
    {
        negative = 1;
        val = -val;
    }

    /* 逆序生成数字 */
    do {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    } while (val > 0 && i < 11);

    /* 补负号 */
    if (negative)
    {
        buf[i++] = '-';
    }

    /* 补空格到指定长度 */
    while (i < len && i < 11)
    {
        buf[i++] = ' ';
    }

    /* 反序输出 */
    while (i > 0)
    {
        bsp_oled_draw_char(x, line, buf[--i]);
        x += 6;
    }
}

/**
 * @brief   显示无符号整数
 */
void bsp_oled_show_uint(uint8 x, uint8 line, uint32 val, uint8 len)
{
    char buf[12];
    uint8 i = 0;

    do {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    } while (val > 0 && i < 11);

    while (i < len && i < 11)
    {
        buf[i++] = ' ';
    }

    while (i > 0)
    {
        bsp_oled_draw_char(x, line, buf[--i]);
        x += 6;
    }
}

/**
 * @brief   显示浮点数
 */
void bsp_oled_show_float(uint8 x, uint8 line, double val, uint8 total, uint8 decimal)
{
    char buf[16];
    uint8 i = 0;
    uint8 negative = 0;

    if (val < 0)
    {
        negative = 1;
        val = -val;
    }

    /* 小数部分 */
    uint32 dec = 1;
    for (uint8 d = 0; d < decimal; d++)
    {
        dec *= 10;
    }
    uint32 int_part  = (uint32)val;
    uint32 frac_part = (uint32)((val - (double)int_part) * (double)dec + 0.5);

    /* 处理进位 */
    if (frac_part >= dec)
    {
        int_part++;
        frac_part -= dec;
    }

    /* 小数部分数字 */
    for (uint8 d = 0; d < decimal; d++)
    {
        buf[i++] = '0' + (frac_part % 10);
        frac_part /= 10;
    }

    /* 小数点 */
    if (decimal > 0)
    {
        buf[i++] = '.';
    }

    /* 整数部分 */
    do {
        buf[i++] = '0' + (int_part % 10);
        int_part /= 10;
    } while (int_part > 0);

    if (negative)
    {
        buf[i++] = '-';
    }

    /* 补空格 */
    while (i < total && i < 15)
    {
        buf[i++] = ' ';
    }

    /* 反序输出 */
    while (i > 0)
    {
        bsp_oled_draw_char(x, line, buf[--i]);
        x += 6;
    }
}

/**
 * @brief   显示中文字符（16×16 点阵）
 * @param   bitmap  指向 32 字节字模数据
 */
void bsp_oled_show_chinese(uint8 x, uint8 line, const uint8 *bitmap)
{
    if (x + 16 > OLED_MAX_X || (line + 1) >= 8) return;

    /* 上半部分（8 行） */
    uint8 page_top = line;
    for (uint8 i = 0; i < 16; i++)
    {
        oled_framebuffer[page_top * OLED_MAX_X + x + i] = bitmap[i];
    }

    /* 下半部分（8 行） */
    uint8 page_bot = line + 1;
    for (uint8 i = 0; i < 16; i++)
    {
        oled_framebuffer[page_bot * OLED_MAX_X + x + i] = bitmap[16 + i];
    }
}

/**
 * @brief   填充矩形
 */
void bsp_oled_fill_rect(uint8 x, uint8 y, uint8 w, uint8 h, uint8 color)
{
    for (uint8 dy = 0; dy < h; dy++)
    {
        for (uint8 dx = 0; dx < w; dx++)
        {
            bsp_oled_draw_point(x + dx, y + dy, color);
        }
    }
}

/**
 * @brief   画水平线
 */
void bsp_oled_draw_hline(uint8 x, uint8 y, uint8 len, uint8 color)
{
    bsp_oled_fill_rect(x, y, len, 1, color);
}

/**
 * @brief   画垂直线
 */
void bsp_oled_draw_vline(uint8 x, uint8 y, uint8 len, uint8 color)
{
    bsp_oled_fill_rect(x, y, 1, len, color);
}

#else

uint8 oled_framebuffer[8 * 128];

void bsp_oled_init(void) {}
void bsp_oled_clear(void) {}
void bsp_oled_fill(uint8 color) { (void)color; }
void bsp_oled_draw_point(uint8 x, uint8 y, uint8 color) { (void)x; (void)y; (void)color; }
void bsp_oled_refresh(void) {}
void bsp_oled_show_string(uint8 x, uint8 line, const char *str) { (void)x; (void)line; (void)str; }
void bsp_oled_show_int(uint8 x, uint8 line, int32 val, uint8 len) { (void)x; (void)line; (void)val; (void)len; }
void bsp_oled_show_uint(uint8 x, uint8 line, uint32 val, uint8 len) { (void)x; (void)line; (void)val; (void)len; }
void bsp_oled_show_float(uint8 x, uint8 line, double val, uint8 total, uint8 decimal) { (void)x; (void)line; (void)val; (void)total; (void)decimal; }
void bsp_oled_show_chinese(uint8 x, uint8 line, const uint8 *bitmap) { (void)x; (void)line; (void)bitmap; }
void bsp_oled_fill_rect(uint8 x, uint8 y, uint8 w, uint8 h, uint8 color) { (void)x; (void)y; (void)w; (void)h; (void)color; }
void bsp_oled_draw_hline(uint8 x, uint8 y, uint8 len, uint8 color) { (void)x; (void)y; (void)len; (void)color; }
void bsp_oled_draw_vline(uint8 x, uint8 y, uint8 len, uint8 color) { (void)x; (void)y; (void)len; (void)color; }

#endif
