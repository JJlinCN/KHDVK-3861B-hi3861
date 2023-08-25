#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifi_device.h"
#include "lwip/netifapi.h"
#include "lwip/api_shell.h"
#include "wifi_utils.h"
#include "mqtt_utils.h"
#include <at.h>
#include <hi_at.h>

#include "Soil.h"
#include "sht40.h"


#define SSID "JJlinCN"
#define PASSWORD "qq893140"

int global_auto_control = 0;
TS_SOIL_PARAM s_Soil = {0};
TS_ADC_PARAM  s_Soil_Adc = {0};
TS_SHT40_PARAM s_Sht40 = {0};
//导入OledTask声明
extern void OledTask(void *arg);


static void mqtt_test_thread(void *arg)
{
    (void)arg;
    connect_wifi(SSID,PASSWORD);
    osDelay(100);
    mqtt();
}

static void SHT40Task(void *arg)
{
    (void)arg;
    uint32_t retval = 0;
    float humidity = 0.0f;
    float temperature = 0.0f;

    Fan_Init();                         //风扇IO初始化
    SHT40_Init();                       //温湿度传感器IO初始化
    s_Sht40.TempThreshold = 20;         //温度阈值20℃

    while(1) 
    {
        if(global_auto_control == 1){
            SHT40_StartMeasure();
            usleep(20*1000);  
            SHT40_GetMeasureResult(&s_Sht40.Temp, &s_Sht40.Humidity);           //获取当前温湿度值 
            printf("\r\n=======================================\r\n");
            printf("\r\ntemperature is %d, humidity is %d\r\n", s_Sht40.Temp, s_Sht40.Humidity);
            printf("\r\n=======================================\r\n");
            if(s_Sht40.Temp >= s_Sht40.TempThreshold)
            {
                IoTGpioSetOutputVal(FAN_PIN,IOT_GPIO_VALUE1);                   //开启散热风扇
                IoTGpioSetOutputVal(FAN_POWER_PIN,IOT_GPIO_VALUE1);             //开启散热风扇电源
            }
            else
            {
                IoTGpioSetOutputVal(FAN_PIN,IOT_GPIO_VALUE0);                   //关闭散热风扇
                IoTGpioSetOutputVal(FAN_POWER_PIN,IOT_GPIO_VALUE0);             //关闭散热风扇电源
            }
        }
        sleep(1);   
    }
}


static void SoilHumiCtrl_Task(void *argument)
/******************************************************************************/  
{
    uint8_t pump_ctrl_state, pump_power_state;
    

    Soil_Init();                                            //硬件初始化
    s_Soil.HumiThreshold = 50;                              //默认阈值50
    
        while (1)
        {
            if(global_auto_control == 1){
                if(Measure_Soil_Humi(&s_Soil.Humidity) != 0)        //检测土壤湿度
                {
                    printf("[SoilHumiCtrl_Task] Soil humi measure false!!!\n");
                }
                
                if(s_Soil.Humidity >= s_Soil.HumiThreshold)         //判断当前湿度是否大于设置水泵开关阈值
                {
                    pump_ctrl_state  = WATER_PUMP_ON;
                    pump_power_state = WATER_PUMP_POWER_OFF;
                }
                else
                {
                    pump_ctrl_state  = WATER_PUMP_ON;
                    pump_power_state = WATER_PUMP_POWER_ON;
                }
                Water_Pump_Power_OnOff_Set(pump_power_state);       //设置水泵电源开关
                Water_Pump_OnOff_Set(pump_ctrl_state);              //设置水泵控制开关
                printf("\r\n=======================================\r\n");
                printf("\r\nSoil humi is %d , water pump state is %d\r\n",s_Soil.Humidity,pump_ctrl_state);
                printf("\r\n=======================================\r\n");
            }
            sleep(1);
        } 
}
// 在函数中创建新线程
static void at_exe_mqtt_test_cmd(void)
{
    osThreadAttr_t attr;
    attr.name = "mqtt_config_thread";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 4096;
    attr.priority = 36;
    // 在新线程中执⾏函数mqtt_test_thread
    if (osThreadNew((osThreadFunc_t)mqtt_test_thread, NULL, &attr) == NULL)
    {
        printf("[mqttExample] Falied to create LedTask!\n");
    }
    attr.name = "SHT40Task_thread";
    if (osThreadNew((osThreadFunc_t)SHT40Task, NULL, &attr) == NULL)
    {
        printf("[Sht40Example] Falied to create Sht40Task!\n");
    }
    attr.name = "SoilHumiCtrl_Task_thread";
    if (osThreadNew((osThreadFunc_t)SoilHumiCtrl_Task, NULL, &attr) == NULL)
    {
        printf("[SoilExample] Falied to create SoilTask!\n");
    }
    attr.name = "OLED_thread";
    if (osThreadNew((osThreadFunc_t)OledTask, NULL, &attr) == NULL)
    {
        printf("[OLEDExample] Falied to create OLEDTask!\n");
    }
    AT_RESPONSE_OK;
    //执行其他线程去了...
}
APP_FEATURE_INIT(at_exe_mqtt_test_cmd);
// SYS_RUN(at_exe_mqtt_test_cmd);