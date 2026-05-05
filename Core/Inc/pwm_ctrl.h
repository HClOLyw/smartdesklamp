#ifndef __PWM_CTRL_H
#define __PWM_CTRL_H

#include "main.h"

/* PWM 模块初始化。 */
void PWM_Init(void);
/* 设置占空比，范围 0~1000。 */
void PWM_SetDutyCycle(uint16_t duty);

#endif
