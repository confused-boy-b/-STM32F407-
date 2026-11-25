/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */
#include "main.h"
/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, ETH_RESET_Pin|SCCB_SCL_Pin|SCCB_SDA_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, OV_PWDN_Pin|OV_ESET_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : ETH_RESET_Pin SCCB_SDA_Pin */
  GPIO_InitStruct.Pin = ETH_RESET_Pin|SCCB_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : SCCB_SCL_Pin */
  GPIO_InitStruct.Pin = SCCB_SCL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(SCCB_SCL_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : OV_PWDN_Pin OV_ESET_Pin */
  GPIO_InitStruct.Pin = OV_PWDN_Pin|OV_ESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

}

/* USER CODE BEGIN 2 */
 static void Eth_Reset(void)
 {
    HAL_GPIO_WritePin(ETH_RESET_GPIO_Port, ETH_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(ETH_RESET_GPIO_Port, ETH_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(5);
 }
void HAL_ETH_MspInit(ETH_HandleTypeDef *heth)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};  // 定义GPIO初始化结构体变量，并全部清零
  __HAL_RCC_GPIOA_CLK_ENABLE();  // 使能GPIOA时钟
  __HAL_RCC_GPIOC_CLK_ENABLE();  // 使能GPIOC时钟
  __HAL_RCC_GPIOG_CLK_ENABLE();  // 使能GPIOG时钟
  __HAL_RCC_ETH_CLK_ENABLE();               // 使能以太网MAC外设时钟

  /* ========== PA1 PA2 PA7 配置为 AF11 ETH ========== */
  GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;  // 选择PA1 PA2 PA7
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;    // 复用功能，推挽输出
  GPIO_InitStruct.Pull = GPIO_NOPULL;        // 无内部上下拉（ETH硬件一般外部处理）
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH; // 高速，ETH需要
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH; // 把所选引脚的复用功能设为以太网外设 AF11 ETH
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);    // 配置GPIOA对应引脚

  /* ========== PC1 PC4 PC5 配置为 AF11 ETH ========== */
  GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;  // 选择PC1 PC4 PC5
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);    // 配置GPIOC对应引脚

  /* ========== PG11 PG13 PG14 配置为 AF11 ETH ========== */
  GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_13 | GPIO_PIN_14; // PG11 PG13 PG14
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);    // 配置GPIOG对应引脚

  HAL_NVIC_SetPriority(ETH_IRQn, 5, 0);  // 设置以太网中断优先级为5
  HAL_NVIC_EnableIRQ(ETH_IRQn);          // 使能以太网中断

  Eth_Reset();  // 复位以太网PHY芯片
}

/* USER CODE END 2 */
