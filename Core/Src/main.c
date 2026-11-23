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
#include "lwip/tcp.h"                 /* 提供 tcp_new/tcp_bind/tcp_listen/tcp_write/tcp_output 声明。 */
#include <string.h>                  /* 使用 strlen 计算串口发送长度。 */
#include "OV2640.h"                        /* 使用摄像头驱动函数。 */
#include "OV2640Run.h"                     /* 使用摄像头采集函数。 */
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
struct tcp_pcb *tcp_server_pcb = NULL;  /* 监听用的 pcb */
struct tcp_pcb *tcp_client_pcb = NULL;  /* 当前已连接的客户端 pcb */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* 读取 PHY 链路模式：-1 表示读取失败，0 表示链路未就绪，1 表示已得到速率和双工。 */
static int32_t App_PHY_GetLinkMode(uint32_t *speed, uint32_t *duplex);
/* 根据 PHY 状态更新 MAC 和 LwIP 网卡的物理链路状态。 */
static void App_Ethernet_Link_Check(struct netif *netif);

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
TCP发送函数
*/
void TCP_Send(struct tcp_pcb *pcb, char *data, u32_t len)
{
  if (pcb == NULL)
    return;  /* 还没客户端连接，没地方发，直接丢弃 */

  /* flags 用 COPY 表示把数据拷进 LwIP 发送队列，
     调用返回后你的 data 缓冲区可以立即复用/被覆盖 */
  tcp_write(pcb, data, len, TCP_WRITE_FLAG_COPY);

  /* 真正把队列里的数据推出去 */
  tcp_output(pcb);
}
/*
命令执行函数
  Input : cmd – 命令字符串
          len – 字符串长度
*/
static void Command_Execute(char *cmd, u16_t len)
{
  if (len == 0)
    return; /* 空命令直接返回 */
  
  uint8_t *p = NULL;
  uint32_t send_len = 0;
  TCP_Send(tcp_client_pcb, cmd, len);
  // /* 本函数只采集一帧；p 指向全局帧缓冲，下次调用会被覆盖，需立即发送。 */
  // jpeg_test(&p, &send_len);
  // /* 这里可以根据 cmd 内容执行不同的操作，这里只是简单回显。 */
  // if (p != NULL && send_len != 0)
  // {
  //   TCP_Send(tcp_client_pcb, (char *)p, send_len);
  // }
}
/*
数据回调
  Input : arg – 注册时传入的自定义参数
          tpcb – 新建的连接 PCB
          p – 收到的数据包
          err – 错误码
*/
static err_t tcp_recv_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  if (p == NULL)
  {
    /* p==NULL 表示对端关闭了连接 */
    tcp_client_pcb = NULL;
    tcp_close(tpcb);
    return ERR_OK;
  }

  if (err == ERR_OK)
  {
    //执行命令
    Command_Execute(p->payload, p->len);
    /* 用完必须释放 pbuf，否则每次收包都泄漏内存，跑一会儿就死 */
    pbuf_free(p);
  }

  return ERR_OK;
}
/*
TCP 错误回调函数
*/
static void tcp_err_callback(void *arg, err_t err)
{
  /* 连接断开了，清空指针，避免后面串口还往无效 pcb 发数据 */
  tcp_client_pcb = NULL;
}
/*
  TCP 连接建立回调函数
  Input : arg – 注册时传入的自定义参数
          newpcb – 新建的连接 PCB
          err – 错误码
*/
static err_t tcp_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  if (err != ERR_OK || newpcb == NULL)
    return ERR_ABRT;   /* 连接建立失败，直接拒绝 */

  /* 记住这个连接，供串口发数据用 */
  tcp_client_pcb = newpcb;

  /* 注册收数据回调、错误回调，并绑定参数 */
  tcp_arg(newpcb, NULL);               /* 想传自定义数据就放这里 */
  tcp_recv(newpcb, tcp_recv_callback); /* 收到数据时调用 */
  tcp_err(newpcb, tcp_err_callback);   /* 连接断开时调用 */

  return ERR_OK;  /* 接受该连接 */
}
/*
TCP初始化函数
*/
void TCP_Server_Init(u16_t port)
{
  /* 1. 新建 pcb */
  tcp_server_pcb = tcp_new();
  if (tcp_server_pcb == NULL)
    return;  /* 内存不足，创建失败 */

  /* 2. 绑定端口，IP_ADDR_ANY 表示不限定来源 IP */
  tcp_bind(tcp_server_pcb, IP_ADDR_ANY, port);

  /* 3. 进入监听状态。
     注意：tcp_listen 会返回一个新 pcb，必须接住，旧的已经失效 */
  tcp_server_pcb = tcp_listen(tcp_server_pcb);

  /* 4. 注册“有客户端来连接”的回调 */
  tcp_accept(tcp_server_pcb, tcp_accept_callback);
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
  delay_init();
  /* 初始化后立即检查一次链路，以便已插网线时及时启动 MAC。 */
  App_Ethernet_Link_Check(netif_default);
  ov2640_init();
  TCP_Server_Init(5000);
  HAL_UART_Transmit(&huart1, (uint8_t *)"TCP Server is running on port 5000\r\n", 36, HAL_MAX_DELAY);
  uint32_t  last_link_check= HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    MX_LWIP_Process();
    if ((uint32_t)(HAL_GetTick() - last_link_check) >= APP_LINK_CHECK_INTERVAL_MS)
    {
        last_link_check = HAL_GetTick();
        App_Ethernet_Link_Check(netif_default);
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    
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
