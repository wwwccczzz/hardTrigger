#include "stm32f10x.h"

/* 设为 1 时输出 10 Hz 测试脉冲；单相机验证通过后改为 0，输出 125 Hz。 */
#define CAMERA_TRIGGER_TEST_10HZ 0U

#if CAMERA_TRIGGER_TEST_10HZ
/* 72 MHz / (719 + 1) / (9999 + 1) = 10 Hz。 */
#define CAMERA_TRIGGER_PSC 719U
#define CAMERA_TRIGGER_ARR 9999U
/* 定时器计数频率为 100 kHz，10 个计数对应 100 us 高电平。 */
#define CAMERA_TRIGGER_CCR 10U
#else
/* 72 MHz / (71 + 1) = 1 MHz 计数时钟，(7999 + 1) 个计数为 8 ms 周期，即 125 Hz。 */
#define CAMERA_TRIGGER_PSC 71U
#define CAMERA_TRIGGER_ARR 7999U
/* 定时器计数频率为 1 MHz，100 个计数对应 100 us 高电平。 */
#define CAMERA_TRIGGER_CCR 100U
/* 排查用其它频率（PSC 均为 71，计数时钟 1 MHz）：100 Hz -> ARR 9999，62.5 Hz -> ARR 15999。 */
#endif

/* 以下位定义只覆盖本测试使用的外设字段，避免引入与触发无关的驱动库。 */
#define CAMERA_RCC_APB2_IOPA_ENABLE (1UL << 2)
#define CAMERA_RCC_APB1_TIM2_ENABLE (1UL << 0)
#define CAMERA_GPIO_PA1_CONFIG_MASK (0xFUL << 4)
#define CAMERA_GPIO_PA1_AF_PP_50MHZ (0xBUL << 4)
#define CAMERA_TIM_OC2_PRELOAD      (1UL << 11)
#define CAMERA_TIM_OC2_PWM1         (6UL << 12)
#define CAMERA_TIM_CC2_ENABLE       (1UL << 4)
#define CAMERA_TIM_UPDATE_EVENT     (1UL << 0)
#define CAMERA_TIM_AUTO_RELOAD      (1UL << 7)
#define CAMERA_TIM_COUNTER_ENABLE   (1UL << 0)

/**
 * @brief 配置 TIM2_CH2，使 PA1 输出固定频率、100 us高电平的相机触发脉冲。
 * @note 创建时间：2026-09-14-15。PA1只允许连接NPN基极电阻，禁止接入12 V。
 */
static void camera_trigger_pwm_init(void)
{
	RCC->APB2ENR |= CAMERA_RCC_APB2_IOPA_ENABLE;
	RCC->APB1ENR |= CAMERA_RCC_APB1_TIM2_ENABLE;

	/* PA1设置为50 MHz复用推挽输出，对应TIM2_CH2默认映射。 */
	GPIOA->CRL &= ~CAMERA_GPIO_PA1_CONFIG_MASK;
	GPIOA->CRL |= CAMERA_GPIO_PA1_AF_PP_50MHZ;

	/* 先停止定时器，防止参数更新过程中产生不完整的触发脉冲。 */
	TIM2->CR1 = 0U;
	TIM2->CCER = 0U;
	TIM2->CNT = 0U;
	TIM2->PSC = CAMERA_TRIGGER_PSC;
	TIM2->ARR = CAMERA_TRIGGER_ARR;
	TIM2->CCR2 = CAMERA_TRIGGER_CCR;

	/* PWM1在CNT小于CCR2时输出高电平，因此每个周期起始输出100 us高脉冲。 */
	TIM2->CCMR1 = CAMERA_TIM_OC2_PRELOAD | CAMERA_TIM_OC2_PWM1;
	TIM2->CCER = CAMERA_TIM_CC2_ENABLE;
	TIM2->EGR = CAMERA_TIM_UPDATE_EVENT;
	TIM2->CR1 = CAMERA_TIM_AUTO_RELOAD | CAMERA_TIM_COUNTER_ENABLE;
}

/**
 * @brief 启动单相机硬触发测试并保持硬件定时器连续运行。
 * @note 创建时间：2026-09-14-15。相机需先进入Line0外触发等待状态，再复位STM32。
 */
int main(void)
{
	camera_trigger_pwm_init();

	while (1)
	{
		/* TIM2独立产生触发脉冲，主循环不参与计时，避免软件延时造成抖动。 */
	}
}
