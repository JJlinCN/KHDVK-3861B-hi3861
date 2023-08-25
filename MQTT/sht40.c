/******************************************************************************\
                                 Includes
\******************************************************************************/
#include "sht40.h"

extern TS_SHT40_PARAM s_Sht40;
/******************************************************************************\
                             Functions definitions
\******************************************************************************/
/*
* 函数名称 : SHT40_Read
* 功能描述 : 读取温湿度数据
* 参    数 : buffer - 读取数据指针
             buffLen - 读取数据长度
* 返回值   : 0 - 读取成功
            -1 - 读取失败
* 示    例 : result = SHT40_Read(&buffer,buffLen);
*/
/******************************************************************************/ 
static uint32_t SHT40_Read(uint8_t* buffer, uint32_t buffLen)
/******************************************************************************/ 
{
    uint32_t retval;

    retval = IoTI2cRead(SHT40_I2C_IDX,(SHT40_ADDR<<1)|1,buffer,buffLen);
    if (retval != 0) {
        printf("I2cRead() failed, %0X!\n", retval);
        return retval;
    }
    return 0;
}
/*
* 函数名称 : SHT40_Write
* 功能描述 : 写命令
* 参    数 : buffer - 写入数据指针
             buffLen - 写入数据长度
* 返回值   : 0 - 写入成功
            -1 - 写入失败
* 示    例 : result = SHT40_Write(&buffer,buffLen);
*/
/******************************************************************************/ 
static uint32_t SHT40_Write(uint8_t* buffer, uint32_t buffLen)
/******************************************************************************/
{
    uint32_t retval = IoTI2cWrite(SHT40_I2C_IDX,(SHT40_ADDR<<1)|0,buffer,buffLen);
    if (retval != 0) {
        printf("I2cWrite(%02X) failed, %0X!\n", buffer[0], retval);
        return retval;
    }
    return 0;
}
/*
* 函数名称 : SHT40_GetMeasureResult
* 功能描述 : 获取测量结果，拼接转换为标准值
* 参    数 : temp - 温度值
             humi - 湿度值
* 返回值   : 0 - 测量成功
            -1 - 测量失败
* 示    例 : result = SHT40_GetMeasureResult(&temp,humi);
*/
/******************************************************************************/ 
uint32_t SHT40_GetMeasureResult(uint8_t* temp, uint8_t* humi)
/******************************************************************************/ 
{

    uint32_t retval = 0;

    float t_degC=0;
    float rh_pRH=0;
    float t_ticks=0.0;
    float rh_ticks=0.0;
    
    if (temp == NULL || humi == NULL) 
    {
        return -1;
    }

    uint8_t buffer[SHT40_STATUS_RESPONSE_MAX] = { 0 };
    memset(&buffer, 0x0, sizeof(buffer));
    retval = SHT40_Read(buffer, sizeof(buffer));  // recv status command result
    if (retval != 0) 
    {
        return retval;
    }

    t_ticks=buffer[0]*256+buffer[1];
    rh_ticks=buffer[3]*256+buffer[4];
    t_degC=-45+175*t_ticks/65535;
    rh_pRH=-6+125*rh_ticks/65535; 
    if(rh_pRH >= 100)
    {
        rh_pRH = 100;
    }
    if(rh_pRH < 0)
    {
        rh_pRH = 0;
    }
    *humi = (uint8_t)rh_pRH;

    *temp = (uint8_t)t_degC;

    return 0;
}
/*
* 函数名称 : SHT40_StartMeasure
* 功能描述 : 开始测量
* 参    数 : temp - 温度值
             humi - 湿度值
* 返回值   : 0 - 操作成功
            -1 - 操作失败
* 示    例 : result = SHT40_StartMeasure();
*/
/******************************************************************************/ 
uint32_t SHT40_StartMeasure(void)
/******************************************************************************/ 
{
    uint8_t triggerCmd[] = {SHT40_CMD_TRIGGER};
    return SHT40_Write(triggerCmd, sizeof(triggerCmd));
}
/*
* 函数名称 : SHT40_Init
* 功能描述 : 温湿度传感器硬件初始化
* 参    数 : 无
* 返回值   : 无
* 示    例 : SHT40_Init();
*/
/******************************************************************************/ 
void SHT40_Init(void)
/******************************************************************************/ 
{
    IoTI2cInit(SHT40_I2C_IDX, SHT40_I2C_BAUDRATE);          //I2C初始化
}
/*
* 函数名称 : Fan_Init
* 功能描述 : 风扇硬件初始化
* 参    数 : 无
* 返回值   : 无
* 示    例 : Fan_Init();
*/
/******************************************************************************/ 
void Fan_Init(void)
/******************************************************************************/ 
{
    IoTGpioInit(FAN_PIN);                                   //初始化散热风扇GPIO
    IoTGpioSetDir(FAN_PIN, FAN_PIN_MODE);                   //设置GPIO工作状态
    IoTGpioSetOutputVal(FAN_PIN,IOT_GPIO_VALUE0);           //关闭散热风扇

    hi_io_set_func(HI_IO_NAME_GPIO_12, HI_IO_FUNC_GPIO_12_GPIO); /* GPIO12 */
    IoTGpioInit(FAN_POWER_PIN);                             //初始化散热风扇电源GPIO
    IoTGpioSetDir(FAN_POWER_PIN, FAN_POWER_PIN_MODE);       //设置GPIO工作状态
    IoTGpioSetOutputVal(FAN_POWER_PIN,IOT_GPIO_VALUE0);     //关闭散热风扇电源
}
/******************************* End of File (C) ******************************/