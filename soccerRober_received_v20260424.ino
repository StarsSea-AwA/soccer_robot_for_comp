//足球機器人の小程序 v2025.7.7

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

int in1 = 2;
int in2 = 4;
int en1 = 3;

int in3 = 5;
int in4 = 7;
int en2 = 6;

int in5 = 8;
int in6 = 9;
int en3 = 10;

int in7 = 12;
int in8 = 13;
int en4 = 11;

unsigned long NoTimeLast[6] = { 0, 0, 0, 0, 0, 0 };

int isSetSpeed = 0;

int speedLog[2] = { 170, 200 };

int i1 = 1;

int isReverse = 0;

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

  SKIP = 1000,
};

enum TIMER_MODES {
  DURATION = 0,
  SET,
  RESET,
};

enum ERROR_CODES {
  SUCCESS = 0,
  //TIMER
  TIMER_OUT_OF_RANGE = 201,


};

//電機1，即右前方那個，isclw是0~1（Flase-True）是否順時針
void motor1(int isclw = 0, int speed = 255) {
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
void motor2(int isclw = 0, int speed = 255) {
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
void motor3(int isclw = 0, int speed = 255) {
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
  if (isReverse == 114) {
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

//計時器，需要擴展可以改“NoTimeLast”數組的數量
/*用法：Timer(計時器的序號，要做的事情（0~3）)
mode = 0時，該函數返回距離上次記錄起始時間的值(ms)
mode = 1時，記錄現在時間，記錄該計時器的起始值(ms)
mode = 2時，重置該計時器
*/
int Timer(int Notimer = 0, int mode = 0) {
  if (Notimer > 5) {
    return TIMER_OUT_OF_RANGE;
  }
  if (mode == 1) {
    NoTimeLast[Notimer] = millis();
    return SUCCESS;
  } else if (mode == 2) {
    NoTimeLast[Notimer] = 0;
    return SUCCESS;
  }
  if (NoTimeLast[Notimer] <= 0) {
    NoTimeLast[Notimer] = 0;
    return SUCCESS;
  }
  return int(millis() - NoTimeLast[Notimer]);
}

void motor4() {
  digitalWrite(in7, 1);
  digitalWrite(in8, 0);
  analogWrite(en4, 255);
  Timer(3, 1);
}

void motor4_stop() {
  digitalWrite(in7, 0);
  digitalWrite(in8, 0);
  analogWrite(en4, 0);
  Timer(3, 2);
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

  isReverse = EEPROM.read(1);
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
    case 114:
      return 1000;
    case 111:
      return 111;  //skip
    case 110:
      if (isReverse != 114) {
        EEPROM.write(1, 114);
        isReverse = 114;
      } else {
        EEPROM.write(1, 0);
        isReverse = 0;
      }
      return 110;
    case 115:
      /*speedLog[0] = 170;
      speedLog[1] = 200;
      rotatedSpeed_all = 170;
      rotatedSpeed_single = 200;*/
      if (EEPROM.read(7) == 1) {
        speedLog[0] = EEPROM.read(5);
        speedLog[1] = EEPROM.read(6);
      } else {
        speedLog[0] = 170;
        speedLog[1] = 200;
      }
      return 115;
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
      motors(-rotatedSpeed_all, -rotatedSpeed_all, -rotatedSpeed_all);
      return 10;
    case ANTI_CLOCKWISE:
      motors(rotatedSpeed_all, rotatedSpeed_all, rotatedSpeed_all);
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
    case 101:
      motors(130, 130, -200);
      return 101;
    default:
      motor_stop();
      motor4_stop();
      return 999;
  }
}


void loop() {

  if (Serial.available()) {
    Timer(1, 2);
    
    rotatedSpeed_single = speedLog[1];
    rotatedSpeed_all = speedLog[0];
    dataReceived = Serial.read();
    if (stateLog == 1000) {
      if (isSetSpeed != 1) {
        rotatedSpeed_all = dataReceived;
        speedLog[0] = dataReceived;
        EEPROM.write(5, dataReceived);
        isSetSpeed = 1;
        dataReceived = 114;
      } else {
        rotatedSpeed_single = dataReceived;
        speedLog[1] = dataReceived;
        EEPROM.write(6, dataReceived);
        if (EEPROM.read(7) != 1) {
          EEPROM.write(7, 1);
        }
        isSetSpeed = 0;
        dataReceived = 111;
      }
    }

    stateLog = commendSwitch(dataReceived);

    stateLoged = stateLog;
  } else {
    if (Timer(1) == 0) {
      Timer(1, 1);
    } else if ((stateLog >= 10) && (Timer(1) >= 1500) && (stateLog < 30)) {
      rotatedSpeed_single = 255;
      rotatedSpeed_all = 255;
    }
    stateLog = commendSwitch(dataReceived);
  }
}
