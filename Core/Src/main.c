#include "main.h"
#include "gpio.h"
#include "i2c.h"
#include "tim.h"
#include <stdio.h>

#include "pwm_ctrl.h"
#include "key.h"
#include "ec11.h"
#include "hcsr501.h"
#include "bh1750.h"
#include "oled.h"

void SystemClock_Config(void);

/* 灯控主状态变量：
 * - Power_State：整灯开关状态，1 表示点亮，0 表示熄灭
 * - Auto_Mode：自动调光模式开关，1 表示根据环境光闭环调节，0 表示手动调节
 * - Target_Brightness：目标亮度等级，范围 0~199
 * - Current_PWM：当前实际输出等级，范围 0~199，按渐变方式逼近目标值
 */
static uint8_t Power_State = 1;
static uint8_t Auto_Mode = 1;
static uint16_t Target_Brightness = 120;
static uint16_t Current_PWM = 0;
static uint16_t Ambient_Lux = 0;
static uint16_t PIR_Timeout = 600;
static uint16_t PIR_Count = 0;

#define TARGET_LUX 500U
#define DEADBAND 30U

static uint16_t Gamma_Table[200];

/* 预先生成一张“伽马近似表”：
 * 用户看到的亮度感受并不是线性的，因此这里把 0~199 的亮度等级
 * 映射到 0~1000 的 PWM 比较值，让低亮度区域变化更柔和，不会显得突兀。
 */
static void Gamma_Init(void)
{
  for (int i = 0; i < 200; i++)
  {
    uint64_t v = (uint64_t)i * (uint64_t)i * (uint64_t)i * 1000ULL;
    v /= 199ULL * 199ULL * 199ULL;
    if (v > 1000ULL) v = 1000ULL;
    Gamma_Table[i] = (uint16_t)v;
  }
}

/* 对亮度相关变量做边界保护，防止编码器或逻辑计算越界。 */
static void Clamp_Values(void)
{
  if (Target_Brightness > 199) Target_Brightness = 199;
  if (Current_PWM > 199) Current_PWM = 199;
}

/* OLED 静态界面只绘制一次：
 * 左侧中文标签和冒号属于固定内容，不需要反复整屏刷新，
 * 这样可以减少 I2C 通信量并明显减轻闪屏。
 */
static void OLED_DrawStaticUI(void)
{
  static const uint8_t *label_mode[] = {hz_mo, hz_shi};
  static const uint8_t *label_current[] = {hz_dang, hz_qian, hz_zhao, hz_du};
  static const uint8_t *label_adjust[] = {hz_tiao, hz_guang, hz_ji, hz_shu};
  static const uint8_t *label_human[] = {hz_ren, hz_ti, hz_zhuang, hz_tai};

  OLED_Clear();

  OLED_ShowChineseString(0, 0, label_mode, 2);
  OLED_ShowString(32, 0, ":");

  OLED_ShowChineseString(0, 2, label_current, 4);
  OLED_ShowString(64, 2, ":");

  OLED_ShowChineseString(0, 4, label_adjust, 4);
  OLED_ShowString(64, 4, ":");

  OLED_ShowChineseString(0, 6, label_human, 4);
  OLED_ShowString(64, 6, ":");
}

/* 以固定宽度输出 ASCII 字段：
 * 通过左对齐并用空格补满整段宽度，达到“直接覆盖旧内容”的效果，
 * 这样就不需要先清空该区域，能进一步减轻刷新闪烁。
 */
static void OLED_ShowField(uint8_t x, uint8_t page, uint8_t width, const char *text)
{
  char buf[24];
  if (width >= sizeof(buf)) width = sizeof(buf) - 1;
  snprintf(buf, sizeof(buf), "%-*s", width, text);
  OLED_ShowString(x, page, buf);
}

/* 仅刷新会变化的数值区：
 * 模式、照度、调光级数、人体状态都位于固定区域，
 * 每次先清除对应局部区域，再重绘当前值，避免整屏闪烁。
 */
static void OLED_UpdateDynamicUI(void)
{
  static const uint8_t *value_auto[] = {hz_zi, hz_dong};
  static const uint8_t *value_manual[] = {hz_shou, hz_dong};
  static const uint8_t *value_has[] = {hz_you};
  static const uint8_t *value_none[] = {hz_wu};
  static uint8_t last_auto_mode = 0xFF;
  static uint16_t last_ambient_lux = 0xFFFF;
  static uint16_t last_current_pwm = 0xFFFF;
  static uint8_t last_human_state = 0xFF;
  char field[16];
  uint8_t human_state = HCSR501_Read();

  /* 只有模式变化时才重绘“自动/手动”两个汉字。 */
  if (last_auto_mode != Auto_Mode)
  {
    OLED_ShowChineseString(48, 0, Auto_Mode ? value_auto : value_manual, 2);
    last_auto_mode = Auto_Mode;
  }

  /* 照度字段固定输出 3 位数字 + 空格 + lux，直接覆盖旧值。 */
  if (last_ambient_lux != Ambient_Lux)
  {
    snprintf(field, sizeof(field), "%3u lux", Ambient_Lux);
    OLED_ShowField(80, 2, 8, field);
    last_ambient_lux = Ambient_Lux;
  }

  /* 调光级数字段固定输出 3 位数字。 */
  if (last_current_pwm != Current_PWM)
  {
    snprintf(field, sizeof(field), "%3u", Current_PWM);
    OLED_ShowField(80, 4, 4, field);
    last_current_pwm = Current_PWM;
  }

  /* 人体状态只有在“有/无”切换时才重绘。 */
  if (last_human_state != human_state)
  {
    OLED_ShowChineseString(80, 6, human_state ? value_has : value_none, 1);
    last_human_state = human_state;
  }
}

int main(void)
{
  /* HAL 库初始化，包含 SysTick、NVIC 优先级组等基础环境配置。 */
  HAL_Init();
  SystemClock_Config();

  /* 由 CubeMX 生成的底层外设初始化：
   * GPIO：按键、编码器、PIR 等引脚
   * I2C1：BH1750 和 OLED 共用总线
   * TIM1：PA8 输出 PWM
   */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();

  /* 用户功能模块初始化。 */
  PWM_Init();
  KEY_Init();
  EC11_Init();
  HCSR501_Init();
  BH1750_Init();
  OLED_Init();
  Gamma_Init();
  OLED_DrawStaticUI();
  OLED_UpdateDynamicUI();

  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

  /* 软件定时调度标记：
   * t10  ：10ms 周期任务
   * t100 ：100ms 周期任务
   * t200 ：200ms 周期任务
   * t500 ：500ms 周期任务
   */
  uint32_t t10 = 0, t100 = 0, t200 = 0, t500 = 0;

  while (1)
  {
    uint32_t now = HAL_GetTick();

    /* 10ms 任务：
     * 1. 扫描按键
     * 2. 处理编码器旋转
     * 3. 按渐变方式更新实际亮度，避免瞬间跳变
     */
    if ((now - t10) >= 10)
    {
      t10 = now;

      /* 按键逻辑：
       * 单击：整灯开/关
       * 长按：自动/手动模式切换
       */
      KeyEvent_t k = KEY_Scan_10ms();
      if (k == KEY_EVENT_CLICK)
      {
        Power_State ^= 1;
        if (!Power_State)
        {
          Target_Brightness = 0;
        }
        else if (Target_Brightness == 0)
        {
          Target_Brightness = 100;
        }
      }
      else if (k == KEY_EVENT_LONG)
      {
        Auto_Mode ^= 1;
      }

      /* 编码器调光逻辑：
       * 只要用户旋转编码器，就默认进入手动模式；
       * step 为 +1/-1，分别表示亮度增加或减少一个等级。
       */
      int8_t step = EC11_Process_10ms();
      if (step != 0 && Power_State)
      {
        Auto_Mode = 0;
        int32_t tmp = (int32_t)Target_Brightness + step;
        if (tmp < 0) tmp = 0;
        if (tmp > 199) tmp = 199;
        Target_Brightness = (uint16_t)tmp;
      }

      if (Current_PWM < Target_Brightness) Current_PWM++;
      else if (Current_PWM > Target_Brightness) Current_PWM--;

      Clamp_Values();
      PWM_SetDutyCycle(Gamma_Table[Current_PWM]);
    }

    /* 100ms 任务：人体红外超时关灯逻辑。 */
    if ((now - t100) >= 100)
    {
      t100 = now;

      /* 检测到人体时：
       * 1. 清零无人计时
       * 2. 如果当前熄灯，则自动唤醒
       */
      if (HCSR501_Read())
      {
        PIR_Count = 0;
        if (!Power_State)
        {
          Power_State = 1;
          if (Target_Brightness == 0) Target_Brightness = 100;
        }
      }
      else
      {
        if (PIR_Count < PIR_Timeout) PIR_Count++;
        if (PIR_Count >= PIR_Timeout)
        {
          Power_State = 0;
          Target_Brightness = 0;
          PIR_Count = PIR_Timeout;
        }
      }
    }

    /* 200ms 任务：
     * 读取光照传感器值，并在自动模式下做简易闭环调光。
     */
    if ((now - t200) >= 200)
    {
      t200 = now;
      Ambient_Lux = BH1750_ReadLux_Filtered();

      if (Power_State && Auto_Mode)
      {
        if (Ambient_Lux < (TARGET_LUX - DEADBAND))
        {
          if (Target_Brightness < 199) Target_Brightness++;
        }
        else if (Ambient_Lux > (TARGET_LUX + DEADBAND))
        {
          if (Target_Brightness > 0) Target_Brightness--;
        }
      }
    }

    /* 500ms 任务：刷新 OLED 的动态数据区域。 */
    if ((now - t500) >= 500)
    {
      t500 = now;
      OLED_UpdateDynamicUI();
    }
  }
}

void SystemClock_Config(void)
{
  /* 系统时钟配置：
   * 使用外部高速晶振 HSE，经 PLL x9 倍频后得到 72MHz 主频。
   */
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  /* 一旦进入错误处理，说明关键外设初始化或运行过程出现不可恢复问题。
   * 这里关闭中断并停在死循环，方便调试定位。
   */
  __disable_irq();
  while (1)
  {
  }
}
