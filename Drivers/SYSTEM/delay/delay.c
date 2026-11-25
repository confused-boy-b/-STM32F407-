#include "stm32f4xx_hal.h"
#include "delay.h"
static uint32_t g_fac_us = 0;  /* us延时倍乘数 */
void delay_init(void)
{
    //用于选择SYSTICK时钟源，让SYSTICK直接使用HCLK（AHB总线时钟）
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);/* SYSTICK使用内核时钟源,同CPU同频率 */
    //SystemCoreClock获取内核时钟频率，/1000000得到1us的节拍数
    g_fac_us = SystemCoreClock/1000000;     
}

void delay_us(uint32_t nus)
{
    uint32_t ticks;
    uint32_t told, tnow, tcnt = 0;
    //LOAD是定时器的重装载值寄存器
    uint32_t reload = SysTick->LOAD;  /* LOAD的值 */
    ticks = nus * g_fac_us;           /* 延时需要的节拍数 */
    told = SysTick->VAL;              /* 刚进入时的计数器值 */
    while (1)
    {
        //当前时钟计数器值
        tnow = SysTick->VAL;
        if (tnow != told)
        {   //还没有溢出
            if (tnow < told)
            {
                //用刚开始的计数器值减去现在的计数器值，得到这段时间内的节拍数
                tcnt += told - tnow;  /* 这里注意一下SYSTICK是一个递减的计数器就可以了 */
            }
            //已经溢出
            else 
            {
                //已经溢出用重装载值-当前值就是溢出的节拍数，再加上计数开始时的节拍数就是总节拍数
                tcnt += reload - tnow + told;
            }
            //更新开始计数节拍数，这样tcnt不会重复累加
            told = tnow;
            //如果总节拍数超过要延迟的节拍数，则延时完毕
            if (tcnt >= ticks)
            {
                break;
            }
        }
    }
}

void delay_ms(uint16_t nms)
{
    
    delay_us(nms * 1000);
    
}