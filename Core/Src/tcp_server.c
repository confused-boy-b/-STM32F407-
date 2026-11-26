
#include "tcp_server.h"

struct tcp_pcb *tcp_server_pcb = NULL;  /* 监听用的 pcb */
struct tcp_pcb *tcp_client_pcb = NULL;  /* 当前已连接的客户端 pcb */
/* 由 freertos.c 定义的拍照信号量。 */
extern osSemaphoreId_t photoSemHandle;
/* ethernetif.c 中的以太网句柄；只能在任务上下文回收已完成的 TX pbuf。 */
extern ETH_HandleTypeDef heth;

/* Keil Watch 中观察这些计数，可区分 ACK 未到与发送内存不足。 */
volatile uint32_t tcp_image_ack_bytes;
volatile uint32_t tcp_image_poll_count;
volatile uint32_t tcp_image_mem_failures;
volatile uint32_t tcp_image_output_failures;

/* 只允许一帧在发送队列中排队；这些状态只在 lwIP 核心线程中读写。 */
static struct
{
  /* 当前TCP连接的控制块*/
  struct tcp_pcb *pcb;
  /* 图像信息指针 */
  tcp_image_job_t *job;
  /* 图像数据偏移,表示已经发送出去多少数据 */
  uint32_t offset;
  /* 图像总长度，用4字节数组装 */
  uint8_t length_header[4];
  /*表示4字节包头发送出去的长度 */
  uint8_t header_offset;
} image_tx;
/*
 * 图像发送完成，释放信号量。
 */
static void TCP_ImageFinish(void)
{
  tcp_image_job_t *job = image_tx.job;

  image_tx.job = NULL;
  image_tx.pcb = NULL;
  image_tx.offset = 0U;
  image_tx.header_offset = 0U;

  /* 摄像头任务只有收到此通知，才能再次覆盖全局 JPEG 缓冲。 */
  if (job != NULL && job->queued_sem != NULL)
  {
    (void)osSemaphoreRelease(job->queued_sem);
  }
}
/*
 * 发送一帧图像的“泵”：尽可能把数据往 lwIP 发送队列里塞。
 *
 * 发送内容按顺序是：
 *   1) 4 字节大端序的图像长度（length_header），让客户端知道后面 JPEG 有多长；
 *   2) JPEG 原始数据（image_tx.job->data）。
 *
 * 一次调用不保证全部发完：
 *   - 发送缓冲不足（tcp_sndbuf==0 或 tcp_write 返回 ERR_MEM）时立即返回；
 *   - 之后由 tcp_sent_callback（对端 ACK 释放缓冲）或
 *     tcp_poll_callback（定时重试）再次调用本函数继续发送。
 * 因此本函数会被多次调用，靠 image_tx 里的 offset / header_offset 记录进度。
 */
static void TCP_ImagePump(struct tcp_pcb *pcb)
{
  /* 本轮是否至少成功写入过一段数据；有写入才需要调用 tcp_output。 */
  uint8_t queued_any = 0U;

  /* 没有待发送的帧，或者传入的 pcb 不是当前正在发送的连接，直接返回。 */
  if (image_tx.job == NULL || image_tx.pcb != pcb)
  {
    return;
  }

  /*
   * ETH 使用异步发送，会额外持有 TCP pbuf 的引用。驱动默认仅在发送描述符
   * 忙时回收它们；小容量 lwIP 堆可能因此在首批 1072 字节后耗尽。
   * 这里运行在 lwIP 核心线程中，回收 DMA 已发送完的包，然后再 tcp_write。
   * 尚由 DMA 持有的包不会被 HAL_ETH_ReleaseTxPacket() 释放。
   */
  (void)HAL_ETH_ReleaseTxPacket(&heth);

  /* 循环入队，直到缓冲满、写失败或全部写完。 */
  for (;;)
  {
    const uint8_t *data;               /* 本段要写入的数据起始地址。 */
    uint32_t remaining;                /* 从 data 开始还剩多少字节未入队。 */
    uint32_t count;                    /* 本次实际要写入的字节数。 */
    u16_t room = tcp_sndbuf(pcb);      /* 当前发送队列还能容纳的字节数。 */
    err_t result;                      /* tcp_write 的返回值。 */

    /* 发送队列已满：退出，等待 ACK 释放缓冲后再由回调继续。 */
    if (room == 0U)
    {
      break; /* 等待对端确认数据后释放发送缓冲。 */
    }

    /* 先发长度头，再发 JPEG 数据。 */
    /*判断数据长度是否发送完*/
      //sizeof(image_tx.length_header)是数据长度的总字节数
      //image_tx.header_offset正好对应已经把数据长度发出去了多少字节
    if (image_tx.header_offset < sizeof(image_tx.length_header))
    {
      /* 长度头还没发完，继续从 length_header 发剩余部分。 */
      data = image_tx.length_header + image_tx.header_offset;
      remaining = sizeof(image_tx.length_header) - image_tx.header_offset;
    }
    /*判断图片数据是否发送完*/
    else if (image_tx.offset < image_tx.job->len)
    {
      data = image_tx.job->data + image_tx.offset;
      remaining = image_tx.job->len - image_tx.offset;
    }
    else
    {
      /* 长度头和 JPEG 数据都已全部入队，没有可发的了。 */
      break;
    }

    /* 本次写入量不能超过：剩余数据、发送缓冲、单个 TCP 报文段三者中的最小值。 */
    count = remaining;
    if (count > room)
    {
      count = room;             /* 受发送缓冲限制。 */
    }
    if (count > tcp_mss(pcb))
    {
      count = tcp_mss(pcb);     /* 受最大报文段限制，避免切出过大的包。 */
    }
    if (count == 0U)
    {
      break;                    /* 没有可写的字节，退出。 */
    }

    /*
     * 把这段数据拷进 lwIP 发送队列。
     * TCP_WRITE_FLAG_COPY：立即复制数据，调用返回后源缓冲可被覆盖/复用。
     */
    result = tcp_write(pcb, data, (u16_t)count, TCP_WRITE_FLAG_COPY);
    if (result == ERR_MEM)
    {
      tcp_image_mem_failures++;
      /* pbuf 队列暂时分配不出空间，退出等 tcp_sent / tcp_poll 重试。 */
      break;
    }
    if (result != ERR_OK)
    {
      /* 其他错误：终止本次发送，并唤醒等待的摄像头任务，避免其永久阻塞。 */
      TCP_ImageFinish();
      return;
    }

    /* 标记本轮已写入数据，循环结束后要调用 tcp_output。 */
    queued_any = 1U;

    /* 推进进度：这段数据取自长度头还是 JPEG 数据，就推进对应的偏移。 */
    if (image_tx.header_offset < sizeof(image_tx.length_header))
    {
      /* 长度头还没发完，继续从 length_header 发剩余部分。 */
      image_tx.header_offset = (uint8_t)(image_tx.header_offset + count);
    }
    else
    {
      /* 长度头已发完，就偏移指针继续发图片数据。 */
      image_tx.offset += count;
    }
  }

  /*
   * 即使本轮没有新写入，也要重试 lwIP 中尚未发出的数据。
   * 例如前一次 tcp_output() 因底层暂时忙而失败，此时发送缓冲可能已满，
   * 不能只靠 queued_any 判断是否需要再次输出。
   */
  if (queued_any != 0U || pcb->unsent != NULL)
  {
    if(tcp_output(pcb) != ERR_OK)
    {
      tcp_image_output_failures++;
      HAL_UART_Transmit(&huart1, (uint8_t *)"tcp_output error\r\n", 18, 1000);
      return; /* 留在当前发送状态，下一次 tcp_poll 再尝试。 */
    }
  }

  /*
   * 长度头和 JPEG 数据都已成功进入发送队列。
   * 因为使用的是 COPY 模式，数据已被 lwIP 复制，不必等远端 ACK，
   * 现在就可以释放帧缓冲（唤醒摄像头任务拍下一帧）。
   */
  if (image_tx.header_offset == sizeof(image_tx.length_header) &&
      image_tx.offset == image_tx.job->len)
  {
    TCP_ImageFinish();
  }
}

static err_t tcp_sent_callback(void *arg, struct tcp_pcb *pcb, u16_t len)
{
  (void)arg;
  tcp_image_ack_bytes += len;
  TCP_ImagePump(pcb); /* ACK 释放发送空间后继续排队。 */
  return ERR_OK;
}

static err_t tcp_poll_callback(void *arg, struct tcp_pcb *pcb)
{
  (void)arg;
  tcp_image_poll_count++;
  TCP_ImagePump(pcb); /* 没有新 ACK 时也重试 ERR_MEM。 */
  return ERR_OK;
}

void TCP_ImageStart(void *arg)
{
  tcp_image_job_t *job = (tcp_image_job_t *)arg;
  uint32_t len;

  if (job == NULL)
  {
    return;
  }

  /* 回调被执行时客户端可能已经断开，也可能已有一帧正在发送。 */
  if (tcp_client_pcb == NULL || image_tx.job != NULL ||
      job->data == NULL || job->len == 0U)
  {
    //释放信号量
    if (job->queued_sem != NULL)
    {
      (void)osSemaphoreRelease(job->queued_sem);
    }
    return;
  }

  image_tx.job = job;
  image_tx.pcb = tcp_client_pcb;
  image_tx.offset = 0U;
  image_tx.header_offset = 0U;
  tcp_image_ack_bytes = 0U;
  tcp_image_poll_count = 0U;
  tcp_image_mem_failures = 0U;
  tcp_image_output_failures = 0U;
  len = job->len;
  image_tx.length_header[0] = (uint8_t)(len >> 24);
  image_tx.length_header[1] = (uint8_t)(len >> 16);
  image_tx.length_header[2] = (uint8_t)(len >> 8);
  image_tx.length_header[3] = (uint8_t)len;
  //发送图像数据
  TCP_ImagePump(image_tx.pcb);
}
/*
TCP发送函数
*/
void TCP_Send(struct tcp_pcb *pcb, char *data, u32_t len)
{
  /* 此辅助函数只适合短消息；图片由 TCP_ImagePump 分段发送。 */
  if (pcb == NULL || data == NULL || len == 0U ||
      len > 0xFFFFU || len > tcp_sndbuf(pcb))
  {
    return;
  }
  if (tcp_write(pcb, data, (u16_t)len, TCP_WRITE_FLAG_COPY) == ERR_OK)
  {
    (void)tcp_output(pcb);
  }
}
/*
命令执行函数
  Input : cmd – 命令字符串
          len – 字符串长度
*/
void Command_Execute(char *cmd, u16_t len)
{
  (void)cmd;
  /* 保持当前协议：收到任何非空数据，就请求拍摄一帧。 */
  if (len != 0U && photoSemHandle != NULL)
  {
    /* 二值信号量会合并拍照过程中重复到来的请求。 */
    (void)osSemaphoreRelease(photoSemHandle);
  }
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
  (void)arg;
  if (p == NULL)
  {
    /* 对端关闭连接；正在等待发送的摄像头任务也要被唤醒。 */
    if (image_tx.pcb == tpcb)
    {
      TCP_ImageFinish();
    }
    tcp_client_pcb = NULL;
    if (tcp_close(tpcb) != ERR_OK)
    {
      tcp_abort(tpcb);
      return ERR_ABRT;
    }
    return ERR_OK;
  }

  if (err != ERR_OK)
  {
    pbuf_free(p);
    return err;
  }

  /* 当前命令没有参数，使用整个 pbuf 链的有效长度判断非空。 */
  Command_Execute((char *)p->payload, p->tot_len);
  tcp_recved(tpcb, p->tot_len); /* 已处理这些字节，归还 TCP 接收窗口。 */
  pbuf_free(p);
  return ERR_OK;
}
/*
TCP 错误回调函数
*/
static void tcp_err_callback(void *arg, err_t err)
{
  (void)arg;
  (void)err;
  /* 此回调执行时 PCB 已被 lwIP 释放，不再访问它。 */
  TCP_ImageFinish();
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
  (void)arg;
  if (err != ERR_OK || newpcb == NULL)
  {
    return ERR_VAL;
  }

  /* 只有一个全局帧缓冲和发送状态，因此一次只服务一个客户端。 */
  if (tcp_client_pcb != NULL)
  {
    tcp_abort(newpcb);
    return ERR_ABRT;
  }

  /* 这个连接，发数据用 */
  tcp_client_pcb = newpcb;

  /* 注册收数据回调、错误回调，并绑定参数 */
  tcp_arg(newpcb, NULL);               /* 想传自定义数据就放这里 */
  tcp_recv(newpcb, tcp_recv_callback); /* 收到数据时调用 */
  tcp_err(newpcb, tcp_err_callback);   /* 连接断开时调用 */
  tcp_sent(newpcb, tcp_sent_callback); /* ACK 后继续发送图片。 */
  tcp_poll(newpcb, tcp_poll_callback, 2); /* 队列满时定期重试。 */

  return ERR_OK;  /* 接受该连接 */
}

/*
TCP初始化函数
*/
void TCP_Server_Init(void *arg)
{
  (void)arg;
   /* 1. 新建 pcb */
  tcp_server_pcb = tcp_new();
  if (tcp_server_pcb == NULL)
    return;  /* 内存不足，创建失败 */

  /* 2. 绑定端口，IP_ADDR_ANY 表示不限定来源 IP */
  if (tcp_bind(tcp_server_pcb, IP_ADDR_ANY, TCP_SERVER_PORT) != ERR_OK)
  {
    (void)tcp_close(tcp_server_pcb);
    tcp_server_pcb = NULL;
    return;
  }

  /* 3. 进入监听状态。
     注意：tcp_listen 会返回一个新 pcb，必须接住，旧的已经失效 */
  tcp_server_pcb = tcp_listen(tcp_server_pcb);
  if (tcp_server_pcb == NULL)
  {
    return;
  }

  /* 4. 注册“有客户端来连接”的回调 */
  tcp_accept(tcp_server_pcb, tcp_accept_callback);
}
