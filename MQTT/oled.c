/******************************************************************************\
**  版    权 :  深圳开鸿数字产业发展有限公司（2021）
**  文件名称 :  oled.c
**  功能描述 :  oled显示驱动：SSD1306 OLED 驱动IC驱动代码，驱动方式:8080并口/4线串口
**  作    者 :  王滨泉
**  日    期 :  2021.10.23
**  版    本 :  V0.0.1
**  变更记录 :  V0.0.1/2021.10.23
                1 首次创建                 
\******************************************************************************/

/******************************************************************************\
                                 Includes
\******************************************************************************/
#include "oled.h"
#include "stdlib.h"
#include "oledfont.h"  	 
#include "iot_i2c.h"

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "QR_Encode.h"
/******************************************************************************\
                             Variables definitions
\******************************************************************************/
#define OLED_I2C_IDX        0
#define OLED_I2C_BAUDRATE   (400*1000)  // 400k
#define OLED_I2C_ADDR       0x78        // 默认地址为 0x78
#define OLED_I2C_CMD        0x00        // 0000 0000       写命令
#define OLED_I2C_DATA       0x40        // 0100 0000(0x40) 写数据
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

//OLED的显存
//存放格式如下.
//[0]0 1 2 3 ... 127	
//[1]0 1 2 3 ... 127	
//[2]0 1 2 3 ... 127	
//[3]0 1 2 3 ... 127	
//[4]0 1 2 3 ... 127	
//[5]0 1 2 3 ... 127	
//[6]0 1 2 3 ... 127	
//[7]0 1 2 3 ... 127 		   
uint8_t OLED_GRAM[128][8] = {0};
uint8_t OLED_GRAM_BANK[128][8] = {0};

/******************************************************************************\
                             Functions definitions
\******************************************************************************/

/*
* 函数名称 : OledWriteByte
* 功能描述 : Oled通过IIc接口写入数据
* 参    数 : regAddr - 操作地址
			 byte - 数据
* 返回值   : 0 - 写入成功
            -1 - 写入失败
* 示    例 : result = OledWriteByte(regAddr,byte);
*/
/******************************************************************************/ 
static uint32_t OledWriteByte(uint8_t regAddr, uint8_t byte)
/******************************************************************************/ 
{
	// TODO: Check SSD1306 DataSheet if must write by byte.

    uint8_t buffer[] = {regAddr, byte};

	return IoTI2cWrite(OLED_I2C_IDX, OLED_I2C_ADDR, buffer, ARRAY_SIZE(buffer));
}

/*
* 函数名称 : OledWriteCmd
* 功能描述 : Oled写命令
* 参    数 : cmd - 命令字
* 返回值   : 0 - 写入成功
            -1 - 写入失败
* 示    例 : result = OledWriteCmd(cmd);
*/
/******************************************************************************/ 
static uint32_t OledWriteCmd(uint8_t cmd)
/******************************************************************************/ 
{
    return OledWriteByte(OLED_I2C_CMD, cmd);
}

/*
* 函数名称 : OledWriteData
* 功能描述 : Oled写数据
* 参    数 : data - 数据
* 返回值   : 0 - 写入成功
            -1 - 写入失败
* 示    例 : result = OledWriteData(data);
*/
/******************************************************************************/ 
static uint32_t OledWriteData(uint8_t data)
/******************************************************************************/ 
{
	return OledWriteByte(OLED_I2C_DATA, data);
}	


/*
* 函数名称 : OledWriteDatas
* 功能描述 : Oled写数据
* 参    数 : data - 数据指针
			 len - 数据长度
* 返回值   : 0 - 写入成功
            -1 - 写入失败
* 示    例 : result = OledWriteDatas(&data,len);
*/
/******************************************************************************/ 
static uint32_t OledWriteDatas(uint8_t *data, uint8_t len)
/******************************************************************************/ 
{
	uint8_t buffer[129] = {0};

	buffer[0] = OLED_I2C_DATA;
	memcpy(&buffer[1], data, len);

	return IoTI2cWrite(OLED_I2C_IDX, OLED_I2C_ADDR, buffer, len+1);

}
/*
* 函数名称 : OLED_Refresh_Gram
* 功能描述 : 更新显存到LCD	
* 参    数 : 空
* 返回值   : 空
* 示    例 : OLED_Refresh_Gram();
*/
/******************************************************************************/  
void OLED_Refresh_Gram(void)
/******************************************************************************/  
{
	uint8_t i,n;
	uint8_t temp[128] = {0};

	for(i=0;i<8;i++)  
	{  
		OLED_WR_Byte (0xb0+i,OLED_CMD);    //设置页地址（0~7）
		OLED_WR_Byte (0x00,OLED_CMD);      //设置显示位置—列低地址
		OLED_WR_Byte (0x10,OLED_CMD);      //设置显示位置—列高地址   
		//原来的写法没有考虑到连续写数据，刷屏效率低
		for(n=0;n<128;n++)
		{
			temp[n]	= OLED_GRAM[n][i];
		}
		OledWriteDatas(temp,128);
		// for(n=0;n<128;n++)
		// {
		// 	OLED_WR_Byte(OLED_GRAM[n][i],OLED_DATA); 
		// }
	}   
}
#if OLED_MODE==1	//8080并口 
/*
* 函数名称 : OLED_WR_Byte
* 功能描述 : 向SSD1306写入一个字节。	
* 参    数 : dat - 要写入的数据/命令
			 cmd - 数据/命令标志 0,表示命令;1,表示数据;
* 返回值   : 空
* 示    例 : OLED_WR_Byte();
*/
/******************************************************************************/  
void OLED_WR_Byte(uint8_t dat,uint8_t cmd)
/******************************************************************************/  
{
	DATAOUT(dat);	    
 	OLED_RS=cmd;
	OLED_CS=0;	   
	OLED_WR=0;	 
	OLED_WR=1;
	OLED_CS=1;	  
	OLED_RS=1;	 
} 	    	    
#else
/*
* 函数名称 : OLED_WR_Byte
* 功能描述 : 向SSD1306写入一个字节。	
* 参    数 : dat - 要写入的数据/命令
			 cmd - 数据/命令标志 0,表示命令;1,表示数据;
* 返回值   : 空
* 示    例 : OLED_WR_Byte(dat, cmd);
*/
/******************************************************************************/  
void OLED_WR_Byte(uint8_t dat,uint8_t cmd)
/******************************************************************************/ 
{	
	uint8_t i;			  
	// OLED_RS=cmd; //写命令 
	// OLED_CS=0;		  
	// for(i=0;i<8;i++)
	// {			  
	// 	OLED_SCLK=0;
	// 	if(dat&0x80)OLED_SDIN=1;
	// 	else OLED_SDIN=0;
	// 	OLED_SCLK=1;
	// 	dat<<=1;   
	// }				 
	// OLED_CS=1;		  
	// OLED_RS=1; 
	if(cmd)  	  
	{
		OledWriteData(dat);
	}
	else
	{
		OledWriteCmd(dat);
		
	}
} 
#endif
	  	  
/*
* 函数名称 : OLED_Display_On
* 功能描述 : 开启OLED显示  	
* 参    数 : 空
* 返回值   : 空
* 示    例 : OLED_Display_On();
*/
/******************************************************************************/   
void OLED_Display_On(void)
/******************************************************************************/   
{
	OLED_WR_Byte(0X8D,OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X14,OLED_CMD);  //DCDC ON
	OLED_WR_Byte(0XAF,OLED_CMD);  //DISPLAY ON
}  
/*
* 函数名称 : OLED_Display_Off
* 功能描述 : 关闭OLED显示   	
* 参    数 : 空
* 返回值   : 空
* 示    例 : OLED_Display_Off();
*/
/******************************************************************************/   
void OLED_Display_Off(void)
/******************************************************************************/   
{
	OLED_WR_Byte(0X8D,OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X10,OLED_CMD);  //DCDC OFF
	OLED_WR_Byte(0XAE,OLED_CMD);  //DISPLAY OFF
}		   			 

/*
* 函数名称 : OLED_Clear
* 功能描述 : 清屏函数,清完屏,整个屏幕是黑色的!和没点亮一样!!!	    	
* 参    数 : 空
* 返回值   : 空
* 示    例 : OLED_Clear();
*/
/******************************************************************************/    
void OLED_Clear(void)  
/******************************************************************************/ 
{  
	uint8_t i,n;  
	for(i=0;i<8;i++)for(n=0;n<128;n++)OLED_GRAM[n][i]=0X00;  
	
	// OLED_Refresh_Gram();//更新显示
}
	
/*
* 函数名称 : OLED_DrawPoint
* 功能描述 : 画点 	    	
* 参    数 : x - 横轴坐标，取值范围：0~127，
			 y - 竖轴坐标，取值范围：0~63
			 t - 1：填充，0：清空	
* 返回值   : 空
* 示    例 : OLED_DrawPoint(x,y,t);
*/
/******************************************************************************/  		   
void OLED_DrawPoint(uint8_t x,uint8_t y,uint8_t t)
/******************************************************************************/  	
{
	uint8_t pos,bx,temp=0;
	if(x>127||y>63)return;//超出范围了.
	pos=7-y/8;
	bx=y%8;
	temp=1<<(7-bx);
	if(t)OLED_GRAM[x][pos]|=temp;
	else OLED_GRAM[x][pos]&=~temp;	    
}

/*
* 函数名称 : OLED_Fill
* 功能描述 : 填充区域的对角坐标, 确保x1<=x2;y1<=y2 0<=x1<=127 0<=y1<=63	    	
* 参    数 : x1,x2 - 横轴坐标，取值范围：0~127
			 y1,y2 - 竖轴坐标，取值范围：0~63
			 dot - 1：填充，0：清空	
* 返回值   : 空
* 示    例 : OLED_Fill(x1,y1,x2,y2,dot);
*/
/******************************************************************************/  
void OLED_Fill(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2,uint8_t dot)  
/******************************************************************************/  
{  
	uint8_t x,y;  
	for(x=x1;x<=x2;x++)
	{
		for(y=y1;y<=y2;y++)OLED_DrawPoint(x,y,dot);
	}													    
	// OLED_Refresh_Gram();//更新显示
}
/*
* 函数名称 : OLED_ShowChar
* 功能描述 : 在指定位置显示一个字符,包括部分字符	    	
* 参    数 : x - 横轴坐标，取值范围：0~127，
			 y - 竖轴坐标，取值范围：0~63
			 chr - 字符
			 size - 选择字体 12/16/24
			 mode - 0,反白显示;1,正常显示
* 返回值   : 空
* 示    例 : OLED_ShowChar(x,y,chr,size,mode);
*/
/******************************************************************************/  
void OLED_ShowChar(uint8_t x,uint8_t y,uint8_t chr,uint8_t size,uint8_t mode)
/******************************************************************************/  
{      			    
	uint8_t temp,t,t1;
	uint8_t y0=y;
	uint8_t csize=(size/8+((size%8)?1:0))*(size/2);		//得到字体一个字符对应点阵集所占的字节数
	chr=chr-' ';//得到偏移后的值		 
    for(t=0;t<csize;t++)
    {   
		if(size==12)temp=asc2_1206[chr][t]; 	 	//调用1206字体
		else if(size==16)temp=asc2_1608[chr][t];	//调用1608字体
		else if(size==24)temp=asc2_2412[chr][t];	//调用2412字体
		else return;								//没有的字库
        for(t1=0;t1<8;t1++)
		{
			if(temp&0x80)OLED_DrawPoint(x,y,mode);
			else OLED_DrawPoint(x,y,!mode);
			temp<<=1;
			y++;
			if((y-y0)==size)
			{
				y=y0;
				x++;
				break;
			}
		}  	 
    }          
}
//m^n函数
uint32_t mypow(uint8_t m,uint8_t n)
{
	uint32_t result=1;	 
	while(n--)result*=m;    
	return result;
}				   	
/*
* 函数名称 : OLED_ShowNum
* 功能描述 : 显示2个数字	    	
* 参    数 : x - 横轴坐标，取值范围：0~127，
			 y - 竖轴坐标，取值范围：0~63
			 num - 数值(0~4294967295)
			 len - 数字的位数
			 size - 选择字体 12/16/24 
* 返回值   : 空
* 示    例 : OLED_ShowNum(x,y,chr,size,mode);
*/
/******************************************************************************/  	  
void OLED_ShowNum(uint8_t x,uint8_t y,uint32_t num,uint8_t len,uint8_t size)
/******************************************************************************/  
{         	
	uint8_t t,temp;
	uint8_t enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				OLED_ShowChar(x+(size/2)*t,y,' ',size,1);
				continue;
			}else enshow=1; 
		 	 
		}
	 	OLED_ShowChar(x+(size/2)*t,y,temp+'0',size,1); 
	}
} 
/*
* 函数名称 : OLED_ShowString
* 功能描述 : 显示字符串	    	
* 参    数 : x - 横轴坐标，取值范围：0~127，
			 y - 竖轴坐标，取值范围：0~63
			 p - 字符串起始地址 
			 size - 选择字体 12/16/24 
* 返回值   : 空
* 示    例 : OLED_ShowString(x,y,p,size);
*/
/******************************************************************************/  
void OLED_ShowString(uint8_t x,uint8_t y,const uint8_t *p,uint8_t size)
/******************************************************************************/  
{	
    while((*p<='~')&&(*p>=' '))//判断是不是非法字符!
    {       
        if(x>(128-(size/2))){x=0;y+=size;}
        if(y>(64-size)){y=x=0;OLED_Clear();}
        OLED_ShowChar(x,y,*p,size,1);	 
        x+=size/2;
        p++;
    }  
	
}	
/*
* 函数名称 : OLED_ShowString
* 功能描述 : 显示字符串	    	
* 参    数 : str - 二维码内容
			 offset - 二维码X轴位置
			 colour - 二维码正反方向，0：反，1：正
* 返回值   : 空
* 示    例 : OLED_ShowString(str,offset,colour);
*/
/******************************************************************************/  
void OLED_QRcode_Display(char *str,uint8_t offset,uint8_t colour)
/******************************************************************************/  
{
	uint32_t i,j,point;
	uint8_t exp = 1;					//放大倍数
	uint8_t pos_X,pos_Y;   
    if(colour)
	{
		point = 1;
	} 
    else
	{
		point = 0;    
	}
    EncodeData(str);        
	exp = 64 / m_nSymbleSize;           //根据屏幕尺寸自动计算最佳放大倍数
	pos_Y = (64 - exp*m_nSymbleSize)/2;	//二维码左下方第一个点的纵坐标
	pos_X = pos_Y + offset;             //二维码左下方第一个点的横坐标
    
    if(point==0)
	{
		OLED_Fill(pos_X-2,pos_Y-2,pos_X + exp*m_nSymbleSize+2,pos_Y + exp*m_nSymbleSize+2,1);//给反显的二维码填充底色
	}
    //exp*m_nSymbleSize为放大后二维码的边长(二维码是正方形)
	for(i=0;i<m_nSymbleSize;i++)
	{
		for(j=0;j<m_nSymbleSize;j++)
		{
				if(m_byModuleData[i][j] == 1)
				{
					OLED_Fill(pos_X,pos_Y,pos_X+exp,pos_Y+exp,point);	//画矩形并填充
				}
				if(m_byModuleData[i][j] == 0)
				{
					OLED_Fill(pos_X,pos_Y,pos_X+exp,pos_Y+exp,1-point);	//清空矩形区域
				}
				pos_Y += exp;
		}
		pos_X += exp;
		pos_Y -= m_nSymbleSize*exp;
	}
    // OLED_Refresh_Gram();
}

/*
* 函数名称 : OLED_Init
* 功能描述 : 初始化SSD1306  	
* 参    数 : 空 
* 返回值   : 空
* 示    例 : OLED_Init();
*/
/******************************************************************************/  				    
void OLED_Init(void)
/******************************************************************************/  
{ 	 	 			 	 				 
	// RCC->APB2ENR|=1<<4;    //使能PORTC时钟 
	// RCC->APB2ENR|=1<<5;    //使能PORTD时钟 
	// RCC->APB2ENR|=1<<8;    //使能PORTG时钟

  	// GPIOD->CRL&=0XF0FF0FFF;//PD3,6 推挽输出 
  	// GPIOD->CRL|=0X03003000;	 
  	// GPIOD->ODR|=1<<3;
  	// GPIOD->ODR|=1<<6;	    
#if OLED_MODE==1			//8080并口模式 
	GPIOC->CRL=0X33333333; 	//PC0~7 OUT
	GPIOC->ODR|=0X00FF;
		
  	GPIOG->CRH&=0X000FFFFF;	//PG13,14,15 OUT
  	GPIOG->CRH|=0X33300000;	 
	GPIOG->ODR|=7<<13; 
#else						//4线SPI模式
	// GPIOC->CRL&=0XFFFFFF00; //PC0,1 OUT
	// GPIOC->CRL|=0X00000033;	    
 	// GPIOC->ODR|=3<<0;

 	// GPIOG->CRH&=0X0FFFFFFF;	//RST	   
 	// GPIOG->CRH|=0X30000000;	 
	// GPIOG->ODR|=1<<15;
	IoTI2cInit(OLED_I2C_IDX, OLED_I2C_BAUDRATE);
#endif									  
	// OLED_CS=1;
	// OLED_RS=1;	 
	
	// OLED_RST=0;
	// delay_ms(100);
	// OLED_RST=1; 

	OLED_WR_Byte(0xAE,OLED_CMD); //关闭显示
	OLED_WR_Byte(0xD5,OLED_CMD); //设置时钟分频因子,震荡频率
	OLED_WR_Byte(0x80,OLED_CMD); //[3:0],分频因子;[7:4],震荡频率
	OLED_WR_Byte(0xA8,OLED_CMD); //设置驱动路数
	OLED_WR_Byte(0X3F,OLED_CMD); //默认0X3F(1/64) 
	OLED_WR_Byte(0xD3,OLED_CMD); //设置显示偏移
	OLED_WR_Byte(0X00,OLED_CMD); //默认为0

	OLED_WR_Byte(0x40,OLED_CMD); //设置显示开始行 [5:0],行数.
													    
	OLED_WR_Byte(0x8D,OLED_CMD); //电荷泵设置
	OLED_WR_Byte(0x14,OLED_CMD); //bit2，开启/关闭
	OLED_WR_Byte(0x20,OLED_CMD); //设置内存地址模式
	OLED_WR_Byte(0x02,OLED_CMD); //[1:0],00，列地址模式;01，行地址模式;10,页地址模式;默认10;
	OLED_WR_Byte(0xA1,OLED_CMD); //段重定义设置,bit0:0,0->0;1,0->127;
	OLED_WR_Byte(0xC0,OLED_CMD); //设置COM扫描方向;bit3:0,普通模式;1,重定义模式 COM[N-1]->COM0;N:驱动路数
	OLED_WR_Byte(0xDA,OLED_CMD); //设置COM硬件引脚配置
	OLED_WR_Byte(0x12,OLED_CMD); //[5:4]配置
		 
	OLED_WR_Byte(0x81,OLED_CMD); //对比度设置
	OLED_WR_Byte(0x66,OLED_CMD); //1~255;默认0X7F (亮度设置,越大越亮)
	OLED_WR_Byte(0xD9,OLED_CMD); //设置预充电周期
	OLED_WR_Byte(0x22,OLED_CMD); //[3:0],PHASE 1;[7:4],PHASE 2;(可消除屏幕抖动，未改之前值为0xF1)
	OLED_WR_Byte(0xDB,OLED_CMD); //设置VCOMH 电压倍率
	OLED_WR_Byte(0x30,OLED_CMD); //[6:4] 000,0.65*vcc;001,0.77*vcc;011,0.83*vcc;

	OLED_WR_Byte(0xA4,OLED_CMD); //全局显示开启;bit0:1,开启;0,关闭;(白屏/黑屏)
	OLED_WR_Byte(0xA6,OLED_CMD); //设置显示方式;bit0:1,反相显示;0,正常显示	    						   
	OLED_WR_Byte(0xAF,OLED_CMD); //开启显示	 
	OLED_Clear();
}  
/******************************* End of File (C) ******************************/




























