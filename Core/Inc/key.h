#ifndef __KEY_H
#define __KEY_H

#include "main.h"

typedef enum {
  KEY_EVENT_NONE = 0,  /* 本次扫描没有产生新事件 */
  KEY_EVENT_CLICK,     /* 检测到一次短按 */
  KEY_EVENT_LONG       /* 检测到一次长按 */
} KeyEvent_t;

/* 按键模块初始化。当前按键 GPIO 由 MX_GPIO_Init 完成，这里保留统一接口。 */
void KEY_Init(void);
/* 每 10ms 调用一次，返回离散按键事件。 */
KeyEvent_t KEY_Scan_10ms(void);

#endif
