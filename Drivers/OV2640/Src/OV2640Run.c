#include "OV2640Run.h"
#include "dcmi.h"                       /* 提供 hdcmi、hdma_dcmi 声明。 */
#include "usart.h"                      /* 提供 huart1 声明。 */
#include <stdio.h>                      /* 提供 sprintf 声明。 */
#include <string.h>                     /* 提供 strlen 声明。 */

/* 帧缓冲按 uint32 计数：8*1024 字 = 32KB，足够容纳 224x168 JPEG 一帧。 */
#define jepg_data_size 8*1024
#define jpeg_line_size 1*1024
__ALIGNED(4) uint32_t g_jpeg_data_buf[jepg_data_size];  /*JEPG数据缓存,一帧数据*/
/* buf中的JPEG有效数据长度 */
volatile uint32_t g_jpeg_data_len = 0;
/* JPEG数据采集完成标志
 * 0,数据没有采集完;
 * 1,数据采集完了,但是还没处理;
 * 2,数据已经处理完成了,可以开始下一帧接收
 */
volatile uint8_t g_jpeg_data_ok = 0;

__ALIGNED(4) uint32_t g_dcmi_line_buf[2][jpeg_line_size];/*使用双缓存，一个用于采集，一个用于处理，每个都是一行的大小*/
//一帧结束时停止DMA，把不足一个缓冲区的尾部数据补进帧数组
void jpeg_data_process(void)
{
  //已传输数据长度
  uint32_t rlen;
  //字指针，指向传输数据的末尾
  uint32_t *pbuf;
  uint16_t i;
  /* 记录关闭 DMA 前正在使用的缓冲区，用于判断关闭DMA时恰好完成的整块传输。 */
  uint32_t ct_before_stop;

  //数据还没采集完，进行收尾处理
  if(g_jpeg_data_ok == 0)
  {
    /* 在关闭DMA之前，检查最后一个缓冲区是否完成。 */
    if (__HAL_DMA_GET_FLAG(&hdma_dcmi, DMA_FLAG_TCIF1_5) != RESET)
    {
        HAL_DMA_IRQHandler(&hdma_dcmi);
    }
    /* 记下关闭前的目标缓冲区；完整传输结束时 CT 才会切换。 */
    ct_before_stop = hdma_dcmi.Instance->CR & DMA_SxCR_CT;
    //关闭DMA
    __HAL_DMA_DISABLE(&hdma_dcmi);
    /* 等待 DMA 真正停止，避免在复制尾部数据时缓冲区继续变化。 */
    while ((hdma_dcmi.Instance->CR & DMA_SxCR_EN) != 0U) { }
    /* 如果关闭过程中判断CT是否切换，切换了说明恰好有一整块传输完成。 */
    if ((hdma_dcmi.Instance->CR & DMA_SxCR_CT) != ct_before_stop)
    {
        HAL_DMA_IRQHandler(&hdma_dcmi);
    }
    else
    {
        /* 停止 DMA 本身也会产生 TCIF；它不代表装满一块，只清标志。 */
        __HAL_DMA_CLEAR_FLAG(&hdma_dcmi, DMA_FLAG_TCIF1_5);
    }
    //获得已传输完成的数据长度，__HAL_DMA_GET_COUNTER在这里返回的是字数
    rlen= jpeg_line_size - __HAL_DMA_GET_COUNTER(&hdma_dcmi);
    //指针指向g_jpeg_data_buf中有效数据末尾
    pbuf = g_jpeg_data_buf + g_jpeg_data_len;
    /* 防止最后一段数据超出整帧数组。 */
    if (rlen > jepg_data_size - g_jpeg_data_len)
    {
      Error_Handler();
    }
    //判断 DMA2_Stream1 的 CT 位，为 1 时正在写 buf1，否则正在写 buf0。
    if (DMA2_Stream1->CR & (1 << 19))
    {
        for (i = 0; i < rlen; i++)
        {
            pbuf[i] = g_dcmi_line_buf[1][i];    /* 读取buf1里面的剩余数据 */
        }
    }
    else 
    {
        for (i = 0; i < rlen; i++)
        {
            pbuf[i] = g_dcmi_line_buf[0][i];    /* 读取buf0里面的剩余数据 */
        }
    }
    g_jpeg_data_len+=rlen;
#if OV2640_DEBUG
    char str[50];
    sprintf((char*)str, "CR:%d,rlen:%d\r\n", (DMA2_Stream1->CR & (1 << 19)),rlen);
    OV2640_DEBUG_PRINT((uint8_t*)str, strlen(str));
#endif
    /* 数据采集完成，等待其他函数处理 */
    g_jpeg_data_ok = 1;
  }
  if (g_jpeg_data_ok == 2)        /* 上一次的jpeg数据已经被处理了 */
  {
      __HAL_DMA_SET_COUNTER(&hdma_dcmi, jpeg_line_size);   /* 设置DMA本次需要传输的大小，传输长度为jpeg_buf_size*4字节 */
      __HAL_DMA_ENABLE(&hdma_dcmi);   /* 重新传输 */
      g_jpeg_data_ok = 0;         /* 标记数据未采集 */
      g_jpeg_data_len = 0;        /* 数据重新开始 */
  }
}
//把双缓冲区的数据拷贝到g_jpeg_data_buf（帧数组）中
void jpeg_dcmi_rx_callback(void)
{
  /* 每次复制一个 DMA 缓冲区前检查整帧数组容量。 */
  if (g_jpeg_data_len > jepg_data_size - jpeg_line_size)
  {
    Error_Handler();
  }
  //字指针，指向传输数据末尾
  volatile uint32_t *pbuf;
  //初始化字指针，完成偏移，指向g_jpeg_data_buf中有效数据末尾
  pbuf = g_jpeg_data_buf + g_jpeg_data_len;
  uint16_t i;
  //第19位为1，表示正在传输buf1，先处理buf0，否则正在传输buf0
  if (hdma_dcmi.Instance->CR & (1 << 19))
  {
    for(i=0;i<jpeg_line_size;i++)
    {
      pbuf[i]=g_dcmi_line_buf[0][i];    /* 读取buf0里面的数据 */
    }
    g_jpeg_data_len+=jpeg_line_size;//有效数据长度增加
  }
  else
  {
    for(i=0;i<jpeg_line_size;i++)
    {
      pbuf[i]=g_dcmi_line_buf[1][i];    /* 读取buf1里面的数据 */
    }
    g_jpeg_data_len+=jpeg_line_size;//有效数据长度增加
  }

}
/*
  DAM，完成中断回调函数，DMA 的任一缓冲区装满后，把已完成的缓冲区追加到整帧数组。
*/
static void jpeg_dma_complete(DMA_HandleTypeDef *hdma)
{
  (void)hdma;
  /* DMA 的任一缓冲区装满后，把已完成的缓冲区追加到整帧数组。 */
  jpeg_dcmi_rx_callback();
  OV2640_DEBUG_PRINT("jpeg_dma_complete\r\n", 20);
}

static void jpeg_dma_error(DMA_HandleTypeDef *hdma)
{
  (void)hdma;
  /* DMA 出错时停止采集，避免把不完整的帧作为 JPEG 发送。 */
  DCMI->CR &= ~DCMI_CR_CAPTURE;
  OV2640_DEBUG_PRINT("jpeg_dma_error\r\n", 17);
  Error_Handler();
}
//配置摄像头输出 JPEG，启动 DCMI 和 DMA，等待一帧采集完成
//然后找到完整 JPEG 数据，发送给电脑。
void jpeg_test(uint8_t **send_ptr, uint32_t *len)
{
  //字节指针，对一帧数据要进行字节级遍历处理
  uint8_t* p;
  //文件大小,图片开始位置,i
  uint32_t jpglen=0,jpgstart=0,i;
  //是否找到文件头标记
  uint8_t headok = 0;
  //配置摄像头
  ov2640_jpeg_mode();                                 /* JPEG模式 */
  if(ov2640_outsize_set(224, 168))    /* 设置分辨率 */
  {
    OV2640_DEBUG_PRINT("ov2640_outsize_set error\r\n", 26);
  }

  
  /* 两个 DMA 缓冲区共用完成回调：每装满一块就保存到 JPEG 帧数组。 */
  hdma_dcmi.XferCpltCallback = jpeg_dma_complete;
  hdma_dcmi.XferM1CpltCallback = jpeg_dma_complete;
  /* DMA 错误时停止采集，不发送损坏的数据。 */
  hdma_dcmi.XferErrorCallback = jpeg_dma_error;
  /* 新的一帧从空缓冲区和“未完成”状态开始。 */
  g_jpeg_data_len = 0;
  g_jpeg_data_ok = 0;
  /* 启动带完成中断的双缓冲 DMA，接收 DCMI 数据寄存器。 */
  if (HAL_DMAEx_MultiBufferStart_IT(&hdma_dcmi, (uint32_t)&DCMI->DR,
                                    (uint32_t)g_dcmi_line_buf[0],
                                    (uint32_t)g_dcmi_line_buf[1], jpeg_line_size) != HAL_OK)
  {
    /* DMA 启动失败时停止程序，防止在无采集缓冲区时开启 DCMI。 */
    Error_Handler();
  }
  /* 使能 DCMI、帧结束中断以及实际的图像采集。 */
  __HAL_DCMI_ENABLE(&hdcmi);
  /* 清除上一帧留下的 FRAME_RIS，避免刚使能中断就误报新的一帧结束。 */
  __HAL_DCMI_CLEAR_FLAG(&hdcmi, DCMI_FLAG_FRAMERI);
  __HAL_DCMI_ENABLE_IT(&hdcmi, DCMI_IT_FRAME);
  DCMI->CR |= DCMI_CR_CAPTURE;
  

  //等待一帧采集完成
  while(1)
  {
    if (g_jpeg_data_ok == 1)
    {
      p=(uint8_t*)g_jpeg_data_buf;
      //HAL_UART_Transmit(&huart1, p, (uint16_t)jepg_data_size, HAL_MAX_DELAY);
      jpglen=0;
      headok=0;
      //g_jpeg_data_len为有效数据长度
      for (i = 0; i + 1U < g_jpeg_data_len * 4U; i++)
      {
        if (!headok && (p[i] == 0xFF) && (p[i + 1] == 0xD8))   /* 找到FF D8 */
        {
            jpgstart = i;
            headok = 1;     /* 标记找到jpg头(FF D8) */
        }
        
        if ((p[i] == 0xFF) && (p[i+1] == 0xD9) && headok)   /* 找到头以后,再找FF D9 */
        {
            //这里+2是加上开头的0xFF和结尾的0XD9
            jpglen = i - jpgstart + 2;
            break;
        }
      }
#if OV2640_DEBUG
      char str[50];
      sprintf((char*)str, "jpglen:%d,jpgstart:%d\r\n", jpglen,jpgstart);
      OV2640_DEBUG_PRINT((uint8_t*)str, strlen(str));
#endif
      //如果文件大小不为0，则说明找到完整的JPEG数据，可以发送了
      if (jpglen||headok)
      {
        p += jpgstart;  /* 偏移到0XFF,0XD8处 */
        
        /* 从 JPEG 文件头开始发送，不能使用查找到文件尾时的 p[i]。 */
        // if (HAL_UART_Transmit(&huart1, p, (uint16_t)jpglen, HAL_MAX_DELAY) != HAL_OK)
        // {
        //   Error_Handler();
        // }
        *len = jpglen;
        *send_ptr = p;
      }

      /* 停止本帧 DMA；main() 随后重新启动下一帧。 */
      if (HAL_DMA_Abort(&hdma_dcmi) != HAL_OK)
      {
        Error_Handler();
      }
      g_jpeg_data_ok = 2;
      return;  /* main() 再次调用 jpeg_test()，重新启动下一帧 */
    }
  }
}

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi)
{
  (void)hdcmi;
  /* 一帧结束时停止采集，并复制 DMA 缓冲区中剩余的数据。 */
  //停止捕获，DCMI不再产生新的 DMA 请求
  DCMI->CR &= ~DCMI_CR_CAPTURE;
  jpeg_data_process();
  OV2640_DEBUG_PRINT("Frame Event Callback\r\n", 22);
}
