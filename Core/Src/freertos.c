/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tcp_server.h"
#include "usart.h"
#include "OV2640Run.h"
#include "delay.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define APP_PHY_ADDRESS             0U    /* YT8512C 在 MDIO 总线上的地址。 */
#define APP_LINK_CHECK_INTERVAL_MS  100U  /* 两次链路检查之间的最短间隔（毫秒）。 */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/* 保存任务句柄。创建成功后，FreeRTOS 会通过它标识这个任务。 */
static osThreadId_t TcpTaskHandle;

/* 计数器：用调试器观察它是否持续增加，就能确认任务在运行。 */
static volatile uint32_t TcpCounter = 0;

/* 任务属性：名称、栈大小和优先级。 */
static const osThreadAttr_t TcpTask_attributes = {
    .name = "TcpTask",              /* 调试时显示的任务名称。 */
    .stack_size = 2048,             /* 摄像头调用链需要比计数示例更多的栈。 */
    .priority = osPriorityNormal,   /* 普通优先级。 */
};

/* TCP 收到拍照请求时释放；摄像头任务等待。 */
osSemaphoreId_t photoSemHandle;
/* 一帧全部进入 lwIP 的复制队列后释放，防止摄像头提前覆盖全局帧缓冲。 */
static osSemaphoreId_t frameQueuedSem;
/* tcpip_callback 异步使用此对象，摄像头任务等待完成前不能改写它。 */
static tcp_image_job_t image_job;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/* 声明任务入口函数；FreeRTOS 启动任务时会调用它。 */
static void TcpAppTask(void *argument);
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

extern void MX_LWIP_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* 两个初始为空的二值信号量：分别表示拍照请求和帧缓冲可复用。 */
  photoSemHandle = osSemaphoreNew(1, 0, NULL);
  frameQueuedSem = osSemaphoreNew(1, 0, NULL);

  if (photoSemHandle == NULL || frameQueuedSem == NULL)
  {
    Error_Handler();  /* 创建失败，例如 FreeRTOS 堆空间不足。 */
  }
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* 创建任务：
  * 第一个参数是任务入口函数；
  * 第二个参数是传给入口函数的参数，这里不需要，所以传 NULL；
  * 第三个参数是上面定义的任务属性。
  *
  * 这里仅完成“创建”。任务会在 main.c 调用 osKernelStart() 后运行。
  */
  TcpTaskHandle = osThreadNew(TcpAppTask, NULL, &TcpTask_attributes);

  /* 返回 NULL 表示创建失败，例如 FreeRTOS 堆空间不足。 */
  if (TcpTaskHandle == NULL) {
      Error_Handler();
  }
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN StartDefaultTask */
  delay_init();
  ov2640_init();
  tcpip_callback(TCP_Server_Init, NULL); /* 安排在 lwIP 核心线程启动 TCP 服务 */
  HAL_UART_Transmit(&huart1, (uint8_t *)"TCP Server is running on port 5000\r\n", 36, HAL_MAX_DELAY);
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
static void TcpAppTask(void *argument)
{
    err_t post_result;
    (void)argument;
    for (;;)
    {
        uint8_t *frame = NULL;
        uint32_t frame_len = 0U;

        /* 等待 TCP 回调发出的拍照请求，不在 lwIP 核心线程中等待 DCMI。 */
        if (osSemaphoreAcquire(photoSemHandle, osWaitForever) != osOK)
        {
            continue;
        }

        jpeg_test(&frame, &frame_len);
        if (frame == NULL || frame_len == 0U)
        {
            continue; /* 没有得到完整 JPEG，本次不发送。 */
        }

        image_job.data = frame;
        image_job.len = frame_len;
        image_job.queued_sem = frameQueuedSem;

        /* 只投递发送任务；TCP PCB 始终由 lwIP 核心线程访问。 */
        do
        {
            post_result = tcpip_callback(TCP_ImageStart, &image_job);
            if (post_result == ERR_MEM)
            {
                osDelay(10); /* 核心线程邮箱暂满，稍后重试而不覆盖帧。 */
            }
        } while (post_result == ERR_MEM);

        if (post_result == ERR_OK)
        {
            /* 再获取信号量，全部字节已经入队，可以复用帧缓冲*/
            (void)osSemaphoreAcquire(frameQueuedSem, osWaitForever);
            TcpCounter++;
        }
    }
}
/* USER CODE END Application */

