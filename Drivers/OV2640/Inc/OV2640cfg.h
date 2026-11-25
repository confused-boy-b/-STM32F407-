#ifndef __OV2640CFG_H
#define __OV2640CFG_H
/* OV2640的ID和访问地址 */
#define OV2640_MID              0x7FA2      /* 厂家ID */
#define OV2640_PID              0x2642      /* 版本ID */
#define OV2640_ADDR             0x60        /* OV2640的SCCB地址 */

/* 当选择DSP地址(0xFF=0x00)时,OV2640的DSP寄存器地址映射表 */
#define OV2640_DSP_R_BYPASS     0x05
#define OV2640_DSP_Qs           0x44
#define OV2640_DSP_CTRL         0x50
#define OV2640_DSP_HSIZE1       0x51
#define OV2640_DSP_VSIZE1       0x52
#define OV2640_DSP_XOFFL        0x53
#define OV2640_DSP_YOFFL        0x54
#define OV2640_DSP_VHYX         0x55
#define OV2640_DSP_DPRP         0x56
#define OV2640_DSP_TEST         0x57
#define OV2640_DSP_ZMOW         0x5A
#define OV2640_DSP_ZMOH         0x5B
#define OV2640_DSP_ZMHH         0x5C
#define OV2640_DSP_BPADDR       0x7C
#define OV2640_DSP_BPDATA       0x7D
#define OV2640_DSP_CTRL2        0x86
#define OV2640_DSP_CTRL3        0x87
#define OV2640_DSP_SIZEL        0x8C
#define OV2640_DSP_HSIZE2       0xC0
#define OV2640_DSP_VSIZE2       0xC1
#define OV2640_DSP_CTRL0        0xC2
#define OV2640_DSP_CTRL1        0xC3
#define OV2640_DSP_R_DVP_SP     0xD3
#define OV2640_DSP_IMAGE_MODE   0xDA
#define OV2640_DSP_RESET        0xE0
#define OV2640_DSP_MS_SP        0xF0
#define OV2640_DSP_SS_ID        0x7F
#define OV2640_DSP_SS_CTRL      0xF8
#define OV2640_DSP_MC_BIST      0xF9
#define OV2640_DSP_MC_AL        0xFA
#define OV2640_DSP_MC_AH        0xFB
#define OV2640_DSP_MC_D         0xFC
#define OV2640_DSP_P_STATUS     0xFE
#define OV2640_DSP_RA_DLMT      0xFF 

/* 当选择传感器地址(0xFF=0x01)时,OV2640的Sensor寄存器地址映射表 */
#define OV2640_SENSOR_GAIN       0x00
#define OV2640_SENSOR_COM1       0x03
#define OV2640_SENSOR_REG04      0x04
#define OV2640_SENSOR_REG08      0x08
#define OV2640_SENSOR_COM2       0x09
#define OV2640_SENSOR_PIDH       0x0A
#define OV2640_SENSOR_PIDL       0x0B
#define OV2640_SENSOR_COM3       0x0C
#define OV2640_SENSOR_COM4       0x0D
#define OV2640_SENSOR_AEC        0x10
#define OV2640_SENSOR_CLKRC      0x11
#define OV2640_SENSOR_COM7       0x12
#define OV2640_SENSOR_COM8       0x13
#define OV2640_SENSOR_COM9       0x14
#define OV2640_SENSOR_COM10      0x15
#define OV2640_SENSOR_HREFST     0x17
#define OV2640_SENSOR_HREFEND    0x18
#define OV2640_SENSOR_VSTART     0x19
#define OV2640_SENSOR_VEND       0x1A
#define OV2640_SENSOR_MIDH       0x1C
#define OV2640_SENSOR_MIDL       0x1D
#define OV2640_SENSOR_AEW        0x24
#define OV2640_SENSOR_AEB        0x25
#define OV2640_SENSOR_W          0x26
#define OV2640_SENSOR_REG2A      0x2A
#define OV2640_SENSOR_FRARL      0x2B
#define OV2640_SENSOR_ADDVSL     0x2D
#define OV2640_SENSOR_ADDVHS     0x2E
#define OV2640_SENSOR_YAVG       0x2F
#define OV2640_SENSOR_REG32      0x32
#define OV2640_SENSOR_ARCOM2     0x34
#define OV2640_SENSOR_REG45      0x45
#define OV2640_SENSOR_FLL        0x46
#define OV2640_SENSOR_FLH        0x47
#define OV2640_SENSOR_COM19      0x48
#define OV2640_SENSOR_ZOOMS      0x49
#define OV2640_SENSOR_COM22      0x4B
#define OV2640_SENSOR_COM25      0x4E
#define OV2640_SENSOR_BD50       0x4F
#define OV2640_SENSOR_BD60       0x50
#define OV2640_SENSOR_REG5D      0x5D
#define OV2640_SENSOR_REG5E      0x5E
#define OV2640_SENSOR_REG5F      0x5F
#define OV2640_SENSOR_REG60      0x60
#define OV2640_SENSOR_HISTO_LOW  0x61
#define OV2640_SENSOR_HISTO_HIGH 0x62

const uint8_t ov2640_yuv422_reg_tbl[][2]= 
{
    0xFF, 0x00, 
    0xDA, 0x10,
    0xD7, 0x03,
    0xDF, 0x00,
    0x33, 0x80,
    0x3C, 0x40,
    0xe1, 0x77,
    0x00, 0x00,
};
const uint8_t ov2640_jpeg_reg_tbl[][2]=
{
    0xff, 0x00, 
    0xe0, 0x14,
    0xe1, 0x77,
    0xe5, 0x1f,
    0xd7, 0x03,
    0xda, 0x10,
    0xe0, 0x00, 
};

/* UXGA初始化寄存器序列表
 * 此模式下帧率为15帧
 * UXGA(1600*1200)
 */
 //数组用存放地址+存放数据为一行
const uint8_t ov2640_uxga_init_reg_tbl[][2]= 
{   
    0xff, 0x00,
    0x2c, 0xff,     /* 保留 */
    0x2e, 0xdf,     /* 保留 */
    
    0xff, 0x01,
    0x3c, 0x32,     /* 保留 */
    0x11, 0x00,     /* CLKRC 时钟频率控制 默认 */
    0x09, 0x02,     /* COM2 输出驱动选择 x2 */
 #if (Image_FlipVer) /* 水平镜像,垂直翻转 */
    0x04, 0xA8,     /* REG04 设置水平镜像 */
 #else
    0x04, 0xD8,
 #endif
    0x13, 0xe5,     /* COM8 开启带过滤器选择，最小曝光时间1/120秒 曝光自动控制 */
    0x14, 0x48,     /* COM9 设置AGC:32x 默认为64x */
    0x2c, 0x0c,     /* 保留 */
    0x33, 0x78,     /* 保留 */
    0x3a, 0x33,     /* 保留 */
    0x3b, 0xfB,     /* 保留 */
    
    0x3e, 0x00,     /* 保留 */
    0x43, 0x11,     /* 保留 */
    0x16, 0x10,     /* 保留 */
    
    0x39, 0x92,     /* 保留 */
    
    0x35, 0xda,     /* 保留 */
    0x22, 0x1a,     /* 保留 */
    0x37, 0xc3,     /* 保留 */
    0x23, 0x00,     /* 保留 */
    0x34, 0xc0,     /* ARCOM2 设置缩放窗口水平起点 默认0 */
    0x36, 0x1a,     /* 保留 */
    0x06, 0x88,     /* 保留 */
    0x07, 0xc0,     /* 保留 */
    0x0d, 0x87,     /* 保留 */
    0x0e, 0x41,     /* 保留 */
    0x4c, 0x00,     /* 保留 */

    0x48, 0x00,     /* COM19 设置缩放模式垂直窗口起点(低2位 默认0) */
    0x5B, 0x00,     /* 保留 */
    0x42, 0x03,     /* 保留 */
    
    0x4a, 0x81,     /* 保留 */
    0x21, 0x99,     /* 保留 */
    
    0x24, 0x40,     /* 亮度信号高范围AEC/AGC操作 当平均亮度大于AEW时，自动模式下AEC/AGC值会下降该寄存器值(默认78) */
    0x25, 0x38,     /* 亮度信号低范围AEC/AGC操作 当平均亮度小于AEW时，自动模式下AEC/AGC值会上升该寄存器值(默认68) */
    0x26, 0x82,     /* 快速模式大步长范围阈值，高阈值和低阈值 */
    0x5c, 0x00,     /* 保留 */
    0x63, 0x00,     /* 保留 */
    0x46, 0x00,     /* 帧长度调整 */
    0x0c, 0x3c,     /* COM3 手动设置带50Hz */
    
    0x61, 0x70,     /* 直方图算法底层 */
    0x62, 0x80,     /* 直方图算法高层 */
    0x7c, 0x05,     /* 保留 */
    
    0x20, 0x80,     /* 保留 */
    0x28, 0x30,     /* 保留 */
    0x6c, 0x00,     /* 保留 */
    0x6d, 0x80,     /* 保留 */
    0x6e, 0x00,     /* 保留 */
    0x70, 0x02,     /* 保留 */
    0x71, 0x94,     /* 保留 */
    0x73, 0xc1,     /* 保留 */
    0x3d, 0x34,     /* 保留 */
    0x5a, 0x57,     /* 保留 */
    
    0x12, 0x00,     /* COM7 分辨率选择、软复位、缩放模式、色条 UXGA 1600*1200 */

    0x17, 0x11,     /* 水平窗口起点 高8位 */
    0x18, 0x75,     /* 水平窗口终点 高8位 */
    0x19, 0x01,     /* 垂直窗口起点 高8位 */
    0x1a, 0x97,     /* 垂直窗口终点 高8位 */
    0x32, 0x36,     /* REG32 设置像素时钟分频 水平窗口的低3位设置 */
    0x03, 0x0f,     /* COM1 设置空帧控制 垂直窗口的低3位设置 */
    0x37, 0x40,     /* 保留 */
    
    0x4f, 0xca,     /* 50Hz Banding AEC 高8位 默认 */
    0x50, 0xa8,     /* 60Hz Banding AEC 高8位 默认 */
    0x5a, 0x23,     /* 保留 */
    0x6d, 0x00,     /* 保留 */
    0x6d, 0x38,     /* 保留 */
    
    0xff, 0x00,
    0xe5, 0x7f,     /* 保留 */
    0xf9, 0xc0,     /* 单片机复位 */
    0x41, 0x24,     /* 保留 */
    0xe0, 0x14,     /* 复位（单片机/SCCB/JPEG/DVP/IPU/CIF） */
    0x76, 0xff,     /* 保留 */
    0x33, 0xa0,     /* 保留 */
    0x42, 0x20,     /* 保留 */
    0x43, 0x18,     /* 保留 */
    0x4c, 0x00,     /* 保留 */
    0x87, 0xd5,     /* CTRL3 模块使能BPC/WPC */
    0x88, 0x3f,     /* 保留 */
    0xd7, 0x03,     /* 保留 */
    0xd9, 0x10,     /* 保留 */
    0xd3, 0x82,     /* R_DVP_SP 自动模式/DVP PCLK输出速度控制 */
    
    0xc8, 0x08,     /* 保留 */
    0xc9, 0x80,     /* 保留 */
    
    0x7c, 0x00,     /* SED相关 */
    0x7d, 0x00,     /* SED相关 */
    0x7c, 0x03,     /* SED相关 */
    0x7d, 0x48,     /* SED相关 */
    0x7d, 0x48,     /* SED相关 */
    0x7c, 0x08,     /* SED相关 */
    0x7d, 0x20,     /* SED相关 */
    0x7d, 0x10,     /* SED相关 */
    0x7d, 0x0e,     /* SED相关 */
    
    0x90, 0x00,     /* 保留 */
    0x91, 0x0e,     /* 保留 */
    0x91, 0x1a,     /* 保留 */
    0x91, 0x31,     /* 保留 */
    0x91, 0x5a,     /* 保留 */
    0x91, 0x69,     /* 保留 */
    0x91, 0x75,     /* 保留 */
    0x91, 0x7e,     /* 保留 */
    0x91, 0x88,     /* 保留 */
    0x91, 0x8f,     /* 保留 */
    0x91, 0x96,     /* 保留 */
    0x91, 0xa3,     /* 保留 */
    0x91, 0xaf,     /* 保留 */
    0x91, 0xc4,     /* 保留 */
    0x91, 0xd7,     /* 保留 */
    0x91, 0xe8,     /* 保留 */
    0x91, 0x20,     /* 保留 */
    
    0x92, 0x00,     /* 保留 */
    0x93, 0x06,     /* 保留 */
    0x93, 0xe3,     /* 保留 */
    0x93, 0x05,     /* 保留 */
    0x93, 0x05,     /* 保留 */
    0x93, 0x00,     /* 保留 */
    0x93, 0x04,     /* 保留 */
    0x93, 0x00,     /* 保留 */
    0x93, 0x00,     /* 保留 */
    0x93, 0x00,     /* 保留 */
    0x93, 0x00,     /* 保留 */
    0x93, 0x00,     /* 保留 */
    0x93, 0x00,     /* 保留 */
    0x93, 0x00,     /* 保留 */
    
    0x96, 0x00,     /* 保留 */
    0x97, 0x08,     /* 保留 */
    0x97, 0x19,     /* 保留 */
    0x97, 0x02,     /* 保留 */
    0x97, 0x0c,     /* 保留 */
    0x97, 0x24,     /* 保留 */
    0x97, 0x30,     /* 保留 */
    0x97, 0x28,     /* 保留 */
    0x97, 0x26,     /* 保留 */
    0x97, 0x02,     /* 保留 */
    0x97, 0x98,     /* 保留 */
    0x97, 0x80,     /* 保留 */
    0x97, 0x00,     /* 保留 */
    0x97, 0x00,     /* 保留 */
    
    0xc3, 0xef,     /* CTRL1 模块使能 CIP/DMY/RAW_GMA/DG/AWB/AWB_GAIN/LENC/PRE */
    
    0xa4, 0x00,     /* 保留 */
    0xa8, 0x00,     /* 保留 */
    0xc5, 0x11,     /* 保留 */
    0xc6, 0x51,     /* 保留 */
    0xbf, 0x80,     /* 保留 */
    0xc7, 0x10,     /* 保留 */
    0xb6, 0x66,     /* 保留 */
    0xb8, 0xA5,     /* 保留 */
    0xb7, 0x64,     /* 保留 */
    0xb9, 0x7C,     /* 保留 */
    0xb3, 0xaf,     /* 保留 */
    0xb4, 0x97,     /* 保留 */
    0xb5, 0xFF,     /* 保留 */
    0xb0, 0xC5,     /* 保留 */
    0xb1, 0x94,     /* 保留 */
    0xb2, 0x0f,     /* 保留 */
    0xc4, 0x5c,     /* 保留 */
    
    0xc0, 0xc8,     /* HSIZE8 图像水平尺寸[10:3] */
    0xc1, 0x96,     /* VSIZE8 图像垂直尺寸[10:3] */
    0x8c, 0x00,     /* HSIZE[11] HSIZE[2:0] VSIZE[2:0] */
    0x86, 0x3d,     /* CTRL2 模块使能 DCW/SDE/UV_ADJ/AVG/CMX */
    0x50, 0x00,     /* CTRL1 驱动使能LP_DP/V_DIVIDER/H_DIVIDER */
    
    0x51, 0x90,     /* H_SIZE[7:0](real/4) */
    0x52, 0x2c,     /* V_SIZE[7:0](real/4) */
    0x53, 0x00,     /* 传感器窗口偏移 OFFSET_X */
    0x54, 0x00,     /* 传感器窗口偏移 OFFSET_Y */
    0x55, 0x88,     /* VHYX[7:0] (VSIZE[8]/OFFSET_Y[10:8]/H_SIZE[8]/OFFSET_X[10:8]) */
    0x57, 0x00,     /* TEST[3:0] H_SIZE[9] */

    0x5a, 0x90,     /* ZMOW[7:0] (OUTW[7:0](real/4)) */
    0x5b, 0x2C,     /* ZMOH[7:0] (OUTH[7:0](real/4)) */
    0x5c, 0x05,     /* ZMHH[7:0] (ZMSPD(缩放速度) / OUTH[8] / OUTW[9:8]) */

    0xd3, 0x02,     /* R_DVP_SP 自动设置/DVP_PCLK */ /* auto设置要小心 */

    0xc3, 0xed,     /* CTRL1 模块使能 CIP/DMY/RAW_GMA/DG/AWB/AWB_GAIN/LENC/PRE */
    0x7f, 0x00,     /* 保留 */

    0xda, 0x09,     /* 图像模式 (JPEG & YUV422(默认)) */

    0xe5, 0x1f,     /* 保留 */
    0xe1, 0x67,     /* 保留 */
    0xe0, 0x00,     /* 复位（单片机/SCCB/JPEG/DVP/IPU/CIF） */
    0xdd, 0x7f,     /* 保留 */
    0x05, 0x00,     /* R_BYPASS旁路DSP选择 */
};

/**
 * OV2640 SVGA初始化寄存器序列表
 * 此模式下,帧率可以达到30帧
 * SVGA 800*600
 */
const uint8_t ov2640_svga_init_reg_tbl[][2]= 
{    
    0xff, 0x00,
    0x2c, 0xff,     /* 保留 */
    0x2e, 0xdf,     /* 保留 */
    
    0xff, 0x01,
    0x3c, 0x32,     /* 保留 */

    0x11, 0x00,     /* CLKRC 时钟频率控制 默认 */
    0x09, 0x02,     /* COM2 输出驱动选择 x2 */
    
    0x04, 0xD8,     /* 水平镜像,垂直翻转 */
    
    0x13, 0xe5,     /* COM8 开启带过滤器选择，最小曝光时间1/120秒 曝光自动控制 */
    0x14, 0x48,     /* COM9 设置AGC:32x 默认为64x */
    0x2c, 0x0c,     /* 保留 */
    0x33, 0x78,     /* 保留 */
    0x3a, 0x33,     /* 保留 */
    0x3b, 0xfB,     /* 保留 */
    
    0x3e, 0x00,     /* 保留 */
    0x43, 0x11,     /* 保留 */
    0x16, 0x10,     /* 保留 */
    
    0x39, 0x92,     /* 保留 */
    
    0x35, 0xda,     /* 保留 */
    0x22, 0x1a,     /* 保留 */
    0x37, 0xc3,     /* 保留 */
    0x23, 0x00,     /* 保留 */
    0x34, 0xc0,     /* ARCOM2 设置缩放窗口水平起点 默认0 */
    0x36, 0x1a,     /* 保留 */
    0x06, 0x88,     /* 保留 */
    0x07, 0xc0,     /* 保留 */
    0x0d, 0x87,     /* 保留 */
    0x0e, 0x41,     /* 保留 */
    0x4c, 0x00,     /* 保留 */
    
    0x48, 0x00,     /* COM19 设置缩放模式垂直窗口起点(低2位 默认0) */
    0x5B, 0x00,     /* 保留 */
    0x42, 0x03,     /* 保留 */
    
    0x4a, 0x81,     /* 保留 */
    0x21, 0x99,     /* 保留 */
    
    0x24, 0x40,     /* 亮度信号高范围AEC/AGC操作 当平均亮度大于AEW时，自动模式下AEC/AGC值会下降该寄存器值(默认78) */
    0x25, 0x38,     /* 亮度信号低范围AEC/AGC操作 当平均亮度小于AEW时，自动模式下AEC/AGC值会上升该寄存器值(默认68) */
    0x26, 0x82,     /* 快速模式大步长范围阈值，高阈值和低阈值 */
    0x5c, 0x00,     /* 保留 */
    0x63, 0x00,     /* 保留 */
    0x46, 0x22,     /* 帧长度调整 */
    0x0c, 0x3c,     /* COM3 手动设置带50Hz */
    
    0x61, 0x70,     /* 直方图算法低层 */
    0x62, 0x80,     /* 直方图算法高层 */
    0x7c, 0x05,     /* 保留 */
    
    0x20, 0x80,     /* 保留 */
    0x28, 0x30,     /* 保留 */
    0x6c, 0x00,     /* 保留 */
    0x6d, 0x80,     /* 保留 */
    0x6e, 0x00,     /* 保留 */
    0x70, 0x02,     /* 保留 */
    0x71, 0x94,     /* 保留 */
    0x73, 0xc1,     /* 保留 */
    0x3d, 0x34,     /* 保留 */
    0x5a, 0x57,     /* 保留 */
    
    /* 根据分辨率不同而设置*/
    0x12, 0x40,     /* COM7 分辨率选择、软复位、缩放模式、色条 SVGA 800*600 */ 
    0x17, 0x11,     /* 水平窗口起点 高8位 */
    0x18, 0x43,     /* 水平窗口终点 高8位 */
    0x19, 0x00,     /* 垂直窗口起点 高8位 */
    0x1a, 0x4b,     /* 垂直窗口终点 高8位 */
    0x32, 0x09,     /* REG32 设置像素时钟分频 水平窗口的低3位设置 */
    0x37, 0xc0,     /* 保留 */
    
    0x4f, 0xca,     /* 50Hz Banding AEC 高8位 默认 */
    0x50, 0xa8,     /* 60Hz Banding AEC 高8位 默认 */
    0x5a, 0x23,     /* 保留 */
    0x6d, 0x00,     /* 保留 */
    0x3d, 0x38,     /* 保留 */
    
    
    0xff, 0x00,
    0xe5, 0x7f,     /* 保留 */
    0xf9, 0xc0,     /* 单片机复位 */
    0x41, 0x24,     /* 保留 */
    0xe0, 0x14,     /* 复位（单片机/SCCB/JPEG/DVP/IPU/CIF） */
    0x76, 0xff,     /* 保留 */
    0x33, 0xa0,     /* 保留 */
    0x42, 0x20,     /* 保留 */
    0x43, 0x18,     /* 保留 */
    0x4c, 0x00,     /* 保留 */
    0x87, 0xd5,     /* CTRL3 模块使能BPC/WPC */
    0x88, 0x3f,     /* 保留 */
    0xd7, 0x03,     /* 保留 */
    0xd9, 0x10,     /* 保留 */
    0xd3, 0x82,     /* R_DVP_SP 自动模式/DVP PCLK输出速度控制 */
    
    0xc8, 0x08,     /* 保留 */
    0xc9, 0x80,     /* 保留 */
    
    0x7c, 0x00,     /* SED相关 */
    0x7d, 0x00,     /* SED相关 */
    0x7c, 0x03,     /* SED相关 */
    0x7d, 0x48,     /* SED相关 */
    0x7d, 0x48,     /* SED相关 */
    0x7c, 0x08,     /* SED相关 */
    0x7d, 0x20,     /* SED相关 */
    0x7d, 0x10,     /* SED相关 */
    0x7d, 0x0e,     /* SED相关 */
    
    0x90, 0x00,     /* 保留 */
    0x91, 0x0e,     /* 保留 */
    0x91, 0x1a,     /* 保留 */
    0x91, 0x31,     /* 保留 */
    0x91, 0x5a,     /* 保留 */
    0x91, 0x69,     /* 保留 */
    0x91, 0x75,     /* 保留 */
    0x91, 0x7e,     /* 保留 */
    0x91, 0x88,     /* 保留 */
    0x91, 0x8f,     /* 保留 */
    0x91, 0x96,     /* 保留 */
    0x91, 0xa3,     /* 保留 */
    0x91, 0xaf,     /* 保留 */
    0x91, 0xc4,     /* 保留 */
    0x91, 0xd7,     /* 保留 */
    0x91, 0xe8,     /* 保留 */
    0x91, 0x20,     /* 保留 */
    
    0x92, 0x00,     /* 保留 */
    0x93, 0x06,     /* 保留 */
    0x93, 0xe3,     /* 保留 */
    0x93, 0x05,     /* 保留 */
    0x93, 0x05,     /* 保留 */
    0x93, 0x00,     /* 保留 */
    0x93, 0x04,     /* 保留 */
    0x93, 0x00,     /* 保留 */
    0x93, 0x00,     /* 保留 */
    0x93, 0x00,     /* 保留 */
    0x93, 0x00,     /* 保留 */
    0x93, 0x00,     /* 保留 */
    0x93, 0x00,     /* 保留 */
    0x93, 0x00,     /* 保留 */
    
    0x96, 0x00,     /* 保留 */
    0x97, 0x08,     /* 保留 */
    0x97, 0x19,     /* 保留 */
    0x97, 0x02,     /* 保留 */
    0x97, 0x0c,     /* 保留 */
    0x97, 0x24,     /* 保留 */
    0x97, 0x30,     /* 保留 */
    0x97, 0x28,     /* 保留 */
    0x97, 0x26,     /* 保留 */
    0x97, 0x02,     /* 保留 */
    0x97, 0x98,     /* 保留 */
    0x97, 0x80,     /* 保留 */
    0x97, 0x00,     /* 保留 */
    0x97, 0x00,     /* 保留 */
    
    0xc3, 0xed,     /* CTRL1 模块使能 CIP/DMY/RAW_GMA/DG/AWB/AWB_GAIN/LENC/PRE */
    
    0xa4, 0x00,     /* 保留 */
    0xa8, 0x00,     /* 保留 */
    0xc5, 0x11,     /* 保留 */
    0xc6, 0x51,     /* 保留 */
    0xbf, 0x80,     /* 保留 */
    0xc7, 0x10,     /* 保留 */
    0xb6, 0x66,     /* 保留 */
    0xb8, 0xA5,     /* 保留 */
    0xb7, 0x64,     /* 保留 */
    0xb9, 0x7C,     /* 保留 */
    0xb3, 0xaf,     /* 保留 */
    0xb4, 0x97,     /* 保留 */
    0xb5, 0xFF,     /* 保留 */
    0xb0, 0xC5,     /* 保留 */
    0xb1, 0x94,     /* 保留 */
    0xb2, 0x0f,     /* 保留 */
    0xc4, 0x5c,     /* 保留 */
    
    /*根据分辨率不同而设置 */
    0xc0, 0x64,     /* HSIZE8 图像水平尺寸[10:3] */
    0xc1, 0x4B,     /* VSIZE8 图像垂直尺寸[10:3] */
    0x8c, 0x00,     /* HSIZE[11] HSIZE[2:0] VSIZE[2:0] */
    
    0x86, 0x3D,     /* CTRL2 模块使能 DCW/SDE/UV_ADJ/AVG/CMX */
    0x50, 0x00,     /* CTRL1 驱动使能LP_DP/V_DIVIDER/H_DIVIDER */
    
    0x51, 0xC8,     /* H_SIZE[7:0](real/4) */
    0x52, 0x96,     /* V_SIZE[7:0](real/4) */
    0x53, 0x00,     /* 传感器窗口偏移 OFFSET_X */
    0x54, 0x00,     /* 传感器窗口偏移 OFFSET_Y */
    0x55, 0x00,     /* VHYX[7:0] (VSIZE[8]/OFFSET_Y[10:8]/H_SIZE[8]/OFFSET_X[10:8]) */
    
    0x5a, 0xC8,     /* ZMOW[7:0] (OUTW[7:0](real/4)) */
    0x5b, 0x96,     /* ZMOH[7:0] (OUTH[7:0](real/4)) */
    0x5c, 0x00,     /* ZMHH[7:0] (ZMSPD(缩放速度) / OUTH[8] / OUTW[9:8]) */

    0xd3, 0x02,     /* R_DVP_SP 自动设置/DVP_PCLK */ /* auto设置要小心 */ /* auto设置要小心 */

    0xc3, 0xed,     /* CTRL1 模块使能 CIP/DMY/RAW_GMA/DG/AWB/AWB_GAIN/LENC/PRE */
    0x7f, 0x00,     /* 保留 */

    0xda, 0x09,     /* 图像模式 (JPEG & YUV422(默认)) */

    0xe5, 0x1f,     /* 保留 */
    0xe1, 0x67,     /* 保留 */
    0xe0, 0x00,     /* 复位（单片机/SCCB/JPEG/DVP/IPU/CIF） */
    0xdd, 0x7f,     /* 保留 */
    0x05, 0x00,     /* R_BYPASS旁路DSP选择 */
};
const uint8_t ov2640_rgb565_reg_tbl[][2]=
{
    0xFF, 0x00,
    0xDA, 0x09,     /* 图像模式 (RGB565 & YUV设置) */
    
    0xD7, 0x03,     /* 保留 */
    0xDF, 0x02,     /* 保留 */
    0x33, 0xa0,     /* 保留 */
    0x3C, 0x00,     /* 保留 */
    0xe1, 0x67,     /* 保留 */

    0xff, 0x01, 
    
    0xe0, 0x00,     /* 保留 */
    0xe1, 0x00,     /* 保留 */
    0xe5, 0x00,     /* 保留 */
    0xd7, 0x00,     /* 保留 */
    0xda, 0x00,     /* 保留 */
    
    0x15, 0x09,     /* 设置输出时序(PCLK上升沿捕获/VSYNC高电平有效数据/HREF高电平有效数据) */
    
    0xe0, 0x00,     /* 保留 */ 
    
};

#endif