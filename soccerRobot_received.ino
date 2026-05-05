//足球機器人の小程序 v2026.4.25

/*
為避免不便，該程序基於以下標準：
- in1 = 2, en1 =3, in2 = 4以此類推(in5-6和en3是8，9，10)
- 接綫方面，電機驅動OUTPUT位置單數位需連接電機上面的銅片
- motor1 在該程序中為右前方的電機（以兩個電機那面為前方）然後順時針分別是motor2,3如下：
   ——
m3    m1
 \    /
   m2
...
*/

#include <math.h>
#include <EEPROM.h>

int moveSpeed = 255;  //
int rotatedSpeed_all = 170;
int rotatedSpeed_single = 200;
int stateLog = 0;
int stateLoged = 0;
int dataReceived = 0;

uint8_t in1 = 2;
uint8_t in2 = 4;
uint8_t en1 = 3;

uint8_t in3 = 5;
uint8_t in4 = 7;
uint8_t en2 = 6;

uint8_t in5 = 8;
uint8_t in6 = 9;
uint8_t en3 = 10;

uint8_t in7 = 12;
uint8_t in8 = 13;
uint8_t en4 = 11;

unsigned long NoTimeLast[6] = { 0, 0, 0, 0, 0, 0 };

int isSetSpeed = 0;

int speedLog[2] = { 170, 200 };

int i1 = 1;

bool isReverse = false;

int motor1_status = 0;
int motor2_status = 0;
int motor3_status = 0;
int motor4_status = 0;

enum commends {
  STOP = 0,
  FRONT_LEFT,
  FORWARD,
  FRONT_RIGHT,
  BACK_RIGHT,
  BACKWARD,
  BACK_LEFT,

  ANTI_CLOCKWISE = 10,
  ANTI_CLOCKWISE_FRONT_LEFT,
  ANTI_CLOCKWISE_FORWARD,
  ANTI_CLOCKWISE_FRONT_RIGHT,
  ANTI_CLOCKWISE_BACK_RIGHT,
  ANTI_CLOCKWISE_BACKWARD,
  ANTI_CLOCKWISE_BACK_LEFT,

  CLOCKWISE = 20,
  CLOCKWISE_FRONT_LEFT,
  CLOCKWISE_FORWARD,
  CLOCKWISE_FRONT_RIGHT,
  CLOCKWISE_BACK_RIGHT,
  CLOCKWISE_BACKWARD,
  CLOCKWISE_BACK_LEFT,

  TEST = 101,
  REVERSE = 110,
  SET_SPEED = 114,
  SKIP = 1000,
};

enum TIMER_MODES {
  GET_DURATION = 0,
  START,
  RESET,
};

enum TIMER_USADEG {
  TIMEOUT = 0,//PLANNING
  ROTATE = 1,
  MOVEFOR = 2,

};

enum ERROR_CODES {
  SUCCESS = 0,
  UNEXPECTED_COMMAND,
  //TIMER
  TIMER_OUT_OF_RANGE = 1000,
  UNEXPECTED_MODE,
  //MOTORS
  SPEED_OUT_OF_RANGE = 2001,
  DEG_OUT_OF_RANGE = 2002,
  //
  TIME_OUT = 10086,
};


//電機1，即右前方那個，isclw是0~1（Flase-True）是否順時針
void motor1(bool isclw = 0, int speed = 255) {
  analogWrite(en1, speed);
  if (isclw == 1) {
    digitalWrite(in1, 0);
    digitalWrite(in2, 1);
  } else {
    digitalWrite(in1, 1);
    digitalWrite(in2, 0);
  }
}
//電機2，即後面那個，isclw是0~1（Flase-True）是否順時針
void motor2(bool isclw = 0, int speed = 255) {
  analogWrite(en2, speed);
  if (isclw == 1) {
    digitalWrite(in3, 0);
    digitalWrite(in4, 1);
  } else {
    digitalWrite(in3, 1);
    digitalWrite(in4, 0);
  }
}
//電機3，即左前方那個，isclw是0~1（Flase-True）是否順時針
void motor3(bool isclw = 0, int speed = 255) {
  analogWrite(en3, speed);
  if (isclw == 1) {
    digitalWrite(in5, 0);
    digitalWrite(in6, 1);
  } else {
    digitalWrite(in5, 1);
    digitalWrite(in6, 0);
  }
  
}



//看其他人是怎麽幹的，是的，一個函數就可以控制3個電機了（原理就是吧之前的三個拼接起來）
void motors(int speed1, int speed2, int speed3) {
  if (isReverse) {
    speed1 = -speed1;
    speed2 = -speed2;
    speed3 = -speed3;
  }
  motor1(int(speed1 >= 0), abs(speed1));
  motor2(int(speed2 >= 0), abs(speed2));
  motor3(int(speed3 >= 0), abs(speed3));
}
//讓電機停止的函數
void motor_stop() {
  analogWrite(en1, 0);
  analogWrite(en2, 0);
  analogWrite(en3, 0);
}

//deg 角度0~360,以正x轴为0，逆时针旋转，speed为0~255
int to_derection (float deg, float speed = 255){
  if (speed > 255){
    return SPEED_OUT_OF_RANGE;
  }
  if (deg > 360){
    return DEG_OUT_OF_RANGE;
  }
  float motor1_speed = cos((deg + 120.0) * PI / 180) * speed;
  float motor2_speed = cos((deg - 120.0) * PI / 180) * speed;
  float motor3_speed = cos((deg) * PI / 180) * speed;
  motors(int(motor1_speed),int(motor2_speed),int(motor3_speed));
  return SUCCESS;
}


struct timer{
  unsigned long time_last;
};
struct timer rotate_speed_up = {0};
struct timer timeout = {0};
struct timer turn_while_move_duration = {0};
int timer(struct timer *t, enum TIMER_MODES mode){
  switch (mode) {
    case RESET:
      t->time_last = millis();
      return SUCCESS;
    case GET_DURATION:
      return int(millis() - t->time_last);
    default:
      return UNEXPECTED_MODE;
  };
};

void motor4() {
  digitalWrite(in7, 1);
  digitalWrite(in8, 0);
  analogWrite(en4, 255);
  
  //timer(3, START);
}

void motor4_stop() {
  digitalWrite(in7, 0);
  digitalWrite(in8, 0);
  analogWrite(en4, 0);
  //Timer(3, RESET);
}

int set_speed() {
  rotatedSpeed_all = Serial.read();
  EEPROM.write(5,rotatedSpeed_all);
  rotatedSpeed_single = Serial.read();
  EEPROM.write(6,rotatedSpeed_single);
  return SUCCESS;
}

void setup() {
  Serial.begin(9600);  //誰偷偷刪了這個我搞死他 by keliang

  pinMode(en1, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);

  pinMode(en2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  pinMode(en3, OUTPUT);
  pinMode(in5, OUTPUT);
  pinMode(in6, OUTPUT);

  pinMode(in7, OUTPUT);
  pinMode(in8, OUTPUT);
  pinMode(en4, OUTPUT);

  digitalWrite(en1, 0);
  digitalWrite(in1, 0);
  digitalWrite(in2, 0);

  digitalWrite(en2, 0);
  digitalWrite(in3, 0);
  digitalWrite(in4, 0);

  digitalWrite(en3, 0);
  digitalWrite(in5, 0);
  digitalWrite(in6, 0);

  bool isReverse = (EEPROM.read(1)==0);
}

int get_num (int num_byte = 2){
  int final_num = 0; 
  for (int i = 1; i < num_byte; i++){
    timer(&timeout,START);
    while (Serial.available() < 1 && timer(&timeout,GET_DURATION) < 1000) {
      continue;
    }
    if (timer(&timeout, START) >= 1000){
      return TIME_OUT;
    }
    final_num += Serial.read();
  }
  return final_num;
}

float get_float (int float_byte = 2){
  float final_float = 0.0;
  for (int i = 1; i < float_byte; i++){
    timer(&timeout,START);
    while (Serial.available() < 1 && timer(&timeout,GET_DURATION) < 1000) {
      continue;
    }
    if (timer(&timeout,GET_DURATION) >= 1000){
      return TIME_OUT;
    }
    final_float += Serial.read();
  }
  return final_float;
}

/*
根據獲取的值執行對應指令
1000：調速度狀態，不執行指令
0   ：停車
1   ：向左前方平移
2   ：向前方平移
3   ：向右前方平移
4   ：向右後方平移
5   ：向後方平移
6   ：向左後方平移
10  ：逆時針（向左）旋轉
20  ：順時針（向右）旋轉
1*  ：在平移的同時逆時針旋轉（平移方向參見1~6）
2*  ：在平移的同時順時針旋轉
*/
int commendSwitch(int commend) {
  switch (commend) {
    case SET_SPEED:
      set_speed();
      return SET_SPEED;
    case REVERSE:
      if (isReverse) {
        EEPROM.write(1, 0);
        isReverse = false;
      } else {
        EEPROM.write(1, 1);
        isReverse = true;
      }
      return 110;
    case 115:
      if (EEPROM.read(7) == 1) {
        speedLog[0] = EEPROM.read(5);
        speedLog[1] = EEPROM.read(6);
        return SUCCESS;
      } else {
        speedLog[0] = 170;
        speedLog[1] = 200;
      }
      
    case 100:
      motor4();
      return 100;
    case 99:
      motor4_stop();
      return 99;
    case STOP:
      motor_stop();
      return 0;
    case FRONT_LEFT:
      motors(-255, 255, 0);
      return 1;
    case FORWARD:
      motors(-255, 0, 255);
      return 2;
    case FRONT_RIGHT:
      motors(0, -255, 255);
      return 3;
    case BACK_RIGHT:
      motors(255, -255, 0);
      return 4;
    case BACKWARD:
      motors(255, 0, -255);
      return 5;
    case BACK_LEFT:
      motors(0, 255, -255);
      return 6;
    case CLOCKWISE:
      motors(rotatedSpeed_all, rotatedSpeed_all, rotatedSpeed_all);
      return 10;
    case ANTI_CLOCKWISE:
      motors(-rotatedSpeed_all, -rotatedSpeed_all, -rotatedSpeed_all);
      return 20;
    case ANTI_CLOCKWISE_FRONT_LEFT:
      motors(-255, 255, -rotatedSpeed_single);
      return 11;
    case ANTI_CLOCKWISE_FORWARD:
      motors(-255, -rotatedSpeed_single, 255);
      return ANTI_CLOCKWISE_FRONT_RIGHT;
    case ANTI_CLOCKWISE_FRONT_RIGHT:
      motors(-rotatedSpeed_single, -255, 255);
      return 13;
    case ANTI_CLOCKWISE_BACK_RIGHT:
      motors(255, -255, -rotatedSpeed_single);
      return 14;
    case ANTI_CLOCKWISE_BACKWARD:
      motors(255, -rotatedSpeed_single, -255);
      return 15;
    case ANTI_CLOCKWISE_BACK_LEFT:
      motors(-rotatedSpeed_single, 255, -255);
      return 16;
    case CLOCKWISE_FRONT_LEFT:
      motors(-255, 255, rotatedSpeed_single);
      return 21;
    case CLOCKWISE_FORWARD:
      motors(-255, rotatedSpeed_single, 255);
      return 22;
    case CLOCKWISE_FRONT_RIGHT:
      motors(rotatedSpeed_single, -255, 255);
      return 23;
    case CLOCKWISE_BACK_RIGHT:
      motors(255, -255, rotatedSpeed_single);
      return 24;
    case CLOCKWISE_BACKWARD:
      motors(255, rotatedSpeed_single, -255);
      return 25;
    case CLOCKWISE_BACK_LEFT:
      motors(rotatedSpeed_single, 255, -255);
      return 26;
    case TEST:
      motors(130, 130, -200);
      return 101;
    default:
      motor_stop();
      motor4_stop();
      return UNEXPECTED_COMMAND;
  }
}

bool Test = false;
void test(){
  digitalWrite(5, 1);
  digitalWrite(7, 0);
  analogWrite(6,255);
}

void loop() {
  
  while (Test){
    test();
  }
  if (Serial.available()) {
    timer(&rotate_speed_up, RESET);
    
    rotatedSpeed_single = speedLog[1];
    rotatedSpeed_all = speedLog[0];
    dataReceived = Serial.read();

    stateLog = commendSwitch(dataReceived);

    if (stateLog > 10 && stateLog != 20 && stateLog < 30){
      timer(&turn_while_move_duration, RESET);
    }
/*
    if (
      !(stateLog > 10 && stateLog != 20 && stateLog < 30) 
      && (stateLoged > 10 && stateLoged != 20 && stateLoged < 30) 
      && (timer(turn_while_move_duration, GET_DURATION) > 500)
      && 
      ){
      int 
      
      switch (stateLoged) {
        case FRONT_LEFT:
          motor1()
      }
    }*/


    stateLoged = stateLog;
  } else {
    if (timer(&rotate_speed_up,GET_DURATION) == 0) {
      timer(&rotate_speed_up, RESET);
    } else if ((stateLog >= 10) && (timer(&rotate_speed_up, GET_DURATION) >= 1500) && (stateLog < 30)) {
      rotatedSpeed_single = 255;
      rotatedSpeed_all = 255;
    }
    //stateLog = commendSwitch(dataReceived);
  }
}
