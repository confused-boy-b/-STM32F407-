#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include "main.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include "lwip.h"
#include "eth_custom_phy_interface.h" /* 使用标准 PHY 寄存器地址和位掩码。 */

#define TCP_SERVER_PORT 5000  /* TCP 服务器监听端口号。 */
/* 监听用 pcb 与当前客户端连接 pcb，定义见 freertos.c。 */
extern struct tcp_pcb *tcp_server_pcb;
extern struct tcp_pcb *tcp_client_pcb;

/* 把 data 拷入发送队列并立即推出去；pcb 为 NULL 时直接丢弃。 */
void TCP_Send(struct tcp_pcb *pcb, char *data, u32_t len);
/* 执行命令。 */
void Command_Execute(char *cmd, u16_t len);
/* 创建 TCP 服务器并监听指定端口。 */
void TCP_Server_Init(void *arg);

#endif
