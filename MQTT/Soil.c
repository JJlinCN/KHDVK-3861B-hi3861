#include "Soil.h"

extern TS_SOIL_PARAM s_Soil;
extern TS_ADC_PARAM  s_Soil_Adc;
/******************************************************************************\
                             Functions definitions
\******************************************************************************/
/*
* 函数名称 : Measure_Soil_Humi
* 功能描述 : 测量土壤湿度
* 参    数 : Humi - 数据指针
* 返回值   : 0 - 测量成功
            -1 - 测量失败
* 示    例 : result = Measure_Soil_Humi(&Humi);
*/
/******************************************************************************/ 
char Measure_Soil_Humi(uint8_t *Humi)
/******************************************************************************/
{
    char ret = 0;
    uint16_t data;
    float voltage = 0;
    if(hi_adc_read(s_Soil_Adc.Channel,&data,s_Soil_Adc.EquMode,s_Soil_Adc.CurBais,s_Soil_Adc.DelayCnt) == HI_ERR_SUCCESS)
    {
        voltage = hi_adc_convert_to_voltage(data);  //ad值转换成电压值
        if(voltage > 3.3)
        {
            voltage = 3.3;
        }
        printf("voltage = %.2f\n",voltage);
        *Humi = (100-(uint8_t)((voltage/3.3)*100)); //电压值转成为湿度值
        ret = 0;                                    //测量成功
    }
    else
    {
        printf("Measure soil humidity fales!!!\n");
        ret = -1;                                   //测量失败                                   
    }
    return ret;
}

/*
* 函数名称 : Gpio_Init
* 功能描述 : gpio工作模式配置
* 参    数 : Param - IO配置句柄指针
* 返回值   : 0 - 初始化成功
            -1 - 初始化失败
* 示    例 : result = Gpio_Init(&Param);
*/
/******************************************************************************/ 
char Gpio_Init(TS_IO_PARAM *Param)
/******************************************************************************/
{
    if(Param == NULL)
    {
        printf("Gpio init fales!!!\n");
        return -1;
    }
    IoTGpioInit(Param->Num);                                //初始化GPIO
    IoTGpioSetDir(Param->Num, Param->Mode);                 //设置GPIO工作状态
    if(Param->Mode == IOT_GPIO_DIR_OUT)
    {
        IoTGpioSetOutputVal(Param->Num, Param->Value);      //输出状态，设置输出电平
    }
    else
    {
        IoTGpioGetInputVal(Param->Num, &Param->Value);       //输入状态，读取电平
    }
    return 0;
}
/*
* 函数名称 : Soil_Init
* 功能描述 : 土壤湿度检测初始化
* 参    数 : 空
* 返回值   : 0 - 初始化成功
            -1 - 初始化失败
* 示    例 : result = Soil_Init();
*/
/******************************************************************************/ 
char Soil_Init(void)
/******************************************************************************/
{
    TS_IO_PARAM param;
    uint8_t ret;
    //GPIO5 设置为输出
    param.Num  = WATER_PUMP_POWER_PIN;
    param.Mode = WATER_PUMP_POWER_PIN_MODE;
    param.Value = WATER_PUMP_POWER_OFF;
    if(Gpio_Init(&param) != 0)
    {
        return -1;
    }
    //GPIO10 设置为输出
    param.Num  = WATER_PUMP_PIN;                
    param.Mode = WATER_PUMP_PIN_MODE;
    param.Value = WATER_PUMP_OFF;
    if(Gpio_Init(&param) != 0)
    {
        return -1;
    }
    //GPIO12 设置为输出

    param.Num  = 12;                

    param.Mode = WATER_PUMP_PIN_MODE;

    param.Value = WATER_PUMP_POWER_ON;

    if(Gpio_Init(&param) != 0)

    {

        return -1;

    }
    // //GPIO9 设置为输入
    // param.Num  = 9;                
    // param.Mode = IOT_GPIO_DIR_IN;
    // if(Gpio_Init(&param) != 0)
    // {
    //     return -1;
    // }

    //ADC 配置
    s_Soil_Adc.Channel  = SOIL_ADC_CHANNEL;
    s_Soil_Adc.EquMode  = HI_ADC_EQU_MODEL_8;
    s_Soil_Adc.CurBais  = HI_ADC_CUR_BAIS_DEFAULT;
    s_Soil_Adc.DelayCnt = 0;
    return 0;
}
/*
* 函数名称 : Water_Pump_OnOff_Set
* 功能描述 : 水泵开关设置
* 参    数 : State - 开关状态：0 - 关闭，1 - 打开
* 返回值   : 空
* 示    例 : Water_Pump_OnOff_Set(State);
*/
/******************************************************************************/ 
void Water_Pump_OnOff_Set(uint8_t State)
/******************************************************************************/
{
    IoTGpioSetOutputVal(WATER_PUMP_PIN, State);
}
/*
* 函数名称 : Water_Pump_Power_OnOff_Set
* 功能描述 : 水泵电源开关设置
* 参    数 : State - 开关状态：0 - 关闭，1 - 打开
* 返回值   : 空
* 示    例 : Water_Pump_Power_OnOff_Set(State);
*/
/******************************************************************************/ 
void Water_Pump_Power_OnOff_Set(uint8_t State)
/******************************************************************************/
{
    IoTGpioSetOutputVal(WATER_PUMP_POWER_PIN, State);
}
