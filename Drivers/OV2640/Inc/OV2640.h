#ifndef __OV2640_H__
#define __OV2640_H__
#include "main.h"
#include "gpio.h"
/* IO控制函数 */
#define OV2640_PWDN(x)      do{ x ? \
                                HAL_GPIO_WritePin(OV_PWDN_GPIO_Port, OV_PWDN_Pin, GPIO_PIN_SET) : \
                                HAL_GPIO_WritePin(OV_PWDN_GPIO_Port, OV_PWDN_Pin, GPIO_PIN_RESET); \
                            }while(0)       /* POWER DOWN控制信号 */

#define OV2640_RST(x)       do{ x ? \
                                HAL_GPIO_WritePin(OV_ESET_GPIO_Port, OV_ESET_Pin, GPIO_PIN_SET) : \
                                HAL_GPIO_WritePin(OV_ESET_GPIO_Port, OV_ESET_Pin, GPIO_PIN_RESET); \
                            }while(0)       /* 复位控制信号 */
                    
#define Image_FlipVer       1               /* 图像垂直翻转使能 1;使能 0:失能,(使用ATK-OV2640模组版本必须使能) */

/* 调试打印开关：1 打开（需本工程已配置 USART2 并定义 huart2），0 关闭。
 * 关闭后 huart2 相关代码不参与编译，避免依赖未配置的外设。 */
#define OV2640_DEBUG        1

#if OV2640_DEBUG
#define OV2640_DEBUG_PRINT(buf, len)  HAL_UART_Transmit(&huart1, (const uint8_t *)(buf), (len), 1000)
#else
#define OV2640_DEBUG_PRINT(buf, len)  ((void)0)
#endif


//读寄存器数据
uint8_t ov2640_read_reg(uint16_t reg);
//写寄存器数据
uint8_t ov2640_write_reg(uint16_t reg, uint8_t data);
//初始化OV2640
uint8_t ov2640_init(void);

/**
 * @brief       OV2640 切换为RGB565模式
 * @param       无
 * @retval      无
 */
void ov2640_rgb565_mode(void);

/**
 * @brief       OV2640 切换为JPEG模式
 * @param       无
 * @retval      无
 */
void ov2640_jpeg_mode(void);

/** 
 * @bref        设置图像输出大小
 * @@param      width : 宽度(对应:horizontal)
 * @param       height: 高度(对应:vertical)
 * @note        OV2640输出图像的大小(分辨率),完全由该函数确定
 *              width和height必须是4的倍数
 * @retval      0     : 设置成功
 *              其他  : 设置失败
 */
uint8_t ov2640_outsize_set(uint16_t width, uint16_t height);

#endif