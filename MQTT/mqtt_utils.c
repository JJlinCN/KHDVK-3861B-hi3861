#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include <unistd.h>
#include "hi_wifi_api.h"
// #include "wifi_sta.h"
#include "lwip/ip_addr.h"
#include "lwip/netifapi.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "MQTTPacket.h"
#include "transport.h"
#include "iot_gpio.h"
#include <string.h>
#include "cJSON.h"
#include <stdlib.h>

#include "sht40.h"
#include "Soil.h"
// #include "traffic_light.h"
extern global_auto_control;
extern TS_SOIL_PARAM s_Soil;
extern TS_SHT40_PARAM s_Sht40;
#define HOST_ADDR "broker.emqx.io" // 域名

char host_addr[64] = ""; // 定义一个全局变量存储域名对应的IP地址
int toStop = 0;
int mysock = 0;

int getipaddrinfo(const char *hostname, char *ipaddr, int ipaddrlen)
{
    struct addrinfo hints, *res;
    int errcode;
    char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    errcode = lwip_getaddrinfo(hostname, NULL, &hints, &res);
    if (errcode != 0)
    {
        printf("[error]Error in getaddrinfo\n");
        return -1;
    }

    void *addr;
    if (res->ai_family == AF_INET)
    {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
        addr = &(ipv4->sin_addr);
    }
    else
    {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)res->ai_addr;
        addr = &(ipv6->sin6_addr);
    }

    inet_ntop(res->ai_family, addr, ipstr, sizeof(ipstr));
    strncpy(ipaddr, ipstr, ipaddrlen);

    freeaddrinfo(res);

    return 0;
}

void set_host_addr(void)
{
    if (strlen(host_addr) == 0)
    {
        int rc = getipaddrinfo(HOST_ADDR, host_addr, sizeof(host_addr));
        if (rc != 0)
        {
            printf("Failed to get IP address for the domain.\n");
            return;
        }
    }
}

int transport_mqtt_read(unsigned char *buf, int buflen)
{
    return MQTTPacket_read(buf, buflen, transport_getdata);
}

int transport_mqtt_sendPacket(unsigned char *buf, int buflen)
{
    return transport_sendPacketBuffer(mysock, buf, buflen);
}

int mqtt_connect(void)
{
    toStop = 0;      // 保证mqtt_task函数正确执行
    set_host_addr(); // 先解析域名
    printf("[success] The %s ip addr is %s\n", HOST_ADDR, host_addr);

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    int rc = 0;
    unsigned char buf[200];
    int buflen = sizeof(buf);
    int msgid = 1;
    int len = 0;
    char *host = host_addr; // MQTT服务器的IP地址
    int port = 1883;
    mysock = transport_open(host, port);
    if (mysock < 0)
        return mysock;
    printf("Sending to hostname %s port %d\n", host, port);

    // 随机生成clientid
    char *client_id = malloc(8); // 分配足够的空间来存储clientid，这里假设最长为8个字符
    if (client_id == NULL)
    {
        printf("Failed to allocate memory for client id\n");
        return -1;
    }
    snprintf(client_id, 8, "%d", rand()); // 使用rand函数生成随机数作为clientid
    printf("The new client_id is %s\n", client_id);
    data.clientID.cstring = client_id; // 修改成⾃⼰名称，
    data.keepAliveInterval = 20;       // ⼼跳时间
    data.cleansession = 1;
    data.username.cstring = client_id;
    data.password.cstring = client_id;
    len = MQTTSerialize_connect(buf, buflen, &data);
    rc = transport_mqtt_sendPacket(buf, len);
    /* wait for connack */
    if (transport_mqtt_read(buf, buflen) == CONNACK)
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
    {
        printf("CONNACK failed\n");
        goto exit;
    }

    return 0;

exit:
    printf("mqtt exit !!!!\n");
    transport_close(mysock);
    return rc;
}

int mqtt_subscribe(const char *topic)
{
    int rc = 0;
    unsigned char buf[200];
    int buflen = sizeof(buf);
    int msgid = 1;
    int len = 0;
    MQTTString topicString = MQTTString_initializer;
    int req_qos = 0;

    /* subscribe */
    topicString.cstring = (char *)topic;
    len = MQTTSerialize_subscribe(buf, buflen, 0, msgid, 1, &topicString, &req_qos);
    rc = transport_mqtt_sendPacket(buf, len);
    if (transport_mqtt_read(buf, buflen) == SUBACK) /* wait for suback */
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
    {
        printf("SUBACK failed\n");
        goto exit;
    }

    return 0;

exit:
    printf("mqtt exit!!!!\n");
    transport_close(mysock);
    return rc;
}

int mqtt_unsubscribe(const char *topic)
{
    printf("[info] unsubscribe %s doing!\n", topic);
    int rc = 0;
    unsigned char buf[200];
    int buflen = sizeof(buf);
    int msgid = 1;
    int len = 0;
    MQTTString topicString = MQTTString_initializer;

    /* unsubscribe */
    topicString.cstring = (char *)topic;
    len = MQTTSerialize_unsubscribe(buf, buflen, 0, msgid, 1, &topicString);
    rc = transport_mqtt_sendPacket(buf, len);
    if (transport_mqtt_read(buf, buflen) == UNSUBACK) /* wait for unsuback */
    {
        unsigned short unsubmsgid;
        rc = MQTTDeserialize_unsuback(&unsubmsgid, buf, buflen);
    }
    else
    {
        printf("UNSUBACK failed\n");
        goto exit;
    }

    return 0;

exit:
    printf("mqtt_unsubscribe exit!!!!\n");
    transport_close(mysock);
    return rc;
}

int mqtt_publish(const char *topic, const char *payload)
{
    printf("[info] PUBLISH START!\n");
    int rc = 0;
    unsigned char buf[200];
    int buflen = sizeof(buf);
    int len = 0;
    MQTTString topicString = MQTTString_initializer;
    int payloadlen = strlen(payload);
    topicString.cstring = (char *)topic; // 设置主题
    len = MQTTSerialize_publish(buf, buflen, 0, 0, 0, 0, topicString, (unsigned char *)payload, payloadlen);
    rc = transport_mqtt_sendPacket(buf, len);

    return rc;
}

void handle_fan_control(const char *action)
{
    if (!strcmp(action, "ON"))
    {
        IoTGpioSetOutputVal(8, 1);  // 开启水泵
        IoTGpioSetOutputVal(12, 1); // 开启水泵电源
        printf("[info] fan is on.\n");
    }
    else if (!strcmp(action, "OFF"))
    {
        IoTGpioSetOutputVal(8, 0);  // 关闭水泵
        IoTGpioSetOutputVal(12, 0); // 关闭水泵电源
        printf("[info] fan is off.\n");
    }
    else
    {
        // 发送错误信息
        mqtt_publish("error", "[error] Invalid fan control action.");
    }
}

void handle_water_control(const char *action)
{
    if (!strcmp(action, "ON"))
    {
        IoTGpioSetOutputVal(5, 1);  // 开启水泵
        IoTGpioSetOutputVal(10, 1); // 开启水泵电源
        printf("[info] water is on.\n");
    }
    else if (!strcmp(action, "OFF"))
    {
        IoTGpioSetOutputVal(5, 0);  // 关闭水泵
        IoTGpioSetOutputVal(10, 0); // 关闭水泵电源
        printf("[info] water is off.\n");
    }
    else
    {
        // 发送错误信息
        mqtt_publish("error", "[error] Invalid water control action.");
    }
}

void handle_auto_control(const char *action)
{
    if (!strcmp(action, "ON"))
    {
        global_auto_control = 1; // 开启自动控制
        printf("[info] auto control is on.\n");
    }
    else if (!strcmp(action, "OFF"))
    {
        global_auto_control = 0; // 关闭自动控制
        printf("[info] auto control is off.\n");
    }
    else
    {
        // 发送错误信息
        mqtt_publish("error", "[error] Invalid auto control action.");
    }
}

void handle_Threshold_control(const cJSON *action)
{
    cJSON *humiThreshold = cJSON_GetObjectItem(action, "HumiThreshold");
    cJSON *tempThreshold = cJSON_GetObjectItem(action, "TempThreshold");

    if (humiThreshold && cJSON_IsNumber(humiThreshold))
    {
        s_Soil.HumiThreshold = (uint8_t)humiThreshold->valueint;
        s_Sht40.HumiThreshold = (uint8_t)humiThreshold->valueint;
        printf("HumiThreshold has changed to %d\n", s_Soil.HumiThreshold);
    }
    else
    {
        printf("HumiThreshold failed to be set!!!\n");
    }

    if (tempThreshold && cJSON_IsNumber(tempThreshold))
    {
        s_Sht40.TempThreshold = (uint8_t)tempThreshold->valueint;
        printf("TempThreshold has changed to %d\n", s_Sht40.TempThreshold);
    }
    else
    {
        printf("TempThreshold failed to be set!!!\n");
    }
}

void handle_mqtt_message(unsigned char *payload_in, int payloadlen_in)
{
    // 解析收到的消息
    payloadlen_in = payloadlen_in;
    cJSON *root = cJSON_Parse((const char *)payload_in);
    if (root)
    {
        cJSON *json_service_id = cJSON_GetObjectItem(root, "service_id");
        if (json_service_id && !strcmp(json_service_id->valuestring, "control"))
        {
            cJSON *json_command_name = cJSON_GetObjectItem(root, "command_name");
            if (json_command_name && cJSON_IsObject(json_command_name))
            {
                cJSON *json_water = cJSON_GetObjectItem(json_command_name, "water");
                if (json_water && cJSON_IsString(json_water))
                {
                    const char *water_action = json_water->valuestring;
                    handle_water_control(water_action);
                }

                cJSON *json_fan = cJSON_GetObjectItem(json_command_name, "fan");
                if (json_fan && cJSON_IsString(json_fan))
                {
                    const char *fan_action = json_fan->valuestring;
                    handle_fan_control(fan_action);
                }

                cJSON *json_auto = cJSON_GetObjectItem(json_command_name, "auto");
                if (json_auto && cJSON_IsString(json_auto))
                {
                    const char *auto_action = json_auto->valuestring;
                    handle_auto_control(auto_action);
                }
            }

            cJSON *json_set = cJSON_GetObjectItem(root, "set");
            if (json_set && cJSON_IsObject(json_set))
            {
                handle_Threshold_control(json_set);
            }
        }
        else
        {
            // 发送错误信息
            mqtt_publish("error", "[error] Unknown service_id.");
        }
        cJSON_Delete(root);
    }
}

void mqtt_disconnect(void)
{
    // toStop置1让while循环!toStop为0退出，退出mqtt_task任务后再断开连接，断开套接字
    toStop = 1;
    unsigned char buf[200];
    int buflen = sizeof(buf);
    int len = 0;
    len = MQTTSerialize_disconnect(buf, buflen);
    transport_mqtt_sendPacket(buf, len);
    printf("disconnecting\n");
    transport_close(mysock);
}

void mqtt_task(void)
{
    while (!toStop)
    {
        int temperature = s_Sht40.Temp;
        int humidity = s_Sht40.Humidity;
        cJSON *root = cJSON_CreateObject();
        // 创建温度字段并设置值
        cJSON_AddNumberToObject(root, "temperature", temperature);

        // 创建湿度字段并设置值
        cJSON_AddNumberToObject(root, "humidity", humidity);

        // 将根 JSON 对象转换为字符串
        char *jsonString = cJSON_PrintUnformatted(root);
        printf("%s\n",jsonString);
        mqtt_publish("sht40_sensor/data_recv", jsonString);
        unsigned char buf[200];
        int buflen = sizeof(buf);

        /* transport_getdata() has a built-in 1 second timeout,
        your mileage will vary */
        if (transport_mqtt_read(buf, buflen) == PUBLISH)
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

            // 处理接收到的消息
            printf("[info] start deal message:%s!\n", payload_in);
            handle_mqtt_message(payload_in, payloadlen_in);
        }
        usleep(1000000);
    }
    printf("the func mqtt_test() has done!\n");
}

void mqtt(void)
{
    int rc = mqtt_connect();
    if (rc != 0)
    {
        printf("Failed to connect to MQTT broker.\n");
        return;
    }
    // 订阅主题
    rc = mqtt_subscribe("subtopic");
    if (rc != 0)
    {
        printf("Failed to subscribe to topic.\n");
        mqtt_disconnect();
        return;
    }
    mqtt_publish("PUBLISH", "mqtt connected!\n");
    // 开始mqtt任务
    mqtt_task();
}