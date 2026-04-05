#include <Arduino.h>
#include <HardwareSerial.h>

#define LED 13
#define PWMA 2
#define AIN1 12
#define AIN2 4
#define PWMB 16
#define BIN1 14
#define BIN2 15
#define STBY 25
#define Voltage A0

HardwareSerial MySerial(1); // UART1---OpenMv串口

int PwmA, PwmB;
double V;

volatile uint32_t Output = 0;   // Openmv传入速度调整值
volatile uint32_t SHI_Flag = 0; // 十字型路口标志位
volatile uint32_t T_Flag = 0;   // T形路口标志位
volatile uint32_t Left_Flag = 0;
volatile uint32_t Right_Flag = 0;
volatile uint32_t K_aim = 0;    // 目标库号
volatile uint32_t A_Flag = 0;   // 识别左边的库号
volatile uint32_t B_Flag = 0;   // 识别右边的库号
volatile uint32_t K_Left = 0;   // 节点左边的库号（暂存）
volatile uint32_t K_Right = 0;  // 节点右边的库号（暂存）
volatile uint32_t mode = 1;     // 摄像头模式  1：数字识别  2：巡线
volatile uint32_t Lcnt = 0;     // 左转计数器
volatile uint32_t Rcnt = 0;     // 右转计数器
volatile uint32_t Spin_cnt = 1; // 旋转计数器
volatile uint32_t Return_Num = 0;
volatile uint32_t mydashcnt = 0;

void task_USART_OpenMV(void *pt)
{
  String command;
  MySerial.begin(115200, SERIAL_8N1, 26, 27); // RX-26 TX-27

  while (1)
  {
    while (MySerial.available())
    {
      command += char(MySerial.read());
    }
    int Output_placeinstring = 0;
    int SHI_placeinstring = 0;
    int T_placeinstring = 0;
    int Left_placeinstring = 0;
    int Right_placeinstring = 0;
    int A_placeinstring = 0;
    int B_placeinstring = 0;
    String Output_command;
    String SHI_command;
    String T_command;
    String Left_command;
    String Right_command;
    String A_command;
    String B_command;

    if (command.length() > 0)
    {
      Output_placeinstring = command.indexOf('O');
      SHI_placeinstring = command.indexOf('S');
      T_placeinstring = command.indexOf('T');
      Left_placeinstring = command.indexOf('a');
      Right_placeinstring = command.indexOf('b');
      A_placeinstring = command.indexOf('A');
      B_placeinstring = command.indexOf('B');

      Output_command = command.substring(Output_placeinstring + 1, SHI_placeinstring);
      SHI_command = command.substring(SHI_placeinstring + 1, T_placeinstring);
      T_command = command.substring(T_placeinstring + 1, Left_placeinstring);
      Left_command = command.substring(Left_placeinstring + 1, Right_placeinstring);
      Right_command = command.substring(Right_placeinstring + 1, A_placeinstring);
      A_command = command.substring(A_placeinstring + 1, B_placeinstring);
      B_command = command.substring(B_placeinstring + 1);

      Output = Output_command.toInt();
      SHI_Flag = SHI_command.toInt();
      T_Flag = T_command.toInt();
      Left_Flag = Left_command.toInt();
      Right_Flag = Right_command.toInt();
      A_Flag = A_command.toInt();
      B_Flag = B_command.toInt();

      Serial.print("Output");
      Serial.println(Output);
      Serial.print("SHI");
      Serial.println(SHI_Flag);
      Serial.print("T");
      Serial.println(T_Flag);
      Serial.print("L");
      Serial.println(Left_Flag);
      Serial.print("R");
      Serial.println(Right_Flag);
      Serial.print("A");
      Serial.println(A_Flag);
      Serial.print("B");
      Serial.println(B_Flag);

      command = "";
    }
  }
}

void task_sending_OpenMV(void *pt)
{
  while (1)
  {
    MySerial.write(mode);
    vTaskDelay(100);
  }
}

void motor_init()
{

  pinMode(AIN1, OUTPUT); // 控制电机A的方向，(AIN1, AIN2)=(1, 0)为正转，(AIN1, AIN2)=(0, 1)为反转
  pinMode(AIN2, OUTPUT);
  pinMode(BIN1, OUTPUT); // 控制电机B的方向，(BIN1, BIN2)=(0, 1)为正转，(BIN1, BIN2)=(1, 0)为反转
  pinMode(BIN2, OUTPUT);
  pinMode(PWMA, OUTPUT); // A电机PWM
  pinMode(PWMB, OUTPUT); // B电机PWM
  pinMode(STBY, OUTPUT); // TB6612FNG使能, 置0则所有电机停止, 置1才允许控制电机

  // 初始化TB6612电机驱动模块
  digitalWrite(AIN1, 1);
  digitalWrite(AIN2, 0);
  digitalWrite(BIN1, 1);
  digitalWrite(BIN2, 0);
  digitalWrite(STBY, 1);
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
}

/**************************************************************************
函数功能：设置指定电机转速
入口参数：指定电机motor，motor=1（2）代表电机A（B）； 指定转速pwm，大小范围为0~255，代表停转和满速
返回  值：无
**************************************************************************/
void SetPWM(int motor, int pwm)
{
  if (motor == 1 && pwm >= 0) // motor=1代表控制电机A，pwm>=0则(AIN1, AIN2)=(1, 0)为正转
  {
    digitalWrite(AIN1, 1);
    digitalWrite(AIN2, 0);
    analogWrite(PWMA, pwm);
  }
  else if (motor == 1 && pwm < 0) // motor=1代表控制电机A，pwm<0则(AIN1, AIN2)=(0, 1)为反转
  {
    digitalWrite(AIN1, 0);
    digitalWrite(AIN2, 1);
    analogWrite(PWMA, -pwm);
  }
  else if (motor == 2 && pwm >= 0) // motor=2代表控制电机B，pwm>=0则(BIN1, BIN2)=(0, 1)为正转
  {
    digitalWrite(BIN1, 0);
    digitalWrite(BIN2, 1);
    analogWrite(PWMB, pwm);
  }
  else if (motor == 2 && pwm < 0) // motor=2代表控制电机B，pwm<0则(BIN1, BIN2)=(1, 0)为反转
  {
    digitalWrite(BIN1, 1);
    digitalWrite(BIN2, 0);
    analogWrite(PWMB, -pwm);
  }
}

void Run(void)
{
  if (Output < 100) // 左偏
  {
    SetPWM(1, 5 + Output);
    SetPWM(2, 5 - Output);
  }
  if (Output >= 100) // 右偏
  {
    SetPWM(1, 5 - Output + 100);
    SetPWM(2, 5 + Output - 100);
  }
}

void Run_delay(int time)
{
  SetPWM(1, 80);
  SetPWM(2, 60);
  vTaskDelay(time);
}

void Left(int speed1, int speed2)
{
  SetPWM(1, -speed1);
  SetPWM(2, speed2);
}

void Right(int speed1, int speed2)
{
  SetPWM(1, speed1);
  SetPWM(2, -speed2);
}

void Stop()
{
  digitalWrite(AIN1, 1);
  digitalWrite(AIN2, 1);
  digitalWrite(BIN1, 1);
  digitalWrite(BIN2, 1);
}

void Stop_delay(int time)
{
  SetPWM(1, 0);
  SetPWM(2, 0);
  vTaskDelay(time);
}

void Back_delay(int time)
{
  SetPWM(1, -30);
  SetPWM(2, -30);
  vTaskDelay(time);
}

void Back_Left(int time) // 左后转，左轮不动右轮后转
{
  SetPWM(1, 75);
  SetPWM(2, -100);
  vTaskDelay(time); // 275
}

void Back_Left_Return(int time)
{
  SetPWM(1, 0);
  SetPWM(2, 56);
  vTaskDelay(time);
}

void Back_Right(int time) // 右后转，左轮后转右轮不动
{
  SetPWM(1, -90);
  SetPWM(2, 100);
  vTaskDelay(time); // 275
}

void Back_Right_Return(int time)
{
  SetPWM(1, 50);
  SetPWM(2, 0);
  vTaskDelay(time);
}

void Spin180() // 记得转完加delay
{
  SetPWM(1, 95);
  SetPWM(2, -95);
  vTaskDelay(800);
}

void Return_Run(int Return_Num) // 回库函数
{
  if (Return_Num == 1)
  {
    if (Spin_cnt == 1)
    {
      Spin180();
      Spin_cnt = 0;
      Left_Flag = 0;
      Right_Flag = 0;
    }
    if (Right_Flag == 1)
    {
      Run_delay(300);
      Right(70, 70);
      vTaskDelay(550);
    }
    while (A_Flag == 9)
    {
      Stop();
    }
    Run();
  }

  if (Return_Num == 2)
  {
    if (Spin_cnt == 1)
    {
      Spin180();
      Spin_cnt = 0;
      Left_Flag = 0;
      Right_Flag = 0;
    }
    if (Left_Flag == 1)
    {
      Run_delay(300);
      Left(70, 70);
      vTaskDelay(550);
    }
    while (A_Flag == 9)
    {
      Stop();
    }
    Run();
  }

  if (Return_Num == 3)
  {
    if (Spin_cnt == 1)
    {
      Spin180();
      Spin_cnt = 0;
      // Left_Flag = 0;
      // Right_Flag = 0;
    }
    if (Right_Flag == 1 and Lcnt == 1)
    {
      Run_delay(300);
      Right(70, 70);
      vTaskDelay(550);
      Lcnt--;
    }
    while (A_Flag == 9 and Spin_cnt == 0)
    {
      Stop();
    }
    Run();
  }

  if (Return_Num == 4)
  {
    if (Spin_cnt == 1)
    {
      Spin180();
      Spin_cnt = 0;
      Left_Flag = 0;
      Right_Flag = 0;
    }
    if (Left_Flag == 1 && Rcnt == 1)
    {
      Run_delay(300);
      Left(70, 70);
      vTaskDelay(550);
      Rcnt--;
    }
    while (A_Flag == 9)
    {
      Stop();
    }
    Run();
  }

  if (Return_Num == 5)
  {
    if (Spin_cnt == 1)
    {
      Left_Flag = 0;
      Right_Flag = 0;
      Spin180();
      Spin_cnt = 0;
    }
    if (Right_Flag == 1 && Lcnt == 2)
    {
      Run_delay(300);
      Right(70, 70);
      vTaskDelay(550);
      Lcnt--;
      mydashcnt--;
    }
    if (Right_Flag == 1 && Lcnt == 1)
    {
      Run_delay(300);
      Right(70, 70);
      vTaskDelay(550);
      Lcnt--;
      mydashcnt--;
    }
    if (Lcnt == 0 && Rcnt == 0 && Left_Flag == 1 && Right_Flag == 1 && mydashcnt == 1)
    {
      Run_delay(300);
      mydashcnt--;
    }
    if (Lcnt == 0 && Rcnt == 0 && Left_Flag == 1 && Right_Flag == 1 && mydashcnt == 0)
    {
      Run_delay(300);
    }
    while (A_Flag == 9)
    {
      Stop();
    }
    Run();
  }

  if (Return_Num == 6)
  {
    if (Spin_cnt == 1)
    {
      Left_Flag = 0;
      Right_Flag = 0;
      Spin180();
      Spin_cnt = 0;
    }
    if (Left_Flag == 1 && Rcnt == 1)
    {
      Run_delay(300);
      Left(70, 70);
      vTaskDelay(550);
      Rcnt--;
      mydashcnt--;
    }
    if (Right_Flag == 1 && Lcnt == 1)
    {
      Run_delay(300);
      Right(70, 70);
      vTaskDelay(550);
      Rcnt--;
      mydashcnt--;
    }
    if (Lcnt == 0 && Rcnt == 0 && Left_Flag == 1 && Right_Flag == 1 && mydashcnt == 2)
    {
      Run_delay(300);
      mydashcnt--;
    }
    if (Lcnt == 0 && Rcnt == 0 && Left_Flag == 1 && Right_Flag == 1 && mydashcnt == 1)
    {
      Run_delay(300);
    }
    while (A_Flag == 9)
    {
      Stop();
    }
    Run();
  }

  if (Return_Num == 7)
  {
    if (Spin_cnt == 1)
    {
      Left_Flag = 0;
      Right_Flag = 0;
      // Spin180();
      SetPWM(1, 95);
      SetPWM(2, -95);
      vTaskDelay(1000);
      Spin_cnt = 0;
    }
    if (Right_Flag == 1 && Lcnt == 2)
    {
      Run_delay(300);
      Right(70, 70);
      vTaskDelay(550);
      Lcnt--;
      mydashcnt--;
    }
    if (Left_Flag == 1 && Lcnt == 1)
    {
      Run_delay(300);
      Left(70, 70);
      vTaskDelay(550);
      Lcnt--;
      mydashcnt--;
    }
    if (Lcnt == 0 && Rcnt == 0 && Left_Flag == 1 && Right_Flag == 1 && mydashcnt == 2)
    {
      Run_delay(300);
      mydashcnt--;
    }
    if (Lcnt == 0 && Rcnt == 0 && Left_Flag == 1 && Right_Flag == 1 && mydashcnt == 1)
    {
      Run_delay(300);
    }
    while (A_Flag == 9)
    {
      Stop();
    }
    Run();
  }

  if (Return_Num == 8)
  {
    if (Spin_cnt == 1)
    {
      Left_Flag = 0;
      Right_Flag = 0;
      Spin180();
      Spin_cnt = 0;
    }
    if (Left_Flag == 1 && Rcnt == 1)
    {
      Run_delay(300);
      Left(70, 70);
      vTaskDelay(550);
      Rcnt--;
      mydashcnt--;
    }
    if (Left_Flag == 1 && Lcnt == 1)
    {
      Run_delay(300);
      Left(70, 70);
      vTaskDelay(550);
      Lcnt--;
      mydashcnt--;
    }
    if (Lcnt == 0 && Rcnt == 0 && Left_Flag == 1 && Right_Flag == 1 && mydashcnt == 2)
    {
      Run_delay(300);
      mydashcnt--;
    }
    if (Lcnt == 0 && Rcnt == 0 && Left_Flag == 1 && Right_Flag == 1 && mydashcnt == 1)
    {
      Run_delay(300);
    }
    while (A_Flag == 9)
    {
      Stop();
    }

    Run();
  }
}

void Main_task(void *pt) // main()
{
  int cnt = 0;
  int maincnt = 0;
  int mydelaycnt = 0;
  int K_aim = 0;
  int Left_temp = 0;
  int Right_temp = 0;
  pinMode(LED, OUTPUT);
  motor_init();
  // pinMode(Voltage, INPUT); // 初始化作为输入端
  //------------------------------------------------------------------------------------------------------
  while (1)
  {
    // Serial.println(touchRead(T9));
    // Run_delay(1000);
    if (A_Flag != 0 && B_Flag != 0 && cnt == 0)
    {
      K_aim = A_Flag;
      digitalWrite(LED, 1);
      cnt = 1;
      mode = 2;
    }
    if (K_aim != 0 && touchRead(T9) < 78)
    {
      maincnt = 1;
      if (mydelaycnt == 0)
      {
        vTaskDelay(1000);
        mydelaycnt++;
      }
      else
      {
        if (K_aim == 1 || K_aim == 2) // 近端
        {
          if (SHI_Flag == 1 && K_aim == 1)
          {
            // digitalWrite(LED, 0);
            Run_delay(300);
            Left(70, 70);
            vTaskDelay(550);
            Return_Num = 1;
          }
          if (SHI_Flag == 1 && K_aim == 2)
          {
            Run_delay(300);
            Right(70, 70);
            vTaskDelay(550);
            Return_Num = 2;
          }
          while (A_Flag == 9 && touchRead(T9) < 78) // 没找到线
          {
            Stop();
          }
          Run();
        }
        else // 中段或远端
        {
          if (SHI_Flag == 1 && mydashcnt == 0) // 冲过第一个十字节点
          {
            Run_delay(300);
            mydashcnt++;
          }

          if (SHI_Flag == 1 && mydashcnt == 1) // 第二个十字节点
          {
            mode = 1;
            Stop_delay(1000);
            Back_delay(800);
            while (K_Left == 0)
            {
              Stop();
              K_Left = A_Flag;
            }
            Left_temp = K_Left;

            while (K_Right == 0)
            {
              Stop();
              K_Right = B_Flag;
            }
            Right_temp = K_Left;

            if (K_aim == K_Left)
            {
              mode = 2;
              Stop_delay(500);
              Left(70, 70);
              vTaskDelay(350);
              Return_Num = 3;
              Lcnt++;
              mydashcnt++;
            }
            else if (K_aim == K_Right)
            {
              mode = 2;
              Stop_delay(500);
              Right(70, 70);
              vTaskDelay(350);
              Return_Num = 4;
              Rcnt++;
              mydashcnt++;
            }
            else
            {
              mode = 2;
              Run_delay(500);
              Stop_delay(500); // 等待OpenMV模式切换完成
              mydashcnt++;
            }
          }

          if (T_Flag == 1 && mydashcnt == 2) // 第一个T字节点
          {
            Left(70, 70);
            vTaskDelay(500);
            Return_Num = 4;
            Lcnt = 1;
            mydashcnt++;
          }

          if (T_Flag == 1 && mydashcnt == 3) // 第二个T字节点
          {
            mode = 1;
            Stop_delay(1000);
            // Back_delay(500);
            SetPWM(1, -30);
            SetPWM(2, -70);
            vTaskDelay(500);
            Stop_delay(300);
            // SetPWM(1, 70);
            // SetPWM(2, 0);
            // vTaskDelay(500);
            while (K_Left == Left_temp || K_Left == 0)
            {
              Stop();
              K_Left = A_Flag;
            }
            Left_temp = K_Left;

            while (K_Right == Right_temp || K_Right == 0)
            {
              Stop();
              K_Right = B_Flag;
            }
            Right_temp = K_Left;

            if (K_aim == K_Left)
            {
              mode = 2;
              Stop_delay(500);
              Run_delay(400);
              Stop_delay(200);
              Left(80, 80);
              vTaskDelay(350);
              Return_Num = 5;
              Lcnt = 2;
            }
            else if (K_aim == K_Right)
            {
              mode = 2;
              Stop_delay(500);
              Run_delay(400);
              Stop_delay(200);
              Right(80, 80);
              vTaskDelay(350);
              Return_Num = 6;
              Rcnt = 1;
            }
            else
            {
              // mode = 2;
              // Spin180();
              Back_delay(1000);
              Stop_delay(300);
              SetPWM(1, -95);
              SetPWM(2, 95);
              vTaskDelay(100);
              mode = 2;
              SetPWM(1, -95);
              SetPWM(2, 95);
              vTaskDelay(700);
              while (A_Flag == 9)
              {
                SetPWM(1, -95);
                SetPWM(2, 95);
              }
              mydashcnt++;
            }
          }

          if (T_Flag == 1 && mydashcnt == 4) // 第三个T字节点（只会因为第二次没找到才会去找第三个）
          {
            mode = 1;
            Stop_delay(1000);
            Back_delay(1000);
            while (K_Left == Left_temp || K_Left == 0)
            {
              K_Left = A_Flag;
            }
            Left_temp = K_Left;

            while (K_Right == Right_temp || K_Right == 0)
            {
              K_Right = B_Flag;
            }
            Right_temp = K_Left;

            if (K_aim == K_Left)
            {
              mode = 2;
              Stop_delay(500);
              Left(70, 70);
              vTaskDelay(350);
              Return_Num = 7;
              Lcnt = 2;
            }
            else if (K_aim == K_Right)
            {
              mode = 2;
              Stop_delay(500);
              Right(70, 70);
              vTaskDelay(350);
              Return_Num = 8;
              Rcnt = 1;
            }
          }

          while (A_Flag == 9 && touchRead(T9) < 78) // 没找到线
          {
            Stop();
          }

          Run();
        }
      }
    }

    else
    {
      if (maincnt == 0)
      {
        Stop();
      }
      else
      {
        // 回库
        digitalWrite(LED, 0);
        Return_Run(Return_Num);
        // digitalWrite(LED, !digitalRead(LED));
        // vTaskDelay(500);
      }
    }
  }
}

void Run_test(void *pt)
{
  int mydelaycnt = 0;
  motor_init();
  while (1)
  {
    if (touchRead(T9) < 70)
    {
      if (mydelaycnt == 0)
      {
        vTaskDelay(1000);
        mydelaycnt++;
      }
      else
      {
        Run();
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);
  xTaskCreatePinnedToCore(task_USART_OpenMV, "OpenMv_Data_task", 1024 * 10, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(task_sending_OpenMV, "Sending_OpenMV_task", 1024 * 10, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(Main_task, "Main_task", 1024 * 10, NULL, 1, NULL, 1);
  // xTaskCreatePinnedToCore(Run_test, "runtest", 1024 * 4, NULL, 1, NULL, 1);
  vTaskDelete(NULL);
}

void loop()
{
  vTaskDelete(NULL);
}
