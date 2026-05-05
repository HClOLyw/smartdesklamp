#ifndef __EC11_H
#define __EC11_H

#include "main.h"

/* EC11 编码器接口：
 * 调用者按固定周期执行 EC11_Process_10ms()，
 * 返回 -1/0/+1 表示一步反向/无动作/一步正向。
 */
void EC11_Init(void);
int8_t EC11_Process_10ms(void);

#endif
