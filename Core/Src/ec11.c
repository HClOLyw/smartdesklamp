#include "ec11.h"

/* EC11 编码器处理说明：
 * 1. PA0(A 相) 配置为 EXTI 双边沿中断，任意翻转都会进入回调。
 * 2. 在中断里读取 A/B 当前电平，根据“当前 A 相电平 + B 相电平”的组合判断方向。
 * 3. 中断只负责把结果累计到全局计数中，主循环仍通过 EC11_Process_10ms()
 *    以原来的接口取出 -1/0/+1，因而 main.c 无需修改。
 */

/* 由 EXTI 中断累计的编码器步数。
 * 顺时针记为正，逆时针记为负。
 */
static volatile int8_t s_ec11_delta = 0;

void EC11_Init(void)
{
  /* GPIO/EXTI 由 MX_GPIO_Init() 完成初始化，这里无需额外操作。 */
}

/* 保持原有接口不变：
 * 每 10ms 由主循环调用一次，取走当前累计的一步结果。
 * 若累计值大于 0，返回 +1；
 * 若累计值小于 0，返回 -1；
 * 否则返回 0。
 */
int8_t EC11_Process_10ms(void)
{
  int8_t ret = 0;

  __disable_irq();
  if (s_ec11_delta > 0)
  {
    s_ec11_delta--;
    ret = +1;
  }
  else if (s_ec11_delta < 0)
  {
    s_ec11_delta++;
    ret = -1;
  }
  __enable_irq();

  return ret;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_0)
  {
    uint8_t a = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) ? 1U : 0U;
    uint8_t b = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_SET) ? 1U : 0U;

    /* 采用 A 相双边沿解码：
     * - A 上升沿时，B=0/1 分别对应两个相反方向
     * - A 下降沿时，B=1/0 分别对应两个相反方向
     * 若方向与实际相反，只需把下面的 +1/-1 对调即可。
     */
    if (a != 0U)
    {
      if (b == 0U)
      {
        if (s_ec11_delta < 100) s_ec11_delta++;
      }
      else
      {
        if (s_ec11_delta > -100) s_ec11_delta--;
      }
    }
    else
    {
      if (b != 0U)
      {
        if (s_ec11_delta < 100) s_ec11_delta++;
      }
      else
      {
        if (s_ec11_delta > -100) s_ec11_delta--;
      }
    }
  }
}
