#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include <unistd.h>
#include "hi_wifi_api.h"
// #include "wifi_sta.h"
#include "ip_addr.h"
#include "netifapi.h"
#include "sockets.h"
#include "MQTTPacket.h"
#include "transport.h"
#include "iot_gpio.h"
#include <string.h>
#include "cJSON.h"
// #include "traffic_light.h"
#define HOST_ADDR "52.35.174.80"
int toStop = 0;
int mqtt_connect(void)
{
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    int rc = 0;
    int mysock = 0;
    unsigned char buf[200];
    int buflen = sizeof(buf);
    int msgid = 1;
    MQTTString topicString = MQTTString_initializer;
    int req_qos = 0;
    char payload[200];
    int payloadlen = strlen(payload);
    int len = 0;
    char *host = HOST_ADDR; // MQTT服务器的IP地址
    int port = 1883;
    mysock = transport_open(host, port);
    if (mysock < 0)
        return mysock;
    printf("Sending to hostname %s port %d\n", host, port);
    data.clientID.cstring = "64c79d99f3a8cf64b9c9291e_hi3861_farm_0_0_2023073112"; // 修改成⾃⼰名称，
    data.keepAliveInterval = 20;                                                   // ⼼跳时间
    data.cleansession = 1;
    data.username.cstring = "64c79d99f3a8cf64b9c9291e_hi3861_farm";
    data.password.cstring = "4cd6c602586b0c18ac398afa0c0355365e6cb7cbe12776b6b714dc862836cc83";
    len = MQTTSerialize_connect(buf, buflen, &data);
    rc = transport_sendPacketBuffer(mysock, buf, len);
    /* wait for connack */
    if (MQTTPacket_read(buf, buflen, transport_getdata) == CONNACK)
    {
        unsigned char sessionPresent, connack_rc;
        if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, buf, buflen) != 1 ||
            connack_rc != 0)
        {
            printf("Unable to connect, return code %d\n", connack_rc);
            goto exit;
        }
    }
    else
        goto exit;
    /* subscribe */
    topicString.cstring = "subtopic";
    len = MQTTSerialize_subscribe(buf, buflen, 0, msgid, 1, &topicString, &req_qos);
    rc = transport_sendPacketBuffer(mysock, buf, len);
    if (MQTTPacket_read(buf, buflen, transport_getdata) == SUBACK) /* wait for suback */
    {
        unsigned short submsgid;
        int subcount;
        int granted_qos;
        rc = MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, buf, buflen);
        if (granted_qos != 0)
        {
            printf("granted qos != 0, %d\n", granted_qos);
            goto exit;
        }
    }
    else
        goto exit;
    /* loop getting msgs on subscribed topic */

    while (!toStop)
    {
        /* transport_getdata() has a built-in 1 second timeout,
        your mileage will vary */
        if (MQTTPacket_read(buf, buflen, transport_getdata) == PUBLISH)
        {
            unsigned char dup;
            int qos;
            unsigned char retained;
            unsigned short msgid;
            int payloadlen_in;
            unsigned char *payload_in;
            int rc;
            rc = rc;
            MQTTString receivedTopic;
            rc = MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic,
                                         &payload_in, &payloadlen_in, buf, buflen);
            //-----------------------------------------------------mqtt_sub_callback-----------------------------------------------------------------
            if (payload_in == NULL){
                printf("[error] payload is NULL\n");
            }else{
                printf("[info] The payload_in is %s\n",payload_in);
            }
            
            cJSON* root = cJSON_Parse((const char *)payload_in);
            if(root){
                cJSON* json_service_id = cJSON_GetObjectItem(root, "service_id");
                if(json_service_id){
                    if(!strcmp(json_service_id->valuestring, "control")){
                        cJSON *json_command_name = cJSON_GetObjectItem(root, "command_name");
                        if(json_command_name){

                            // 接收风扇控制命令
                            if(!strcmp(json_command_name->valuestring, "fan")){
                                cJSON *json_value = cJSON_GetObjectItem(cJSON_GetObjectItem(root, "paras"), "value");
                                if(json_value){
                                    if(!strcmp(json_value->valuestring, "ON")){
                                        IoTGpioSetOutputVal(8,1);                   //开启散热风扇
                                        IoTGpioSetOutputVal(12,1);             //开启散热风扇电源
                                        printf("[info] fan is on.\n");
                                    }
                                    else if(!strcmp(json_value->valuestring, "OFF")){
                                        IoTGpioSetOutputVal(8,0);                   //关闭散热风扇
                                        IoTGpioSetOutputVal(12,0);             //关闭散热风扇电源
                                        printf("[info] fan is off.\n");
                                    }else{
                                        topicString.cstring = "error"; // 设置主题
                                        unsigned char * error_payload = "[error] The fan control command just in on or off,please check the paras.";
                                        int error_payloadlen = strlen(error_payload);
                                        len = MQTTSerialize_publish(buf, buflen, 0, 0, 0, 0, topicString, (unsigned char *)error_payload, error_payloadlen);
                                        rc = transport_sendPacketBuffer(mysock, buf, len);
                                    }
                                }
                                json_value = NULL;
                            }
                            //结束------------------------

                        }
                        json_command_name = NULL;
                    }
                }
                json_service_id = NULL;
            }
            cJSON_Delete(root);
            root = NULL; 
        }   
        topicString.cstring = "publish"; // 设置主题
        len = MQTTSerialize_publish(buf, buflen, 0, 0, 0, 0, topicString, (unsigned char *)payload, payloadlen);
        rc = transport_sendPacketBuffer(mysock, buf, len);
        usleep(100000);
        printf("/***************************/while 1\n");
    }
    printf("disconnecting\n");
    len = MQTTSerialize_disconnect(buf, buflen);
    rc = transport_sendPacketBuffer(mysock, buf, len);
exit:
    printf("mqtt exit!!!!\n");
    transport_close(mysock);
    rc = rc;
    return 0;
}

