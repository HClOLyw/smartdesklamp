#include "pwm_ctrl.h"
#include "tim.h"

/* PWM 输出由 TIM1 的通道 1 提供，对应引脚 PA8。 */
void PWM_Init(void)
{
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
}

/* 更新 TIM1 的比较值：
 * 本工程约定占空比范围为 0~1000，对应 0%~100%。
 */
void PWM_SetDutyCycle(uint16_t duty)
{
  if (duty > 1000) duty = 1000;
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty);
}
