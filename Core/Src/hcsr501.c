#include "hcsr501.h"

/* 人体红外输入说明：
 * HC-SR501 的数字输出接到 PB13。
 * 当前 GPIO 配置为下拉输入，便于在传感器未驱动时保持低电平。
 */
void HCSR501_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* 返回人体检测结果：
 * - 高电平：检测到人体
 * - 低电平：未检测到人体
 */
uint8_t HCSR501_Read(void)
{
  return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == GPIO_PIN_SET ? 1U : 0U;
}
