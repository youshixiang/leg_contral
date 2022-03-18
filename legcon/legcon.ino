/*
同步支腿控制程序
加了电流
 */
#include <ESP8266WiFi.h>
// #include <WiFiClient.h>
// #include <ESP8266WebServer.h>
// #include <ESP8266mDNS.h>
// #include <ESPAsyncUDP.h>
//#include <SPIFFS.h>
#include <FS.H>
// #include <WebSocketsServer.h>
// #include <Arduino_JSON.h>
#include <Ticker.h>
//#include<ModbusMaster232.h>
#include "leg_manager.h"
#include "init.h"
#include <SoftwareSerial.h>
#include <EspnowMeshBackend.h>
#include <TcpIpMeshBackend.h>
#include <TypeConversionFunctions.h>
#include <assert.h>
#include <espnow.h>
#define ESP8266WIFIMESH_DISABLE_COMPATIBILITY // Excludes redundant compatibility code. TODO: Should be used for new code until the compatibility code is removed with release 3.0.0 of the Arduino core.
namespace TypeCast = MeshTypeConversionFunctions;

constexpr char exampleMeshName[] PROGMEM = "MeshNode_";                       // The name of the mesh network. Used as prefix for the node SSID and to find other network nodes in the example networkFilter and broadcastFilter functions below.
constexpr char exampleWiFiPassword[] PROGMEM = "ChangeThisWiFiPassword_TODO"; // Note: " is an illegal character. The password has to be min 8 and max 64 characters long, otherwise an AP which uses it will not be found during scans.

// A custom encryption key is required when using encrypted ESP-NOW transmissions. There is always a default Kok set, but it can be replaced if desired.
// All ESP-NOW keys below must match in an encrypted connection pair for encrypted communication to be possible.
// Note that it is also possible to use Strings as key seeds instead of arrays.
uint8_t espnowEncryptedConnectionKey[16] = {0x33, 0x44, 0x33, 0x44, 0x33, 0x44, 0x33, 0x44, // This is the key for encrypting transmissions of encrypted connections.
                                            0x33, 0x44, 0x33, 0x44, 0x33, 0x44, 0x32, 0x11};
uint8_t espnowEncryptionKok[16] = {0x22, 0x44, 0x33, 0x44, 0x33, 0x44, 0x33, 0x44, // This is the key for encrypting the encrypted connection key.
                                   0x33, 0x44, 0x33, 0x44, 0x33, 0x44, 0x32, 0x33};
uint8_t espnowHashKey[16] = {0xEF, 0x44, 0x33, 0x0C, 0x33, 0x44, 0xFE, 0x44, // This is the secret key used for HMAC during encrypted connection requests.
                             0x33, 0x44, 0x33, 0xB0, 0x33, 0x44, 0x32, 0xAD};

unsigned int requestNumber = 0;
unsigned int responseNumber = 0;

const char broadcastMetadataDelimiter = 23; // 23 = End-of-Transmission-Block (ETB) control character in ASCII

String manageRequest(const String &request, MeshBackendBase &meshInstance);
TransmissionStatusType manageResponse(const String &response, MeshBackendBase &meshInstance);
void networkFilter(int numberOfNetworks, MeshBackendBase &meshInstance);
bool broadcastFilter(String &firstTransmission, EspnowMeshBackend &meshInstance);

/* Create the mesh node object */
EspnowMeshBackend espnowNode = EspnowMeshBackend(manageRequest, manageResponse, networkFilter, broadcastFilter, FPSTR(exampleWiFiPassword), espnowEncryptedConnectionKey, espnowHashKey, FPSTR(exampleMeshName), TypeCast::uint64ToString(ESP.getChipId()), true);

#define sp_pin 5        //转速传感器引脚
#define lt_pin 4        //限位传感器引脚
#define LED_BUILTIN 15 //指示灯输出引脚
Ticker esp_now_send_tk; //定时发送ESP-now
Ticker u8_Ticker;       //串口数据同通信定时器
Ticker pwmctr;          // pwm速率控
Ticker save_status_tk;  // pwm速率控
Ticker adc_read_tk;     // pwm速率控
int pwmctr_rotate;      // PWM速度控制圈数跟踪
int leg_num;            //支腿编号，0为未配置
int op_flag;            //支腿操作标识
int on_line_chick_flag; // udpsed flag
int adc = analogRead(A0);//电流值
int adc_max;//秒内的最大流值
int pin_opty = digitalRead(3); //操作引脚状态
int old_pin_opty = pin_opty;
int esp_now_send_flag; //发送ESP-now标识
int save_flag;
int sp_cont_flag;
int adc_read_flag;
int pin_D5 = digitalRead(5);
int pin_D4 = digitalRead(4);
int loop_count=0;
// uint8_t u8_code[];

const char *host = "esp8266-webupdate";

leg leg_self;

leg_send_para leg_send[5];

uint8_t u8_other_leg[4][6];

unsigned long previousMillis = 0; //毫秒时间记录
int PIN_D4 = HIGH;

int u8_op_flag = 0; //串口操作指令发送提示，0不用发送，1发送

init_hand inithand;

// SoftwareSerial u8_serial(2, 15);

ICACHE_RAM_ATTR void spcount()
{
  // leg_self.sp_count();
  sp_count();
  // lllSerial.println("spcount");
};
ICACHE_RAM_ATTR void limitlow()
{
  leg_self.limit_low();
};
// spcount=leg_self.sp_count;
// limitlow=leg_self.limit_low;

bool exampleTransmissionOutcomesUpdateHook(MeshBackendBase &meshInstance)
{
  // Currently this is exactly the same as the default hook, but you can modify it to alter the behaviour of attemptTransmission.

  (void)meshInstance; // This is useful to remove a "unused parameter" compiler warning. Does nothing else.

  return true;
}

bool exampleResponseTransmittedHook(bool transmissionSuccessful, const String &response, const uint8_t *recipientMac, uint32_t responseIndex, EspnowMeshBackend &meshInstance)
{
  // Currently this is exactly the same as the default hook, but you can modify it to alter the behaviour of sendEspnowResponses.

  (void)transmissionSuccessful; // This is useful to remove a "unused parameter" compiler warning. Does nothing else.
  (void)response;
  (void)recipientMac;
  (void)responseIndex;
  (void)meshInstance;

  return true;
}

void setup()
{

  // 串口波特率设置
  Serial.begin(115200);
  // Serial.swap(); // RX=GPIO13 TX=GPIO15
  // pin初始化
  leg_self.pin_init();
  // analogWriteFreq(1500000);
  // u8_serial.begin(115200);
  // u8_serial.enableIntTx(false);

  // logger = ss;
  // logger->println();
  // logger->printf("\n\nOn Software Serial for logging\n");
  //测速及限位中断
  // attachInterrupt(5, spcount, FALLING);  //转速传感器中断
  // attachInterrupt(4, limitlow, FALLING); //限位传感器中断
  //文件系统挂载
  bool ok = SPIFFS.begin();
  if (ok)
  {
    Serial.println("SPIFFS LOAD SUCC");
    initlegpara();
    leg_status_read();
    leg_self.u8_updata();
  }
  else
  {
    Serial.println("SPIFFS LOAD FAIL");
    if (SPIFFS.format())
    {
      ESP.restart();
      Serial.println("SPIFFS  FORMAT SUCC");
    }
  }
  //初始化状态，如果回收限位传感器接收到数据，则支脚状态更新为闭合
  int n = digitalRead(4);
  if (n != 1)
  {
    leg_self.leg_status = "close";
    leg_self.leg_staus_u8[u8_status] = u8_close; //支腿状态为全收
  }
  // leg_num = 2;
  // ESPNOW建立
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  /* Initialise the mesh node */
  espnowNode.begin();
  espnowNode.setEspnowEncryptedConnectionKey(espnowEncryptedConnectionKey);
  espnowNode.activateAP();
  espnowNode.setMessage(leg_self.leg_status);
  espnowNode.setTransmissionOutcomesUpdateHook(exampleTransmissionOutcomesUpdateHook);
  espnowNode.setResponseTransmittedHook(exampleResponseTransmittedHook);
  esp_now_send_tk.attach_ms(1000, []()
                            { esp_now_send_flag = 1; });
  save_status_tk.attach_ms(1000, []()
                           { if(leg_self.leg_status_save_flag == 1) {save_flag=1;
                          //  leg_self.leg_status_save_flag =0;
                     }; });

  // if (leg_num == 1)
  // {
  //   u8_Ticker.attach_ms(200, u8_master_read);
  // }
  // esp_now_send_tk.attach_ms(1000, we_se_flag_op);
}

void loop()
{
  loop_count=loop_count+1;
  //读取电源操作口状态，执行举升或回收操作
  pin_opty = digitalRead(13);              //操作引脚状态
  int new_d5 = digitalRead(5);
  int oldopty = op_flag;
  // Serial.println("llllllll");
  // Serial.println(n);
  //转动圈数计数
  if (new_d5 == 0 && pin_D5 == 1)
  {
    leg_self.sp_count();
    leg_self.leg_status_save_flag = 1;
  }
  pin_D5 = new_d5;

  op_flag = 0;
  if (pin_opty == 0)
  {
    if (leg_self.leg_status.indexOf("full_open") == -1) //支腿状态不在全出状态
    {
      op_flag = 1;
    }
  }
  else
  {
    if (leg_self.leg_status.indexOf("close") == -1) //支腿状态不在全收状态
    {
      op_flag = 2;
    }
  }
  uint8_t old_status = leg_self.leg_staus_u8[u8_status];
  op_flag = Position_Limitation_Protection(op_flag); //限位保护
  adc = (analogRead(A0)+adc)/2;
  if (abs(adc * 1000 / 1024 * 4 - 1650) >abs(adc_max * 1000 / 1024 * 4 - 1650) )
  {
    adc_max=adc;
  } 
  if (adc_read_flag == 1)                            //间隔100ms,读取电流值
  {
    
    if (leg_self.opty > 0 && leg_self.d_pwm > 0)
    {
      op_flag = overcurrent_protection(op_flag); //电流保护
    }
    adc_read_flag = 0;
  }
  //有举升回收操作，启动任务“pwmctr”。
  if (pin_opty != old_pin_opty)
  { //操作类型改变，重新计时定时器
    pwmctr.detach();
    old_pin_opty = pin_opty;
  }
  if (!pwmctr.active())
  {

    if (pin_opty == 0)
    {
      pwmctr.attach_ms(24000, pwmctr_fun); //启动转速同步定时器
      pwmctr_rotate = leg_self.rotate_count + 10;
    }
    else
    {
      pwmctr.attach_ms(23000, pwmctr_fun); //启动转速同步定时器
      pwmctr_rotate = leg_self.rotate_count - 10;
    }
  }

  op_carry_out();
  //指标灯控制
  ledctr();
  // Serial.println(PIN_D4);
  //保存状态
  if (save_flag == 1)
  {
    if (leg_self.leg_status_save_flag = 1)
    {
      leg_status_save();
      leg_self.leg_status_save_flag = 0;
    }
    save_flag = 0;
  }

  if (old_status != leg_self.leg_staus_u8[u8_status])
  {
    leg_self.leg_status_save_flag = 1;
  }

  // 当支腿状态为全出时，间1秒发送一次状态信息
  if (/* leg_self.leg_status.indexOf("full_open") != -1 && */ esp_now_send_flag == 1)
  {
    EspnowMeshBackend::performEspnowMaintenance();
    espnowNode.setMessage(leg_self.leg_status);
    espnowNode.attemptTransmission(espnowNode.getMessage());
    esp_now_send_flag = 0; 
    Serial.printf("loop_count:");
    Serial.print(loop_count);
    Serial.printf(",adc:");
    Serial.print(abs(adc * 1000 / 1024 * 4 - 1650) / 66 * 1000);
    Serial.printf(",adc_max:");
    Serial.println(abs(adc_max * 1000 / 1024 * 4 - 1650) / 66 * 1000);
    loop_count=0;
    adc_max=0;
  }
  // delay(1);
}

void op_carry_out()
{
  String opty;
  int pwm_values;

  // Serial.printf("digitalRead(5):");
  // Serial.println(digitalRead(lt_pin));
  int i = op_flag;

  int k = i;
  uint8_t old_status = leg_self.leg_staus_u8[u8_status];
  uint8_t old_rotate = leg_self.leg_staus_u8[u8_rotate];
  // pwm输出值控制
  if (i == 0) //如果停止操作，则PWM归零
  {
    pwm_values = 0;
  }
  else if (i == 1) //有举升或回收操作，则判断PWM值
  {
    if (pwmctr_rotate > leg_self.rotate_count)
    {                    //同步控制高速条件满足，或支腿没有落地
      pwm_values = 1023; //同步控制高速PWM值
    }
    else
    {
      pwm_values = 0; //同步控制低速PWM值
      i = 0;
    }
  }
  else if (i == 2) //有举升或回收操作，则判断PWM值
  {
    if (pwmctr_rotate < leg_self.rotate_count)
    {                    //同步控制高速条件满足，或支腿没有落地
      pwm_values = 1023; //同步控制高速PWM值
    }
    else
    {
      pwm_values = 0; //同步控制低速PWM值
      i = 0;
    }
  }
  if (pwm_values == 1023 && !adc_read_tk.active())
  {
    adc_read_tk.attach_ms(100, []()
                          { adc_read_flag = 1; });
  }
  else if (pwm_values == 0)
  {
    adc_read_tk.detach();
  }

  //参数改保存文件
  if (old_status != leg_self.leg_staus_u8[u8_status] || old_rotate != leg_self.leg_staus_u8[u8_rotate])
  {
    leg_self.leg_status_save_flag = 1;
  }

  // Serial.println(pwm_values);
  if (i != leg_self.opty || pwm_values != leg_self.d_pwm)
  {
    leg_self.motordrive(i, pwm_values);
  }
}

// pwm调速，转速控制
void pwmctr_fun()
{
  if (op_flag == 1)
  {
    pwmctr_rotate = pwmctr_rotate + 10;
  }
  else
  {
    pwmctr_rotate = pwmctr_rotate - 10;
  } //达到时间后，圈数同步，PWM值可调为1000，高速运行
  // Serial.print("Time:");
  // prints time since program started
  //  Serial.println( millis());
}

//限位保护函数
int Position_Limitation_Protection(int opty)
{
  int rop = opty;
  if (opty == 1)
  {
    if (leg_self.leg_status.indexOf("full_open") > -1)
    { //本支腿状态全出，不可进行举升操作
      rop = 0;
    }
  }
  else if (opty == 2)
  {
    if (digitalRead(lt_pin) == 0) //支腿闭合，到达回收限位点，不可再继续回收；
    {
     
      rop = 0;
      leg_self.rotate_count = 0;
      leg_self.leg_staus_u8[u8_rotate] = 0;
      leg_self.leg_staus_u8[u8_rotate_floor] = 0;
      leg_self.leg_status = "close";
      leg_self.leg_staus_u8[u8_status] = u8_close;
      leg_self.leg_status_save_flag = 1;
    }
    if (leg_self.leg_status.indexOf("close") > -1)
    {
      rop = 0;
    }
  }
  else
  {
    rop = 0;
    // lllSerial.println("Position_Limitation_Protection input 0." + String(leg_num) + "is order!");
  }
  if (rop == 0 && opty > 0)
  {
    op_flag = rop;
    u8_op_flag = 1;
  }
  return rop;
}
//过流保护
int overcurrent_protection(int opty)
{
  int rop = opty;
  int current;

  current = abs(adc * 1000 / 1024 * 4 - 1650) / 66 * 1000;
  if (opty == 1 && current > 0)
  {
    if (current < leg_self.floor_current)
    {
      leg_self.leg_status = "suspend";
      leg_self.leg_staus_u8[u8_status] = u8_suspend;
    }
    else if (current <= leg_self.up_current_max)
    {
      leg_self.leg_status = "floor";
      leg_self.leg_staus_u8[u8_status] = u8_floor;
    }
    else
    {
      rop = 0;
      leg_self.leg_status = "full_open";
      leg_self.leg_staus_u8[u8_status] = u8_full_open;
      // leg_op_flag_up("stop"); //通知其他支腿停止
      // lllSerial.println("LEG is over up cerrent,stop op!leg" + String(leg_num) + "is order!");
      // alarm
    }
  }

  else if (opty == 2 && current > 0)
  {
    if (current > leg_self.down_current_max)
    {
      rop = 0;
      leg_self.leg_status = "close";
      leg_self.leg_staus_u8[u8_status] = u8_close;
      // leg_op_flag_up("stop"); //通知其他支腿停止
      // lllSerial.println("LEG is over down cerrent,stop op!leg" + String(leg_num) + "is order!");
      // alarm
    }
  }

  if (rop == 0 && opty > 0)
  {
    op_flag = rop;
    u8_op_flag = 1;
  }
  return rop;
}
void leg_status_save()
{

  File f = SPIFFS.open("/legstatus.txt", "w+");
  if (f)
  {
    f.write(leg_self.leg_staus_u8, 5);
    f.readBytes(leg_self.leg_staus_u8, 5);
    f.close();
    // Serial.printf("legstatus is close!\n");
    Serial.printf("rotate_count:");
    Serial.println(leg_self.leg_staus_u8[u8_rotate]);
    // Serial.println();
  }
}
void leg_status_read()
{

  bool exist = SPIFFS.exists("/legstatus.txt");
  if (exist)
  {
    File f = SPIFFS.open("/legstatus.txt", "r");
    Serial.printf("legstatus is  exist and opened!\n");
    if (f)
    {
      Serial.printf("legstatus is open!\n");
      f.readBytes(leg_self.leg_staus_u8, 5);
      f.close();
      Serial.printf("leg_self.leg_staus_u8[2]:");
      Serial.println(leg_self.leg_staus_u8[2]);
    }
  }
}
void initlegpara()
{

  //检查文件是否存在
  bool exist = SPIFFS.exists("/legpara.txt");
  if (!exist)
  {
    Serial.printf("legpara is not exist!\n");
    leg_self.leg_para_set();
    File f = SPIFFS.open("/legpara.txt", "w");
    if (f)
    {
      f.println(0);
      f.println(800);
      f.println(8000);
      f.println(8000);
      f.close();
    }
  }
  else
  {
    File f = SPIFFS.open("/legpara.txt", "r");
    Serial.printf("legpara is  exist!\n");
    if (f)
    {
      Serial.printf("legpara is open!\n");
      leg_num = atoi(f.readStringUntil('\n').c_str());
      leg_self.floor_current = atoi(f.readStringUntil('\n').c_str());
      leg_self.up_current_max = atoi(f.readStringUntil('\n').c_str());
      leg_self.down_current_max = atoi(f.readStringUntil('\n').c_str());
      f.close();
    }
  }
}

String manageRequest(const String &request, MeshBackendBase &meshInstance)
{
  // To get the actual class of the polymorphic meshInstance, do as follows (meshBackendCast replaces dynamic_cast since RTTI is disabled)
  Serial.print(F("Request received: "));
  if (request.indexOf("full_open") > -1 && op_flag == 1 )
  {
    leg_self.leg_status = "full_open";
    leg_self.leg_staus_u8[u8_status] = u8_full_open;
  }
  Serial.println(request);

  return (leg_self.leg_status);
}

TransmissionStatusType manageResponse(const String &response, MeshBackendBase &meshInstance)
{
  TransmissionStatusType statusCode = TransmissionStatusType::TRANSMISSION_COMPLETE;

  /* Print out received message */
  // Only show first 100 characters because printing a large String takes a lot of time, which is a bad thing for a callback function.
  // If you need to print the whole String it is better to store it and print it in the loop() later.
  // Note that response.substring will not work as expected if the String contains null values as data.
  Serial.print(F("Response received: "));
  if (response.indexOf("full_open") > -1 && op_flag == 1 )
  {
    leg_self.leg_status = "full_open";
    leg_self.leg_staus_u8[u8_status] = u8_full_open;
  }
  Serial.println(response);
  return statusCode;
}

void networkFilter(int numberOfNetworks, MeshBackendBase &meshInstance)
{
  // Note that the network index of a given node may change whenever a new scan is done.
  for (int networkIndex = 0; networkIndex < numberOfNetworks; ++networkIndex)
  {
    String currentSSID = WiFi.SSID(networkIndex);
    int meshNameIndex = currentSSID.indexOf(meshInstance.getMeshName());

    /* Connect to any _suitable_ APs which contain meshInstance.getMeshName() */
    if (meshNameIndex >= 0)
    {
      uint64_t targetNodeID = TypeCast::stringToUint64(currentSSID.substring(meshNameIndex + meshInstance.getMeshName().length()));

      if (targetNodeID < TypeCast::stringToUint64(meshInstance.getNodeID()))
      {
        if (EspnowMeshBackend *espnowInstance = TypeCast::meshBackendCast<EspnowMeshBackend *>(&meshInstance))
        {
          espnowInstance->connectionQueue().emplace_back(networkIndex);
        }
        else if (TcpIpMeshBackend *tcpIpInstance = TypeCast::meshBackendCast<TcpIpMeshBackend *>(&meshInstance))
        {
          tcpIpInstance->connectionQueue().emplace_back(networkIndex);
        }
        else
        {
          Serial.println(F("Invalid mesh backend!"));
        }
      }
    }
  }
}

bool broadcastFilter(String &firstTransmission, EspnowMeshBackend &meshInstance)
{
  // This example broadcastFilter will accept a transmission if it contains the broadcastMetadataDelimiter
  // and as metaData either no targetMeshName or a targetMeshName that matches the MeshName of meshInstance.

  int32_t metadataEndIndex = firstTransmission.indexOf(broadcastMetadataDelimiter);

  if (metadataEndIndex == -1)
  {
    return false; // broadcastMetadataDelimiter not found
  }

  String targetMeshName = firstTransmission.substring(0, metadataEndIndex);

  if (!targetMeshName.isEmpty() && meshInstance.getMeshName() != targetMeshName)
  {
    return false; // Broadcast is for another mesh network
  }
  else
  {
    // Remove metadata from message and mark as accepted broadcast.
    // Note that when you modify firstTransmission it is best to avoid using substring or other String methods that rely on null values for String length determination.
    // Otherwise your broadcasts cannot include null values in the message bytes.
    firstTransmission.remove(0, metadataEndIndex + 1);
    return true;
  }
}

//检查文件是否存在,动脚状态保存
void ledctr()
{
  unsigned long currentMillis = millis(); //读取当前时间

  // Serial.println(op_flag);
  // Serial.println(oldopty);
  // Serial.println(leg_self.leg_status);
  if (op_flag == 1)
  {
    const long interval = 500; //时间间隔 举升操作快闪
    if (currentMillis - previousMillis >= interval)
    {
      if (PIN_D4 == LOW)
      {
        PIN_D4 = HIGH;
      }
      else
      {
        PIN_D4 = LOW;
      }
      previousMillis = currentMillis;
    }
  }
  else if (op_flag == 2)
  {
    const long interval = 1000; //时间间隔 举升操作慢闪
    if (currentMillis - previousMillis >= interval)
    {
      if (PIN_D4 == LOW)
      {
        PIN_D4 = HIGH;
      }
      else
      {
        PIN_D4 = LOW;
      }
      previousMillis = currentMillis;
    }
  }
  else
  {
    if (leg_self.leg_status.indexOf("close") > -1)
    {
      PIN_D4 = HIGH; //回收到位灭灯
    }
    else
    {
      PIN_D4 = LOW; //举升到位亮灯
    }
  }
  digitalWrite(LED_BUILTIN, PIN_D4);
}

void sp_count()
{
  sp_cont_flag = 1;
  leg_self.leg_status_save_flag = 1;
}; //转数据更新函数