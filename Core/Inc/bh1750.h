#ifndef __BH1750_H
#define __BH1750_H

#include "main.h"

/* BH1750 初始化为连续高分辨率模式。 */
void BH1750_Init(void);
/* 读取一次即时 lux 值。 */
uint16_t BH1750_ReadLux(void);
/* 读取经过简单滤波后的 lux 值。 */
uint16_t BH1750_ReadLux_Filtered(void);

#endif
