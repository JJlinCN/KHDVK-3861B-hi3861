#ifndef SOIL_H
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <securec.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "ohos_types.h"
#include "iot_errno.h"
#include "iot_gpio.h"
#include "hi_adc.h"

/******************************************************************************\
                             Variables definitions
\******************************************************************************/

#define SOIL_ADC_CHANNEL                    HI_ADC_CHANNEL_4        //土壤湿度adc采集通道
#define WATER_PUMP_PIN                      5                       //GPIO10控制电磁阀开关
#define WATER_PUMP_PIN_MODE                 IOT_GPIO_DIR_OUT        //GPIO10输出状态
#define WATER_PUMP_POWER_PIN                10                      //GPIO5控制电磁阀电源
#define WATER_PUMP_POWER_PIN_MODE           IOT_GPIO_DIR_OUT        //GPIO5输出状态
#define WATER_PUMP_ON                       1                       //水泵开
#define WATER_PUMP_OFF                      0                       //水泵关
#define WATER_PUMP_POWER_ON                 1                       //水泵开
#define WATER_PUMP_POWER_OFF                0                       //水泵关


//------------------------------------------------------------------------------
// IO配置句柄
typedef struct
{
    uint8_t         Num;                    //GPIO 编号
    IotGpioDir      Mode;                   //工作模式
    IotGpioValue    Value;                  //GPIO值
}TS_IO_PARAM;
//------------------------------------------------------------------------------
// ADC配置句柄
typedef struct
{
    hi_adc_channel_index    Channel;        //通道编号
    hi_adc_equ_model_sel    EquMode;        //平均算法模式
    hi_adc_cur_bais         CurBais;        //模拟电源控制
    uint16_t                DelayCnt;       //延时计数
}TS_ADC_PARAM;
//------------------------------------------------------------------------------
// 土壤湿度控制模组句柄
typedef struct
{
    uint8_t Humidity;                   //湿度值
    uint8_t HumiThreshold;              //水泵开启对应湿度阈值   
}TS_SOIL_PARAM;

char Measure_Soil_Humi(uint8_t *Humi);
char Gpio_Init(TS_IO_PARAM *Param);
char Soil_Init(void);
void Water_Pump_OnOff_Set(uint8_t State);
void Water_Pump_Power_OnOff_Set(uint8_t State);

#endif