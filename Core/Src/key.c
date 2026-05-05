#include "key.h"

/* PB12 按键说明：
 * - 低电平表示按下
 * - 短按触发单击事件
 * - 长按触发长按事件
 */
void KEY_Init(void)
{
}

/* 每 10ms 扫描一次按键：
 * - 通过 DEBOUNCE_TICKS 做软件消抖
 * - 通过 LONG_TICKS 判断长按阈值
 * - 返回值是离散事件，而不是电平状态
 */
KeyEvent_t KEY_Scan_10ms(void)
{
  /* 3 个采样周期稳定后认定电平有效；80*10ms=800ms 视为长按。 */
  enum { DEBOUNCE_TICKS = 3, LONG_TICKS = 80 };
  static GPIO_PinState stable = GPIO_PIN_SET;
  static GPIO_PinState last_raw = GPIO_PIN_SET;
  static uint8_t debounce = 0;
  static uint16_t press_ticks = 0;
  static uint8_t long_sent = 0;

  GPIO_PinState raw = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);

  /* 原始电平连续一致，说明可能进入稳定状态。 */
  if (raw == last_raw)
  {
    if (debounce < DEBOUNCE_TICKS)
    {
      debounce++;
    }
    else if (stable != raw)
    {
      /* 原始电平连续稳定足够久后，才更新稳定态。 */
      stable = raw;
      if (stable == GPIO_PIN_RESET)
      {
        /* 检测到“稳定按下”的起点，重新开始计时。 */
        press_ticks = 0;
        long_sent = 0;
      }
      else if (!long_sent && press_ticks > 0 && press_ticks < LONG_TICKS)
      {
        /* 松开前没有触发长按，则判定为一次短按。 */
        return KEY_EVENT_CLICK;
      }
    }
  }
  else
  {
    /* 原始电平发生变化，重新开始消抖。 */
    last_raw = raw;
    debounce = 0;
  }

  /* 在“稳定按下”状态下持续累计按压时间。 */
  if (stable == GPIO_PIN_RESET)
  {
    if (press_ticks < 0xFFFF) press_ticks++;
    if (press_ticks >= LONG_TICKS && !long_sent)
    {
      long_sent = 1;
      return KEY_EVENT_LONG;
    }
  }

  return KEY_EVENT_NONE;
}
