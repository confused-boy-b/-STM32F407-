/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "dcmi.h"
#include "dma.h"
#include "lwip.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "eth_custom_phy_interface.h" /* 使用标准 PHY 寄存器地址和位掩码。 */
#include "lwip/udp.h"                 /* 提供 udp_new/udp_bind/udp_recv/udp_sendto 声明。 */
#include <string.h>                  /* 使用 strlen 计算串口发送长度。 */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define APP_PHY_ADDRESS             0U    /* YT8512C 在 MDIO 总线上的地址。 */
#define APP_LINK_CHECK_INTERVAL_MS  100U  /* 两次链路检查之间的最短间隔，单位为毫秒。 */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
//接收长度
#define LEN 1024
//接收缓冲区
uint8_t UART1_RxBuffer[LEN];
struct udp_pcb * udp;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* 读取 PHY 链路模式：-1 表示读取失败，0 表示链路未就绪，1 表示已得到速率和双工。 */
static int32_t App_PHY_GetLinkMode(uint32_t *speed, uint32_t *duplex);
/* 根据 PHY 状态更新 MAC 和 LwIP 网卡的物理链路状态。 */
static void App_Ethernet_Link_Check(struct netif *netif);
/* 通过指定 UDP 控制块向已连接的对端发送字符串。 */
void UDP_Sent(struct udp_pcb *pcb, char *send_p);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static int32_t App_PHY_GetLinkMode(uint32_t *speed, uint32_t *duplex)
{
  uint32_t bsr;     /* PHY 基本状态寄存器：包含链路和协商完成标志。 */
  uint32_t bcr;     /* PHY 基本控制寄存器：包含自动协商使能位。 */
  uint32_t anar;    /* 本端向对端宣告的速率和双工能力。 */
  uint32_t anlpar;  /* 对端向本端宣告的速率和双工能力。 */
  uint32_t common;  /* 本端和对端共同支持的能力。 */

  /* 链路状态位可能锁存为断开，第一次清除旧状态，第二次取得当前状态。 */
  if (HAL_ETH_ReadPHYRegister(&heth, APP_PHY_ADDRESS, USER_PHY_BSR, &bsr) != HAL_OK ||
      HAL_ETH_ReadPHYRegister(&heth, APP_PHY_ADDRESS, USER_PHY_BSR, &bsr) != HAL_OK)
  {
    return -1; /* MDIO 读取失败，保持当前链路状态，等待下次重试。 */
  }

  /* 总线无响应时可能读到全 1，不能将其当作有效的链路状态。 */
  if (bsr == 0xFFFFU)
  {
    return -1;
  }

  /* 链路位为 0，说明网线未连接或对端链路尚未建立。 */
  if ((bsr & USER_PHY_BSR_LINK_STATUS) == 0U)
  {
    return 0;
  }

  /* 读取控制寄存器，以区分自动协商模式和手动强制模式。 */
  if (HAL_ETH_ReadPHYRegister(&heth, APP_PHY_ADDRESS, USER_PHY_BCR, &bcr) != HAL_OK)
  {
    return -1;
  }

  if ((bcr & USER_PHY_BCR_AUTONEGO_EN) == 0U)
  {
    /* 关闭自动协商时，控制寄存器中的速率位直接决定 10M 或 100M。 */
    *speed = ((bcr & USER_PHY_BCR_SPEED_SELECT) != 0U) ? ETH_SPEED_100M : ETH_SPEED_10M;
    /* 双工位决定全双工或半双工，并通过输出参数交给调用者。 */
    *duplex = ((bcr & USER_PHY_BCR_DUPLEX_MODE) != 0U) ? ETH_FULLDUPLEX_MODE : ETH_HALFDUPLEX_MODE;
    return 1; /* 已得到强制模式下的速率和双工。 */
  }

  /* 自动协商仍在进行时，暂不启动 MAC，避免速率或双工配置错误。 */
  if ((bsr & USER_PHY_BSR_AUTONEGO_CPLT) == 0U)
  {
    return 0;
  }

  /* 分别读取本端与对端在自动协商中宣告的能力。 */
  if (HAL_ETH_ReadPHYRegister(&heth, APP_PHY_ADDRESS, USER_PHY_ANAR, &anar) != HAL_OK ||
      HAL_ETH_ReadPHYRegister(&heth, APP_PHY_ADDRESS, USER_PHY_ANLPAR, &anlpar) != HAL_OK)
  {
    return -1;
  }

  common = anar & anlpar; /* 只有双方共同支持的模式才能作为协商结果。 */
  /* 按优先级依次判断：100M 全双工、100M 半双工、10M 全双工、10M 半双工。 */
  if ((common & USER_PHY_ANAR_100BASE_TX_FD) != 0U)
  {
    *speed = ETH_SPEED_100M;      /* 选择 100M。 */
    *duplex = ETH_FULLDUPLEX_MODE; /* 选择全双工。 */
  }
  else if ((common & USER_PHY_ANAR_100BASE_TX) != 0U)
  {
    *speed = ETH_SPEED_100M;      /* 选择 100M。 */
    *duplex = ETH_HALFDUPLEX_MODE; /* 选择半双工。 */
  }
  else if ((common & USER_PHY_ANAR_10BASE_T_FD) != 0U)
  {
    *speed = ETH_SPEED_10M;       /* 选择 10M。 */
    *duplex = ETH_FULLDUPLEX_MODE; /* 选择全双工。 */
  }
  else if ((common & USER_PHY_ANAR_10BASE_T) != 0U)
  {
    *speed = ETH_SPEED_10M;       /* 选择 10M。 */
    *duplex = ETH_HALFDUPLEX_MODE; /* 选择半双工。 */
  }
  else
  {
    return 0; /* 没有找到共同支持的模式，暂不认为链路可用。 */
  }

  return 1; /* 速率和双工已通过输出参数返回。 */
}

static void App_Ethernet_Link_Check(struct netif *netif)
{
  ETH_MACConfigTypeDef mac_config; /* 保存当前 MAC 配置，以便只修改速率和双工。 */
  uint32_t speed;                  /* PHY 当前协商出的速率。 */
  uint32_t duplex;                 /* PHY 当前协商出的双工模式。 */
  int32_t link_mode;               /* PHY 检查结果：读取失败、未就绪或已连通。 */

  /* 网卡尚未注册成功时，不访问网卡状态或以太网硬件。 */
  if (netif == NULL)
  {
    return;
  }

  link_mode = App_PHY_GetLinkMode(&speed, &duplex); /* 查询 PHY 链路及协商结果。 */
  if (link_mode < 0)
  {
    /* MDIO 读取失败时不凭一次错误改变当前链路状态。 */
    return;
  }

  /* 未连线或尚未协商完成时，通知 LwIP 并停止 MAC。 */
  if (link_mode == 0)
  {
    if (netif_is_link_up(netif))
    {
      netif_set_link_down(netif); /* 只在状态发生变化时通知 LwIP。 */
    }
    if (heth.gState == HAL_ETH_STATE_STARTED)
    {
      (void)HAL_ETH_Stop(&heth); /* 停止 MAC 的收发和 DMA。 */
    }
    return;
  }

  /* 读取 MAC 原有设置，保留校验和等其他配置。 */
  if (HAL_ETH_GetMACConfig(&heth, &mac_config) != HAL_OK)
  {
    return;
  }

  /* 如果 PHY 重新协商出不同模式，运行中的 MAC 必须先停止。 */
  if (heth.gState == HAL_ETH_STATE_STARTED &&
      (mac_config.Speed != speed || mac_config.DuplexMode != duplex))
  {
    if (netif_is_link_up(netif))
    {
      netif_set_link_down(netif); /* 暂停向协议栈报告链路可用。 */
    }
    if (HAL_ETH_Stop(&heth) != HAL_OK)
    {
      return; /* 停止失败时不能继续修改运行中的 MAC。 */
    }
  }

  /* HAL 只允许在就绪状态修改 MAC 配置，然后再启动收发。 */
  if (heth.gState == HAL_ETH_STATE_READY)
  {
    mac_config.Speed = speed;       /* 将 MAC 速率设为 PHY 协商结果。 */
    mac_config.DuplexMode = duplex; /* 将 MAC 双工模式设为 PHY 协商结果。 */
    if (HAL_ETH_SetMACConfig(&heth, &mac_config) != HAL_OK ||
        HAL_ETH_Start(&heth) != HAL_OK)
    {
      return; /* 配置或启动失败时，不向 LwIP 报告链路接通。 */
    }
  }

  /* MAC 已可收发时，才把物理链路已连接的状态通知 LwIP。 */
  if (heth.gState == HAL_ETH_STATE_STARTED && !netif_is_link_up(netif))
  {
    netif_set_link_up(netif);
  }
}
/*
  UDP接收回调函数
*/
void udp_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  udp_connect(pcb, addr, port);
  UDP_Sent(pcb,"ok");
  pbuf_free(p);
}
/*
DUP固定IP和端口发送函数
*/
void UDP_Sent(struct udp_pcb *pcb,char *send_p)
{
  u16_t len=strlen(send_p);
  struct pbuf * p=pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
  pbuf_take(p, send_p, len);
  udp_send(pcb, p);
  pbuf_free(p);
}
/*
  UDP初始化函数
*/
void UDP_inti(u16_t port)
{
  udp=udp_new();
  udp_bind(udp,IP_ADDR_ANY,port);
  udp_recv(udp,udp_receive_callback,NULL);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_LWIP_Init();
  MX_USART1_UART_Init();
  MX_DCMI_Init();
  /* USER CODE BEGIN 2 */
  /* 初始化后立即检查一次链路，以便已插网线时及时启动 MAC。 */
  App_Ethernet_Link_Check(netif_default);
  //UDP初始化并输入UDP绑定端口
  UDP_inti(5000);
  //串口不定长接收
  HAL_UARTEx_ReceiveToIdle_IT(&huart1, (uint8_t *)UART1_RxBuffer, LEN);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    MX_LWIP_Process();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    static uint32_t last_link_check_tick = 0U; /* 保存上一次链路检查的系统时间。 */
    uint32_t now = HAL_GetTick();             /* 读取当前系统时间，单位为毫秒。 */
    /* 利用无符号减法处理计数器回绕，并将检查频率限制在每 100 毫秒一次。 */
    if ((now - last_link_check_tick) >= APP_LINK_CHECK_INTERVAL_MS)
    {
      last_link_check_tick = now;             /* 更新检查时间。 */
      App_Ethernet_Link_Check(netif_default); /* 同步 PHY、MAC 和 LwIP 的链路状态。 */
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
//不定长接受回调
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if(huart->Instance == USART1)
  {
    UART1_RxBuffer[Size]='\0';
    UDP_Sent(udp,UART1_RxBuffer);
    //重新开启不定长中断接收
    HAL_UARTEx_ReceiveToIdle_IT(&huart1, (uint8_t *)UART1_RxBuffer, LEN);
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
