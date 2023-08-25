/******************************************************************************\
                                 Includes
\******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "../Bluetooth/include/ly_evk_config.h"
//使用oled
#include "oled.h"

//使用头文件导入结构体定义共享，直接在本文件内定义结构体会导致重定义
#include "Soil.h"
#include "sht40.h"
/******************************************************************************\
                             Variables definitions
\******************************************************************************/
//使用main.c定义的变量，便于消息传递，代替消息队列
extern TS_SOIL_PARAM s_Soil;
extern TS_ADC_PARAM  s_Soil_Adc;
extern TS_SHT40_PARAM s_Sht40;

/******************************************************************************\
                             Functions definitions
\******************************************************************************/
/*
* 函数名称 : OledTask
* 功能描述 : oled显示任务
* 参    数 : arg - 任务参数
* 返回值   : 空
* 示    例 : OledTask(&argument);
*/
/******************************************************************************/ 
//不能使用static修饰，方便外部main.c文件extern使用
void OledTask(void *arg)
/******************************************************************************/ 
{
    
    (void)arg;
    char str[128]= {0};
    int n;
    bool updata_flag = true;
    static bool startup_flag = true;
    uint8_t data_num = 0;
    uint8_t y_num = 0;
    
    OLED_Init();
    OLED_Clear(); 
	OLED_Refresh_Gram();	 			
    while(1)
    {
        usleep(1000*100);
        OLED_Clear();                                                   //清理显示缓存
        data_num = 0;
        y_num = 0;
        updata_flag = false;
        static uint8_t soil_humi = 255;
        static uint8_t soil_humiThreshold = 255;
        data_num += 2;
        if(soil_humi != s_Soil.Humidity || soil_humiThreshold != s_Soil.HumiThreshold)  //判读数据是否发生变化
        {
            updata_flag = true;
        } 
        soil_humi = s_Soil.Humidity;                                    //获取土壤湿度
        soil_humiThreshold = s_Soil.HumiThreshold;          //获取水泵开启阈值
        //刷新显示缓存
        sprintf(str, "Soilhumi:%d%s", soil_humi,"%");               
        OLED_ShowString(0,((y_num++)*16),str,16);
        sprintf(str, "Threshold:%d%s", soil_humiThreshold,"%");
        OLED_ShowString(0,((y_num++)*16),str,16);
        static uint8_t humi = 255;
        static uint8_t temp = 255;
        data_num += 2;
        if(temp != s_Sht40.Temp || humi != s_Sht40.Humidity)    //判读数据是否发生变化
        {
            updata_flag = true;
        }
        temp = s_Sht40.Temp;                            //获取环境温度
        humi = s_Sht40.Humidity;                        //获取环境湿度
        //刷新显示缓存
        sprintf(str, "temp:%d", temp);
        OLED_ShowString(0,((y_num++)*16),str,16);
        sprintf(str, "humi:%d%s", humi,"%");
        OLED_ShowString(0,((y_num++)*16),str,16);
        if(updata_flag == true || startup_flag == true)
        {
            startup_flag = false;
            OLED_Refresh_Gram();                                            //更新显示
        }
        
    }
    
}
/******************************* End of File (C) ******************************/