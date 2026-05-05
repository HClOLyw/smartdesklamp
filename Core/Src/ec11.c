#include "ec11.h"

/* EC11 编码器处理：
 * A/B 两相信号共同决定旋转方向；
 * 这里采用四相状态表解码，把抖动较多的边沿变化累积成完整一步。
 */
void EC11_Init(void)
{
  /* GPIO/EXTI are configured in MX_GPIO_Init(). */
}

/* 返回值说明：
 * +1：检测到一个正向完整步进
 * -1：检测到一个反向完整步进
 *  0：当前还不足以构成完整步进
 */
int8_t EC11_Process_10ms(void)
{
  static uint8_t last_state = 0;
  static int8_t accum = 0;
  /* 四相编码状态转移表：
   * 上一状态和当前状态组合成 4bit 索引，
   * 表中给出这一次边沿变化对应的方向增量。
   */
  static const int8_t transition_table[16] = {
    0, -1,  1,  0,
    1,  0,  0, -1,
   -1,  0,  0,  1,
    0,  1, -1,  0
  };

  uint8_t a = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) ? 1U : 0U;
  uint8_t b = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET) ? 1U : 0U;
  uint8_t state = (uint8_t)((a << 1) | b);
  uint8_t index = (uint8_t)((last_state << 2) | state);
  int8_t step = transition_table[index];
  last_state = state;

  /* 只要检测到有效边沿，就先累计“1/4 步”。
   * 累计到 4 或 -4 时，才输出一个完整步进。
   */
  if (step != 0)
  {
    accum += step;
    if (accum >= 4)
    {
      accum = 0;
      return +1;
    }
    if (accum <= -4)
    {
      accum = 0;
      return -1;
    }
  }

  return 0;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  /* 当前版本没有在 EXTI 回调里做业务处理，保留空实现即可。 */
  (void)GPIO_Pin;
}
