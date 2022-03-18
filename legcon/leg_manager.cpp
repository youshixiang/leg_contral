#include "leg_manager.h"
#include "Arduino.h"

#include <Arduino_JSON.h>

#include <ESP8266WiFi.h>
leg::leg()
{
  opty = leg_op_stop;
  motordrive(opty, 0);
  floor_rotate_count = 0;
  leg_staus_u8[u8_rotate_floor] = 0x00;
  // int n = digitalRead(4);
  // if (n == 1)
  // {
  //   leg_status = "open";
  //   leg_staus_u8[u8_status] = u8_suspend; //支腿状态为悬空
  // }
  // else
  // {
  //   leg_status = "close";
  //   leg_staus_u8[u8_status] = u8_close; //支腿状态为全收
  // }
  current = 0;
}

void leg::motordrive(int op, int pwm)
{
  if (op == leg_op_up)
  {
    digitalWrite(relay_ctr_pin, HIGH);
  }
  else if (op == leg_op_down)
  {
    digitalWrite(relay_ctr_pin, HIGH);
  }
  else if (op == leg_op_stop)
  {
    digitalWrite(relay_ctr_pin, LOW);
  }
  // lllSerial.println("motordrive:" + String(op));
  opty = op;
  d_pwm = pwm;
}; //电机驱动函数
void leg::pin_init()
{
  //继电器操作引脚初始化
  pinMode(relay_ctr_pin, OUTPUT);
  digitalWrite(relay_ctr_pin, LOW);
  //传感器引脚初始化
  pinMode(5, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  //电源控制转向引脚初始化
  pinMode(13, INPUT_PULLUP);
  //指示灯GPIO2设置为输出
  pinMode(15, OUTPUT);

}; //引脚初始化函数
void leg::sp_count()
{
  if (opty == leg_op_up)
  {
    rotate_count = rotate_count + 1; //举升圈数加1
                                     //电机旋转圈数达到最大值（举升到位）
    leg_staus_u8[u8_rotate] = rotate_count;
    if (rotate_count >= limit_higt_count)
    {

      leg_status = "full_open";
      leg_staus_u8[u8_status] = u8_full_open;
    }
    else if (leg_staus_u8[u8_status] == u8_close)
    {
      leg_status = "suspend";
      leg_staus_u8[u8_status] = u8_suspend;
    }
    if (leg_status.indexOf("floor") > -1)
    {
      floor_rotate_count++;
      leg_staus_u8[u8_rotate_floor]++;
    }
    if (down_rotate_count != 0)
    {
      down_rotate_count--;
      leg_staus_u8[4]--;
    }
  }
  else if (opty == leg_op_down)
  {

    rotate_count = rotate_count - 1; //回收圈数减1
    leg_staus_u8[u8_rotate] = rotate_count;

    if (rotate_count < limit_higt_count)
    {
      leg_status = "open";
      leg_staus_u8[u8_status] = u8_floor;
    }
    if (floor_rotate_count != 0)
    {
      floor_rotate_count--;
      leg_staus_u8[u8_rotate_floor]--;
    }
    down_rotate_count++;
    leg_staus_u8[4]++;
  }

  Serial.printf("rotate_count:");
  Serial.println(rotate_count);
  leg_status_save_flag = 1;
}; //转数据更新函数
void leg::limit_low()
{ //当是回收操作时，电机停止动作,并且电机旋转圈数归0，支腿状态修改为“close"
  rotate_count = 0;
  leg_staus_u8[u8_rotate] = 0;
  leg_staus_u8[u8_rotate_floor] = 0;
  leg_status = "close";
  leg_staus_u8[u8_status] = u8_close;
  leg_status_save_flag = 1;
  // if (opty == leg_op_down)
  // {
  //   motordrive(leg_op_stop, 0);
  // }
}; //回收限位函数
void leg::adc_read()
{
  int adc;
  adc = analogRead(A0);
  // Serial.print("adc:");
  // Serial.println(adc);
  // if (adc > 100)
  // {
  current = abs(adc * 1000 / 1024 * 4 - 1650) / 66 * 1000;
  // }
  // else
  // {
  //   current = 0;
  // }
  // Serial.print("current:");
  // Serial.println(current);
  //当有进行正在举升或回收时，更新电流数据
  if (opty == leg_op_up)
  {

    //电流大于落地电流门限时，判断支腿是否落地
    if (current > floor_current)
    {
      leg_status = "floor";
      leg_staus_u8[u8_status] = u8_floor;
    }
    else
    {
      leg_status = "suspend";
      leg_staus_u8[u8_status] = u8_suspend;
    }
    //电流超过门限，停止动作
  }
  else if (opty == leg_op_down)
  {
    if (leg_status.indexOf("close") == -1)
    {
      leg_status = "open";
      leg_staus_u8[u8_status] = u8_suspend;
    }
  }
  if ((opty == leg_op_up && current > up_current_max) || (opty == leg_op_down && current > down_current_max))
  {
    motordrive(leg_op_stop, 0);
  }
  // lllSerial.println(leg_status);
}; //电流数据读取
void leg::leg_para_set()
{
  floor_current = 800;
  // lllSerial.println(floor_current);
  up_current_max = 5000;
  // lllSerial.println(up_current_max);
  down_current_max = 3000;
  // lllSerial.println(up_current_max);
}; //设置支腿运行参数
void leg::leg_para_set(int f, int um, int dm)
{
  floor_current = f;
  up_current_max = um;
  down_current_max = dm;
}; //设置支腿运行参数

leg_send_para::leg_send_para(void)
{
  leg_json["STATUS"] = "LOSS";
}

/*udp发送数据转换*/
void leg_send_para::date_tran(){

    //  return _str[0]
};
void leg_send_para::date_json_up(int wifi_rssi, leg *leg, int leg_num)
{
  JSONVar temp_json; // json格式数据
  sq_num = leg_json["sq_num"];
  sq_num++;
  String strop;
  int i = leg->opty;
  switch (i)
  {
  case 0:
    strop = "stop";
    // pwm_values = 0;
    break;
  case 1:
    strop = "up";
    // pwm_values = 1000;
    break;
  case 2:
    strop = "down";
    // pwm_values = 1000;
    break;
  }
  // lllSerial.println("date_json_up,opty:"+String(i)+" is "+strop);
  leg_json["leg_num"] = leg_num;
  leg_json["sq_num"] = sq_num;
  leg_json["opty"] = strop; // opty 操作类型，停止为"stop"，举升为"up",回收为"down"
  // leg_json["leg_status"] = leg->leg_status; //支腿状态，"close"全部回收，"full_open"，全部伸出（举升到位），"suspend"悬空，"floor"落地支腿受力；
  // leg_json["current"] = leg->current;
  leg_json["floor_current"] = leg->floor_current;
  leg_json["up_current_max"] = leg->up_current_max;
  leg_json["down_current_max"] = leg->down_current_max;
  leg_json["rotate_count"] = leg->rotate_count;

  leg_json["RSSI"] = wifi_rssi;

  leg_json["CU"] = leg->current;
  leg_json["OP"] = strop;
  leg_json["LONG"] = leg->rotate_count * 5 / 10;
  leg_json["STATUS"] = leg->leg_status;
  leg_json["floor_rotate_count"] = leg->floor_rotate_count;
  leg_json["down_rotate_count"] = leg->down_rotate_count;
  // // lllSerial.println(leg_json["STATUS"]);

  // leg_ws_json[0] = leg_json["RSSI"];
  // leg_ws_json[1] = leg_json["CU"];
  // leg_ws_json[2] = leg_json["OP"];
  // leg_ws_json[3] = leg_json["LONG"];
  // leg_ws_json[4] = leg_json["STATUS"];

  JSONVar keys = temp_json.keys();
  // for (int i = 0; i < keys.length(); i++)
  // {
  //   String temp = String(leg_num);
  //   String temp1 = JSON.stringify(keys[i]);
  //   temp = temp1 + temp;
  //   const char *result = temp.c_str();
  //   leg_json[keys[i]] = temp_json[keys[i]];
  // }
};

bool leg_send_para::data_json_compare(JSONVar target_json)
{
  // JSONVar target_json = JSON.parse(target_data);
  JSONVar target_keys = target_json.keys();
  // JSONVar leg_json_keys = leg_json.keys();
  bool rt = false;

  // // lllSerial.println(1);
  //判断消息序列号，如果接收到的数据序列号比当前保存的值大或者当前保存的数据中没有这一支腿的数据，则更新数据

  if (!leg_json.hasOwnProperty("sq_num") && target_json.hasOwnProperty("sq_num")) //当前数据为空并且收到数据对应支腿数据不为空，更新数据
  {
    // // lllSerial.println(2);
    for (int i = 0; i < target_keys.length(); i++)
    {
      // // lllSerial.println(i);
      // // lllSerial.println(target_keys[i]);
      if (JSON.typeof(target_json[target_keys[i]]) == "number")
      {
        leg_json[target_keys[i]] = (int)(target_json[target_keys[i]]);
      }
      if (JSON.typeof(target_json[target_keys[i]]) == "string")
      {
        leg_json[target_keys[i]] = (const char *)(target_json[target_keys[i]]);
      }

      // // lllSerial.printf("leg_json[");
      // // lllSerial.printf(target_keys[i]);
      // // lllSerial.printf("]");
      // // lllSerial.println(leg_json[target_keys[i]]);
      // // lllSerial.printf("target_json[");
      // // lllSerial.printf(target_keys[i]);
      // // lllSerial.printf("]");
      // // lllSerial.println(target_json[target_keys[i]]);
    }
    // if (target_json.hasOwnProperty("sq_num"))
    // {
    //   rt = true;
    //   // // lllSerial.printf{"NULL,I:"};// lllSerial.printf{i};
    // }
  }
  else if (leg_json.hasOwnProperty("sq_num") && target_json.hasOwnProperty("sq_num")) //当前数据不为空并且收到数据对应支腿数据不为空，进行下判断
  {
    if ((int)target_json["sq_num"] > (int)leg_json["sq_num"]) //当前数据旧，收到数据新，更新数据
    {
      for (int i = 0; i < target_keys.length(); i++)
      {
        if (JSON.typeof(target_json[target_keys[i]]) == "number")
        {
          leg_json[target_keys[i]] = (int)(target_json[target_keys[i]]);
        }
        if (JSON.typeof(target_json[target_keys[i]]) == "string")
        {
          leg_json[target_keys[i]] = (const char *)(target_json[target_keys[i]]);
        }
        // // lllSerial.printf("leg_json[");
        // // lllSerial.printf(target_keys[i]);
        // // lllSerial.printf("]");
        // // lllSerial.println(leg_json[target_keys[i]]);
        // // lllSerial.printf("target_json[");
        // // lllSerial.printf(target_keys[i]);
        // // lllSerial.printf("]");
        // // lllSerial.println(target_json[target_keys[i]]);
      }
    }
    if ((int)target_json["sq_num"] < (int)leg_json["sq_num"]) //当前数据新，发送新数据
    {
      rt = true;
    }
  }
  else if (!target_json.hasOwnProperty("sq_num") && leg_json.hasOwnProperty("sq_num")) //当前有数据，接收没有数据，发送数据
  {
    rt = true;
  }
  return rt;
}
void leg_send_para::u8_updata(uint8_t *buffer)
{
  if (*buffer == u8_close)
  {
    leg_json["STATUS"] = "close";
  }
  else if (*buffer == u8_suspend)
  {
    leg_json["STATUS"] = "suspend";
  }
  else if (*buffer == u8_floor)
  {
    leg_json["STATUS"] = "floor";
  }
  else if (*buffer == u8_full_open)
  {
    leg_json["STATUS"] = "full_open";
  }
  *buffer++; //电流
  *buffer++; //举升圈数
  leg_json["rotate_count"] = *buffer;
  *buffer++; //落地圈数
  leg_json["floor_rotate_count"] = *buffer;
  *buffer++; //回收圈数
  leg_json["down_rotate_count"] = *buffer;
  *buffer++; //操作状态
  String strop;
  int i = *buffer;
  switch (i)
  {
  case 0:
    strop = "stop";
    // pwm_values = 0;
    break;
  case 1:
    strop = "up";
    // pwm_values = 1000;
    break;
  case 2:
    strop = "down";
    // pwm_values = 1000;
    break;
  }
  leg_json["OP"] = strop;
}

void leg::u8_updata()
{
  if (leg_staus_u8[u8_status] == u8_close)
  {
    leg_status = "close";
  }
  else if (leg_staus_u8[u8_status] == u8_suspend)
  {
    leg_status = "suspend";
  }
  else if (leg_staus_u8[u8_status] == u8_floor)
  {
    leg_status = "floor";
  }
  else if (leg_staus_u8[u8_status] == u8_full_open)
  {
    leg_status = "full_open";
  }
  if (leg_staus_u8[u8_rotate] > 170)
  {
    rotate_count = leg_staus_u8[u8_rotate] - 255;
  }
  else
  {
    rotate_count = leg_staus_u8[u8_rotate];
  }

  floor_rotate_count = leg_staus_u8[u8_rotate_floor];

  down_rotate_count = leg_staus_u8[u8_rotate_bk];
}