
#include "tcp_server.h"

struct tcp_pcb *tcp_server_pcb = NULL;  /* 监听用的 pcb */
struct tcp_pcb *tcp_client_pcb = NULL;  /* 当前已连接的客户端 pcb */

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
void Command_Execute(char *cmd, u16_t len)
{
  if (len == 0)
    return; /* 空命令直接返回 */
  
  uint8_t *p = NULL;
  uint32_t send_len = 0;
  TCP_Send(tcp_client_pcb, cmd, len);
  // /* 本函数只采集一帧；p 指向全局帧缓冲，下次调用会被覆盖，需立即发送。 */
  // jpeg_test(&p, &send_len);
  // /* 这里可以根据 cmd 内容执行不同的操作，这里发送图片。 */
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

  /* 这个连接，发数据用 */
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
void TCP_Server_Init(void *arg)
{
  (void)arg;
   /* 1. 新建 pcb */
  tcp_server_pcb = tcp_new();
  if (tcp_server_pcb == NULL)
    return;  /* 内存不足，创建失败 */

  /* 2. 绑定端口，IP_ADDR_ANY 表示不限定来源 IP */
  tcp_bind(tcp_server_pcb, IP_ADDR_ANY, TCP_SERVER_PORT);

  /* 3. 进入监听状态。
     注意：tcp_listen 会返回一个新 pcb，必须接住，旧的已经失效 */
  tcp_server_pcb = tcp_listen(tcp_server_pcb);

  /* 4. 注册“有客户端来连接”的回调 */
  tcp_accept(tcp_server_pcb, tcp_accept_callback);
}