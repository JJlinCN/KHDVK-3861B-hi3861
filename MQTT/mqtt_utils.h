#ifndef MQTT_UTILS_H
#include "cJSON.h"

int getipaddrinfo(const char* hostname, char* ipaddr, int ipaddrlen);
void set_host_addr(void);
int transport_mqtt_read(unsigned char* buf, int buflen);
int transport_mqtt_sendPacket(unsigned char* buf, int buflen);
int mqtt_connect(void);
int mqtt_subscribe(const char* topic);
int mqtt_unsubscribe(const char* topic);
int mqtt_publish(const char* topic, const char* payload);
void handle_fan_control(const char* action);
void handle_water_control(const char* action);
void handle_auto_control(const char* action);
void handle_Threshold_control(const cJSON* action);
void handle_mqtt_message(unsigned char* payload_in, int payloadlen_in);
void mqtt_disconnect(void);
void mqtt_task(void);
void mqtt(void);

#endif