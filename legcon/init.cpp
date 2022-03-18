#include <ESP8266WiFi.h>
// #include <WiFiClient.h>
//#include <SPIFFS.h>
//#include <AsyncUDP_STM32.h>
#include <FS.h>
#include "Arduino.h"
#include "leg_manager.h"
#include "init.h"
void init_hand::initwifi(int leg_num)
{
  int ip_three = leg_num;
  wifi_server.mode(WIFI_AP_STA);
  String apName = "jffczt";
  if (leg_num == 0)
  {
    ip_three = 10;
    apName = ("LEG_" + (String)ESP.getChipId()); // 2 设置WIFI名称
  }
  const char *softAPName = apName.c_str();
  const char *password = "adminadmin";
  IPAddress softLocal(192, 168, 100, ip_three);
  IPAddress softSubnet(255, 255, 255, 0);
  IPAddress softGateway(192, 168, 100, ip_three);
  if (leg_num < 2)
  {
    wifi_server.softAPConfig(softLocal, softGateway, softSubnet);
    wifi_server.softAP(softAPName, password, 2, 0, 6); // 3创建wifi  名称 +密码 adminadmin
    IPAddress myIP = wifi_server.softAPIP();           // 4输出创建的WIFI IP地址
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    Serial.print("softAPName: "); // 5输出WIFI 名称
    Serial.println(apName);
  }

  // if (leg_num > 1)
  // {
  //   wifi_server.begin(softAPName, password);
  //   int connCount = 0;
  //   while (wifi_server.status() != WL_CONNECTED)
  //   {
  //     delay(500);
  //     Serial.println(connCount);
  //     if (connCount > 30)
  //     { //如果连续30次连不上，则跳出循环，执行后面的程序
  //       break;
  //       Serial.println(connCount);
  //     }
  //     connCount += 1;
  //     ESP.restart();
  //   }
  //   wifi_server.config(softLocal, softGateway, softSubnet, softGateway);
  //   IPAddress myIP = wifi_server.localIP(); // 4输出创建的WIFI IP地址
  //   Serial.print("STA IP address: ");
  //   Serial.println(myIP);
  // };
  // Serial.printf("WIFI MODE:");
  // Serial.println(wifi_server.getMode());
};

void init_hand::initlegpara(leg *leg_self, int *leg_num)
{

  // //检查文件是否存在
  // bool exist = SPIFFS.exists("/legpara.txt");
  // if (!exist)
  // {
  //   Serial.printf("legpara is not exist!\n");
  //   leg_self->leg_para_set();
  //   File f = SPIFFS.open("/legpara.txt", "w");
  //   if (f)
  //   {
  //     f.println(0);
  //     f.println(800);
  //     f.println(5000);
  //     f.println(3000);
  //     f.close();
  //   }
  //   else
  //   {
  //     // err_log+=static_cast<word>(micros());
  //     // err_log+=static_cast<word>("legpara.txt is create fail!\n");
  //   }
  // }
  // else
  // {
  //   File f = SPIFFS.open("/legpara.txt", "r");
  //   Serial.printf("legpara is  exist!\n");
  //   if (f)
  //   {
  //     Serial.printf("legpara is open!\n");
  //     *leg_num = atoi(f.readStringUntil('\n').c_str());
  //     leg_self->floor_current = atoi(f.readStringUntil('\n').c_str());
  //     leg_self->up_current_max = atoi(f.readStringUntil('\n').c_str());
  //     leg_self->down_current_max = atoi(f.readStringUntil('\n').c_str());
  //     f.close();
  //     // Serial.printf("legpara is close!\n");
  //     // Serial.printf("rotate_count:");
  //     // Serial.println(leg_self->rotate_count);
  //   }
  //   else
  //   {
  //     Serial.println("file failed to open..");
  //     // err_log+=static_cast<word>(micro());
  //     // err_log+=static_cast<word>("legpara.txt is open fail!\n");
  //   }
  // }

  // //检查文件是否存在,动脚状态保存
  // exist = SPIFFS.exists("/legstatus.txt");
  // if (!exist)
  // {
  //   Serial.printf("legstatus is not exist!\n");
  //   leg_self->leg_para_set();
  //   File f = SPIFFS.open("/legstatus.txt", "w");
  //   if (f)
  //   {
  //     f.println("close");
  //     f.println(0);
  //     f.println(0);
  //     f.println(0);
  //     f.close();
  //   }
  //   else
  //   {
  //     // err_log+=static_cast<word>(micros());
  //     // err_log+=static_cast<word>("legstatus.txt is create fail!\n");
  //   }
  // }
  // else
  // {
  //   File f = SPIFFS.open("/legstatus.txt", "r");
  //   Serial.printf("legstatus is  exist and opened!\n");
  //   if (f)
  //   {
  //     Serial.printf("legstatus is open!\n");
  //     // leg_self->leg_status = f.readStringUntil('\n');
  //     // leg_self->rotate_count = atoi(f.readStringUntil('\n').c_str());
  //     // leg_self->floor_rotate_count = atoi(f.readStringUntil('\n').c_str());
  //     // leg_self->down_rotate_count = atoi(f.readStringUntil('\n').c_str());
  //     f.readBytes(leg_self->leg_staus_u8,5);
  //     f.close();
  //     Serial.printf("rotate_count:");
  //     Serial.println(leg_self->leg_staus_u8[2]);
  //     // Serial.printf("legstatus is close!\n");
  //     // Serial.printf("rotate_count:");
  //     // Serial.println(leg_self->rotate_count);
  //   }
  //   else
  //   {
  //     Serial.println("file failed to open..");
  //     // err_log+=static_cast<word>(micro());
  //     // err_log+=static_cast<word>("legpara.txt is open fail!\n");
  //   }
  // }
};

void init_hand::savelegpara(leg *leg_self, int *leg_num)
{
  // // leg_self->leg_para_set();
  // File f = SPIFFS.open("/legstatus.txt", "w");
  
  // if (f)
  // {
  //   // Serial.printf("legstatus is  exist and opened!\n");
  //   // String str=leg_self->leg_status;
  //   // str+="\r\n";
  //   // str+=String(leg_self->rotate_count);
  //   // str+="\r\n";
  //   // str+=String(leg_self->floor_rotate_count);
  //   // str+="\r\n";
  //   // str+=String(leg_self->down_rotate_count);
  //   // Serial.println(str);
  //   // f.println(str);
  //   // f.println(leg_self->leg_status);
  //   // f.println(leg_self->rotate_count);
  //   // f.println(leg_self->floor_rotate_count);
  //   // f.println(leg_self->down_rotate_count);
  //   // Serial.println(f.readStringUntil('\n'));
  //   // Serial.println(f.readStringUntil('\n'));
  //   // Serial.println(f.readStringUntil('\n'));
  //   // Serial.println(f.readStringUntil('\n'));

  //   f.write(leg_self->leg_staus_u8,5);
  //   f.readBytes(leg_self->leg_staus_u8,5);
  //   f.close();
  //   // Serial.printf("legstatus is close!\n");
  //   Serial.printf("rotate_count:");
  //   Serial.println(leg_self->leg_staus_u8[2]);
  // }
  // else
  // {
  //   // err_log+=static_cast<word>(micros());
  //   // err_log+=static_cast<word>("legstatus.txt is create fail!\n");
  // }
}