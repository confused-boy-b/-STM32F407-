#include "OV2640.h"
#include "OV2640cfg.h"
#include "sccb.h"
#include "delay.h"
#include "usart.h"
//读寄存器数据
uint8_t ov2640_read_reg(uint16_t reg)
{
    uint8_t data = 0;

    sccb_start();                       /* 启动SCCB传输 */
    sccb_send_byte(OV2640_ADDR);        /* 写器件ADDR */
    delay_us(100);
    sccb_send_byte(reg);                /* 写寄存器地址 */
    delay_us(100);
    sccb_stop();
    delay_us(100);

    /* 设置寄存器地址后，才是读 */
    sccb_start();
    sccb_send_byte(OV2640_ADDR | 0X01); /* 发送读命令 */
    delay_us(100);
    data = sccb_read_byte();            /* 读取数据 */
    sccb_nack();
    sccb_stop();

    return data;
}
//写寄存器数据
uint8_t ov2640_write_reg(uint16_t reg, uint8_t data)
{
    uint8_t res = 0;

    sccb_start();                       /* 启动SCCB传输 */
    delay_us(100);

    if (sccb_send_byte(OV2640_ADDR))    /* 写器件ID */
    {
        res = 1;
    }

    delay_us(100);

    if (sccb_send_byte(reg))            /* 写寄存器地址 */
    {
        res = 1;
    }

    delay_us(100);

    if (sccb_send_byte(data))           /* 写数据 */
    {
        res = 1;
    }

    delay_us(100);
    sccb_stop();
    
    return res;
}

uint8_t ov2640_init(void)
{
    uint16_t i = 0;
    uint16_t reg;

    OV2640_PWDN(0);     /* POWER ON */
    delay_ms(10);
    OV2640_RST(0);      /* 必须先拉低OV2640的RST脚,再上电 */
    delay_ms(20);
    OV2640_RST(1);      /* 结束复位 */
    delay_ms(20);
    
    ov2640_write_reg(OV2640_DSP_RA_DLMT, 0x01);     /* 操作sensor寄存器 */
    ov2640_write_reg(OV2640_SENSOR_COM7, 0x80);     /* 软复位OV2640 */
    delay_ms(50);
    
    reg = ov2640_read_reg(OV2640_SENSOR_MIDH);      /* 读取厂家ID 高八位 */
    reg <<= 8;
    reg |= ov2640_read_reg(OV2640_SENSOR_MIDL);     /* 读取厂家ID 低八位 */

    if (reg != OV2640_MID)      /* ID 是否正常 */
    {
        /* 串口提示厂家 ID 校验失败，发送长度不包含字符串结束符。 */
        OV2640_DEBUG_PRINT("OV2640 MID error\r\n", sizeof("OV2640 MID error\r\n") - 1U);
        return 1;               /* 失败 */
    }
    
    reg = ov2640_read_reg(OV2640_SENSOR_PIDH);  /* 读取器件ID 高八位 */
    reg <<= 8;
    reg |= ov2640_read_reg(OV2640_SENSOR_PIDL); /* 读取器件ID 低八位 */

    if (reg != OV2640_PID)      /* ID是否正常 */
    {
        /* 串口提示器件 ID 校验失败，发送长度不包含字符串结束符。 */
        OV2640_DEBUG_PRINT("OV2640 PID error\r\n", sizeof("OV2640 PID error\r\n") - 1U);
        return 1;               /* 失败 */
    }
    
    /* 初始化 OV2640 */
    for (i = 0; i < sizeof(ov2640_uxga_init_reg_tbl) / 2; i++)
    {
        ov2640_write_reg(ov2640_uxga_init_reg_tbl[i][0], ov2640_uxga_init_reg_tbl[i][1]);
    }

    return 0;   /* OV2640初始化完成 */
}

/**
 * @brief       OV2640 切换为RGB565模式
 * @param       无
 * @retval      无
 */
void ov2640_rgb565_mode(void)
{
    uint16_t i = 0;

    /* 设置:RGB565输出 */
    for (i = 0; i < (sizeof(ov2640_rgb565_reg_tbl) / 2); i++)
    {
        ov2640_write_reg(ov2640_rgb565_reg_tbl[i][0], ov2640_rgb565_reg_tbl[i][1]); /* 发送配置数组 */
    }
}

/**
 * @brief       OV2640 切换为JPEG模式
 * @param       无
 * @retval      无
 */
void ov2640_jpeg_mode(void)
{
    uint16_t i = 0;

    /* 先配置内部 YUV422 图像格式（主要是颜色格式等），供后续 JPEG 编码使用。 */
    for (i = 0; i < (sizeof(ov2640_yuv422_reg_tbl) / 2); i++)
    {
        ov2640_write_reg(ov2640_yuv422_reg_tbl[i][0], ov2640_yuv422_reg_tbl[i][1]); /* 发送配置数组 */
    }

    /* 再配置 JPEG 输出；DCMI 最终接收的是压缩后的 JPEG 数据。 */
    for (i = 0; i < (sizeof(ov2640_jpeg_reg_tbl) / 2); i++)
    {
        if(ov2640_write_reg(ov2640_jpeg_reg_tbl[i][0], ov2640_jpeg_reg_tbl[i][1]))     /* 发送配置数组 */
        {
            /* JPEG 寄存器写入失败时提示 SCCB 配置异常。 */
            OV2640_DEBUG_PRINT("OV2640 write reg error: jpeg_set\r\n",
                               sizeof("OV2640 write reg error: jpeg_set\r\n") - 1U);
        }
    }
}

/** 
 * @bref        设置图像输出大小
 * @@param      width : 宽度(对应:horizontal)
 * @param       height: 高度(对应:vertical)
 * @note        OV2640输出图像的大小(分辨率),完全由该函数确定
 *              width和height必须是4的倍数
 * @retval      0     : 设置成功
 *              其他  : 设置失败
 */
uint8_t ov2640_outsize_set(uint16_t width, uint16_t height)
{
    uint16_t outh;
    uint16_t outw;
    uint8_t temp;
    uint8_t sum=0;

    if (width % 4) return 1;
    if (height % 4) return 2;

    outw = width / 4;
    outh = height/ 4;
    sum+=ov2640_write_reg(0xFF, 0x00);    
    sum+=ov2640_write_reg(0xE0, 0x04);
    sum+=ov2640_write_reg(0x5A, outw & 0xFF);    /* 设置OUTW的低八位 */
    sum+=ov2640_write_reg(0x5B, outh & 0xFF);    /* 设置OUTH的低八位 */

    temp = (outw >> 8) & 0x03;
    temp |= (outh >> 6) & 0x04;
    sum+=ov2640_write_reg(0x5C, temp);           /* 设置OUTH/OUTW的高位 */
    sum+=ov2640_write_reg(0xE0, 0x00);
    if(sum)
    {
        return sum;
    }
    return 0;
}
