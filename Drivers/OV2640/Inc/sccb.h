#ifndef __SCCB_H
#define __SCCB_H
#include "gpio.h"
#include "main.h"
//使用do while方便调用这个宏，宏展开后在if else中，可以直接使用;
#define SCCB_SCL(x)     do{x?\
                            HAL_GPIO_WritePin(SCCB_SCL_GPIO_Port, SCCB_SCL_Pin, GPIO_PIN_SET):\
                            HAL_GPIO_WritePin(SCCB_SCL_GPIO_Port, SCCB_SCL_Pin, GPIO_PIN_RESET);\
                        }while(0)

#define SCCB_SDA(x)     do{x?\
                            HAL_GPIO_WritePin(SCCB_SDA_GPIO_Port, SCCB_SDA_Pin, GPIO_PIN_SET):\
                            HAL_GPIO_WritePin(SCCB_SDA_GPIO_Port, SCCB_SDA_Pin, GPIO_PIN_RESET);\
                        }while(0)

//读取SDA
#define SCCB_READ_SDA   HAL_GPIO_ReadPin(SCCB_SDA_GPIO_Port, SCCB_SDA_Pin)

/**
 * @brief       产生SCCB起始信号
 * @param       无
 * @retval      无
 */
void sccb_start(void);

/**
 * @brief       产生SCCB停止信号
 * @param       无
 * @retval      无
 */
void sccb_stop(void);

/**
 * @brief       不产生ACK应答
 * @param       无
 * @retval      无
 */
void sccb_nack(void);

/**
 * @brief       SCCB 发送一个字节
 * @param       data: 要发送的数据
 * @retval      无
 */
uint8_t sccb_send_byte(uint8_t data);

/**
 * @brief       SCCB 读取一个字节
 * @param       无
 * @retval      读取到的数据
 */
uint8_t sccb_read_byte(void);
#endif