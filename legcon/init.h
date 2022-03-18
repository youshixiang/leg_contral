


#ifndef INIT_H_
#define INIT_H_
#include <ESP8266WiFi.h>
// #include <WiFiClient.h>
//#include <AsyncUDP_STM32.h>
#include "Arduino.h"
#include "leg_manager.h"
#include <FS.h>
class init_hand {
    public:
    ESP8266WiFiClass wifi_server;

    void initwifi(int leg_num);
    void initlegpara(leg *leg_self,int *leg_num);
    void savelegpara(leg *leg_self,int *leg_num);
};


#endif
