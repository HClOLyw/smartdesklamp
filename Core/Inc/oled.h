#ifndef __OLED_H
#define __OLED_H

#include "main.h"

/* OLED 初始化。 */
void OLED_Init(void);
/* 整屏清空。 */
void OLED_Clear(void);
/* 清空局部区域，区域按 page 对齐。 */
void OLED_ClearArea(uint8_t x, uint8_t page, uint8_t width, uint8_t page_count);
/* 显示单个 ASCII 字符。 */
void OLED_ShowChar(uint8_t x, uint8_t page, char chr);
/* 显示 ASCII 字符串。 */
void OLED_ShowString(uint8_t x, uint8_t page, const char *str);
/* 显示单个 16x16 汉字字模。 */
void OLED_ShowChinese(uint8_t x, uint8_t page, const uint8_t *glyph);
/* 连续显示多个 16x16 汉字。 */
void OLED_ShowChineseString(uint8_t x, uint8_t page, const uint8_t *glyphs[], uint8_t count);
/* 显示固定宽度数字。 */
void OLED_ShowNumber(uint8_t x, uint8_t page, uint32_t num, uint8_t width);

extern const uint8_t hz_mo[32];
extern const uint8_t hz_shi[32];
extern const uint8_t hz_zi[32];
extern const uint8_t hz_dong[32];
extern const uint8_t hz_shou[32];
extern const uint8_t hz_dang[32];
extern const uint8_t hz_qian[32];
extern const uint8_t hz_zhao[32];
extern const uint8_t hz_du[32];
extern const uint8_t hz_tiao[32];
extern const uint8_t hz_guang[32];
extern const uint8_t hz_ji[32];
extern const uint8_t hz_shu[32];
extern const uint8_t hz_ren[32];
extern const uint8_t hz_ti[32];
extern const uint8_t hz_zhuang[32];
extern const uint8_t hz_tai[32];
extern const uint8_t hz_you[32];
extern const uint8_t hz_wu[32];

#endif
