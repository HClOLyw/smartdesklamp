#ifndef __HCSR501_H
#define __HCSR501_H

#include "main.h"

/* HC-SR501 初始化。 */
void HCSR501_Init(void);
/* 读取人体检测结果，返回 0/1。 */
uint8_t HCSR501_Read(void);

#endif
