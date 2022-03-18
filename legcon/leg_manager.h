

#ifndef LEG_MANAGER_H_
#define LEG_MANAGER_H_

#include "Arduino.h"
#include <Arduino_JSON.h>
#include <ESP8266WiFi.h>
// #define  slp_pin  13           //电机制动引脚，高电平下PWM、DIR低电平制动，低电平自由
// #define  pwm_pin  12           //电机PWM引脚
// #define  dir_pin  14           //电机方向引脚，高电平正转，低电平反转
#define relay_ctr_pin 12 //继电器控制引脚，高电平电机动行，低电平电机停止
#define dir_pin 14
#define sp_pin 5             //转速传感器引脚
#define lt_pin 4             //限位传感器引脚
#define limit_higt_count 170 //举升高限位,最高850mm,螺距5mm，电机圈数170
#define leg_op_up 1
#define leg_op_down 2
#define leg_op_stop 0
#define u8_status 0
#define u8_current 1
#define u8_rotate 2
#define u8_rotate_floor 3
#define u8_rotate_bk 4
#define u8_close 0x00
#define u8_full_open 0x01
#define u8_suspend 0x02
#define u8_floor 0x03

class leg
{
public:
  leg();
  int opty; // opty 操作类型，停止为"stop"，举升为"up",回收为"down"
  int d_pwm;
  String leg_status;    //支腿状态，"close"全部回收，"full_open"，全部伸出（举升到位），"suspend"悬空，"floor"落地支腿受力；
  int current;          //电流值，单位ma
  int floor_current;    //支腿落地判断电流，单位ma
  int up_current_max;   //举升最大电流，单位ma
  int down_current_max; //回收最大电流，单位ma
  int rotate_count;     //电机旋转圈数计数器
  int floor_rotate_count;
  int down_rotate_count;
  int leg_status_save_flag;                 //状态保存标识
  uint8_t leg_staus_u8[5];                  // 0为支腿状态：0全收，1全出，2悬空，3落地；1为电流值，单位ma；2为支腿旋转圈数；3落地圈数;4回收圈数
  void motordrive(int op, int pwm);         //电机驱动函数
  void pin_init();                          //引脚初始化函数
  void sp_count();                          //转数据更新函数
  void limit_low();                         //回收限位函数
  void adc_read();                          //电流数据读取
  void leg_para_set();                      //设置支腿运行参数
  void leg_para_set(int f, int um, int dm); //设置支腿运行参数
  void u8_updata();                         //串口接收支腿参数同步
};

class leg_send_para
{
public:
  leg_send_para();
  JSONVar leg_json;                                        // json格式数据
  JSONVar leg_ws_json;                                     // json格式数据
  int leg_num;                                             //支腿编号，0为未配置
  int sq_num;                                              //发送序列号
  void date_tran();                                        //数据处理
  void date_json_up(int wifi_rssi, leg *leg, int leg_num); //数据处理
  bool data_json_compare(JSONVar target_json);
  void u8_updata(uint8_t *buffer); //串口接收支腿参数同步
private:
  String _str[9];
};

// class operation_cabin{

// };
#endif
