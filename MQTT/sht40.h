#ifndef SHT40_H
#include <stdio.h>
#include <unistd.h>
#include <securec.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "ohos_types.h"
#include "iot_errno.h"
#include "iot_gpio.h"
#include "iot_i2c.h"
// #include "sht40.h"
#include <hi_io.h>

#define FAN_PIN                     8                   //GPIO8控制散热风扇开关
#define FAN_PIN_MODE                IOT_GPIO_DIR_OUT    //GPIO8输出状态
#define FAN_POWER_PIN               12                  //GPIO12控制散热风扇电源开关
#define FAN_POWER_PIN_MODE          IOT_GPIO_DIR_OUT    //GPIO12输出状态

#define SHT40_I2C_IDX               0                   //I2C设备号
#define SHT40_I2C_BAUDRATE          (400*1000)          //I2C波特率
#define SHT40_ADDR                  0x44                //SHT40设备地址
#define SHT40_STATUS_RESPONSE_MAX   6                   //读取传感器数据长度

#define SHT40_CMD_TRIGGER           0xFD                //高精度测量命令

//------------------------------------------------------------------------------
// 土壤湿度控制模组句柄
typedef struct
{
    uint8_t Temp;                       //温度值
    uint8_t Humidity;                   //湿度值
    uint8_t TempThreshold;              //温度报警阈值
    uint8_t HumiThreshold;              //湿度报警阈值
}TS_SHT40_PARAM;

static uint32_t SHT40_Read(uint8_t* buffer, uint32_t buffLen);
static uint32_t SHT40_Write(uint8_t* buffer, uint32_t buffLen);
uint32_t SHT40_GetMeasureResult(uint8_t* temp, uint8_t* humi);
uint32_t SHT40_StartMeasure(void);
void SHT40_Init(void);
void Fan_Init(void);


#endif