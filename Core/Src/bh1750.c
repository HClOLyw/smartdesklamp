#include "bh1750.h"
#include "i2c.h"

/* BH1750 的 7 位 I2C 地址。 */
#define BH1750_ADDR 0x23

/* 向 BH1750 发送一个命令字节。 */
static HAL_StatusTypeDef BH1750_Write(uint8_t cmd)
{
  return HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(BH1750_ADDR << 1), &cmd, 1, 100);
}

/* 初始化 BH1750：
 * 先上电，再切到连续高分辨率模式。
 * 在该模式下传感器会持续测量，主循环可随时读取最新值。
 */
void BH1750_Init(void)
{
  uint8_t cmd;

  cmd = 0x01;
  BH1750_Write(cmd);
  cmd = 0x10;
  BH1750_Write(cmd);
}

/* 读取原始测量值并换算为 lux：
 * BH1750 在默认 MTreg、连续高分辨率模式下，
 * 典型换算公式为 lux = raw / 1.2。
 */
uint16_t BH1750_ReadLux(void)
{
  uint8_t buf[2] = {0};
  if (HAL_I2C_Master_Receive(&hi2c1, (uint16_t)(BH1750_ADDR << 1), buf, 2, 100) != HAL_OK)
  {
    return 0;
  }

  uint16_t raw = ((uint16_t)buf[0] << 8) | buf[1];
  return (uint16_t)((float)raw / 1.2f);
}

/* 用一个很轻量的滑动平均滤波降低显示抖动：
 * 每次读取一个新值，放入长度为 N 的缓冲区，
 * 再对已有样本取平均，避免 OLED 上的照度数字频繁跳动。
 */
uint16_t BH1750_ReadLux_Filtered(void)
{
  enum { N = 5 };
  static uint16_t samples[N] = {0};
  static uint8_t index = 0;
  static uint8_t filled = 0;
  uint32_t sum = 0;
  uint16_t lux = BH1750_ReadLux();

  samples[index] = lux;
  index = (uint8_t)((index + 1U) % N);
  if (filled < N) filled++;

  for (uint8_t i = 0; i < filled; i++)
  {
    sum += samples[i];
  }

  if (filled == 0) return lux;
  return (uint16_t)(sum / filled);
}
