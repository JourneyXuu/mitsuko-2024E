#include "control.h"
#include "sys.h"  // 添加在调用 osDelay() 的文件顶部
#include "Emm_V5.h"
#include "emm42_uart.h"
#include "Servo.h"
#include "Chess_Task.h"
#include "Control_logic.h"


uint16_t velocity = 300;
uint16_t acc = 0;
uint8_t raf = 1;//使用绝对位置模式



// 坐标映射
typedef struct {
   uint32_t x;      // X轴目标位置
   uint32_t y;      // Y轴目标位置
} Position;
//棋盘坐标
Position board_position[9]=
{
    // 第0行（实际开发中建议从0开始计数）
    {5200, 16600}, {5200, 13650}, {5200, 10500},  // 棋盘位置1 , 2 , 3 -200 -550
    // 第1行
    {8300, 16500}, {8250, 13650}, {8100, 10500},  // 棋盘位置4 , 5 , 6
    // 第2行
    {11150, 16500}, {11150, 13650}, {11150, 10500}   // 棋盘位置7 , 8 , 9
};
//棋盘旋转45度坐标
Position board_rotate_position[9]=
{

                     {4000, 13900},//1 
            {6100, 15800},       {5900, 11700}, //2 3
   {8300, 17800},   {8100, 13700},       {7900, 9600},  // 4 , 5 , 6
            {10300,15600},       {10000,11500},//7 8
                     {12100, 13400}   //9
};

//棋子坐标
Position chess_position[10]=
{
   {1850, 23400},   // 棋子1的初始位置
   {4850, 23550},   // 棋子2
   {7850, 23700},   // 棋子3
   {10900, 23800},   // 棋子4
   {13950, 23800},   // 棋子5
   {2500, 3650},   // 棋子6 x方向 y方向CW方向
   {5500, 3650},  // 棋子7
   {8500, 3650},  // 棋子8
   {11500,3750},  // 棋子9
   {14500,3850}   // 棋子10
};
/*
 *@:brief:移动棋子从一个棋盘到另一个棋盘
 *note:   将被移动的棋子移动会原本的位置
 */
void Return_Chess(uint8_t Board_ID1,uint8_t Board_ID2)
{
   if (Board_ID1 == 0 || Board_ID1 > 10 || Board_ID2 == 0 || Board_ID2 > 10) {
      return;
   }
   uint8_t Board_Id1_Index = Board_ID1 - 1;
   uint8_t Board_Id2_Index = Board_ID2 - 1;
   Position start = {board_position[Board_Id1_Index].x, board_position[Board_Id1_Index].y};
   Position end = {board_position[Board_Id2_Index].x, board_position[Board_Id2_Index].y};
   if(rotate_flag == 1)  //换成旋转后的棋盘位置
   {
      start.x = board_rotate_position[Board_Id1_Index].x;
      start.y = board_rotate_position[Board_Id1_Index].y;
      end.x = board_rotate_position[Board_Id2_Index].x;
      end.y = board_rotate_position[Board_Id2_Index].y;
   }
   LED_Task_Off();

   // 1. 移动机械臂到棋子存放位置
   control_t(start.x, start.y);
   Delay_Timer(180);
   while(!checkDelayTimer());
   
   // 2. 执行抓取动作（需实现夹爪控制）
   Servo_Down();
   Magnet_On();
   Delay_Timer(100);
   while(!checkDelayTimer());
   Servo_Up();
   Delay_Timer(100);
   while(!checkDelayTimer());

   // 3. 移动机械臂到目标棋盘位置
   control_t(end.x,end.y);
   Delay_Timer(150);
   while(!checkDelayTimer());
   
   // 4. 执行放置动作
   Servo_Down();
   Delay_Timer(100);
   while(!checkDelayTimer());
   Magnet_Off();
   Servo_Up();

   // 5. 返回回零位置
   control_to_zero();
   Delay_Timer(100);
   while(!checkDelayTimer());

   LED_Task_On();
}
/*
 *@brief:放置棋子
 *param: 棋子号，祺盘号
*/
void Place_Chess(uint8_t Chess_ID,uint8_t Board_ID)
{
    // 检查参数的有效性
    if (Chess_ID == 0 || Chess_ID > 10 || Board_ID == 0 || Board_ID > 10) {
      return;
  }
   // 计算棋子的起始位置索引（假设Chess_ID从1开始）
   uint8_t chessIndex = Chess_ID - 1;
   uint8_t boardIndex = Board_ID - 1;

   Position start = {chess_position[chessIndex].x, chess_position[chessIndex].y};   //棋子位置
   Position end = {board_position[boardIndex].x, board_position[boardIndex].y};     //棋盘位置
   
   if(rotate_flag == 1)  //换成旋转后的棋盘位置
   {
      end.x = board_rotate_position[boardIndex].x;
      end.y = board_rotate_position[boardIndex].y;
   }

   LED_Task_Off();
   // 1. 移动机械臂到棋子存放位置
   control_t(start.x, start.y);
   Delay_Timer(180);
   while(!checkDelayTimer());

   
   // 2. 执行抓取动作（需实现夹爪控制）
   Servo_Down();
   Magnet_On();
   Delay_Timer(100);
   while(!checkDelayTimer());
   Servo_Up();
   Delay_Timer(100);
   while(!checkDelayTimer());
   
   // 3. 移动机械臂到目标棋盘位置
   control_t(end.x,end.y);
   Delay_Timer(150);
   while(!checkDelayTimer());

   // 4. 执行放置动作
   Servo_Down();
   Delay_Timer(150);
   while(!checkDelayTimer());
   Magnet_Off();
   Servo_Up();
   
   // 5. 返回回零位置
   control_to_zero();
   Delay_Timer(100);
   while(!checkDelayTimer());
   LED_Task_On();

}

void Disable_ALL(void)
{
   Emm_V5_En_Control(1,false,false);  Delay_Timer(10);while(!checkDelayTimer());
   Emm_V5_En_Control(2,false,false);  Delay_Timer(10);while(!checkDelayTimer());
   Emm_V5_En_Control(3,false,false);  Delay_Timer(10);while(!checkDelayTimer());
}


/*
 *@brief:回零
*/
void control_to_zero(void)
{
   Emm_V5_Pos_Control(1, 0, velocity, acc, 0, raf, 1); Delay_Timer(10);while(!checkDelayTimer());
   Emm_V5_Pos_Control(2, 1, velocity, acc, 0, raf, 1); Delay_Timer(10);while(!checkDelayTimer());
   Emm_V5_Pos_Control(3, 1, velocity, acc, 0, raf, 1); Delay_Timer(10);while(!checkDelayTimer());
   Emm_V5_Synchronous_motion(0); Delay_Timer(10);while(!checkDelayTimer());// 广播地址0触发
}

/*
 *@brief:记忆回零位置
*/
void Remembei_Zero(void) 
{
   Emm_V5_Reset_CurPos_To_Zero(01);Delay_Timer(10);while(!checkDelayTimer());
   Emm_V5_Reset_CurPos_To_Zero(02);Delay_Timer(10);while(!checkDelayTimer());
   Emm_V5_Reset_CurPos_To_Zero(03);Delay_Timer(10);while(!checkDelayTimer());
}


/*  
*@brief: 控制三电机运动x,y三轴
*note:   电机1、2控制x方向，电机3控制y方向
*@param: x_distance:    电机1、2运动距离
*@param: dir_X:         电机1、2运动方向 
*@param: y_distance:    电机3运动距离
*@param: dir_y:         电机3运动方向
*note:如果发送失败则重新发送命令
*/
void control_t(uint32_t x_distance,uint32_t y_distance)
{
   Emm_V5_Pos_Control(1, 0, velocity, acc, x_distance, raf, 1); Delay_Timer(10);while(!checkDelayTimer());// 多机同步标志位置1
   Emm_V5_Pos_Control(2, 1, velocity, acc, x_distance, raf, 1); Delay_Timer(10);while(!checkDelayTimer());
   Emm_V5_Pos_Control(3, 1, velocity, acc, y_distance, raf, 1); Delay_Timer(10);while(!checkDelayTimer());
   Emm_V5_Synchronous_motion(0); 
}

/*
 *@brief:使能三电机
*/
void EmmEN_Init(void)
{
   Emm_V5_En_Control(1,true,false);  Delay_Timer(10);while(!checkDelayTimer());//电机1使能，关闭多机同步 
   Emm_V5_En_Control(2,true,false);  Delay_Timer(10);while(!checkDelayTimer());//电机2使能
   Emm_V5_En_Control(3,true,false);  Delay_Timer(10);while(!checkDelayTimer());//电机3使能
}

/*单个电机的测试标志位*/
//  #define TestMotor1 
// #define TestMotor2 
// #define TestMotor3 
// #define TestMultiMotor 
// #define TestServo
// #define TestMagnet 
#define TestChess
// #define TestChessToBoard
/*
*@brief：控制三地址电机多机同步
*@note：电机1、2控制x方向，电机3控制y方向
*@param: 
*@return:none
*/
void Emm_Test(void)
{
   control_to_zero();//上电必须回零，记得断电前也重新回零


   // uint8_t rxCount = 0;
   // float test_speed = 1500.f;
   // uint8_t test_raf = 0; //0为相对位置，1为绝对位置

/* 不启用多机标志，单独测试单个电机*/
   #ifdef TestMotor1
   Emm_V5_En_Control(1,true,false);delay_ms(10);//电机1使能，关闭多机同步
   Emm_V5_Vel_Control(1, 0, 1000, 10, 0);delay_ms(10);//速度模式：方向CW，速度1000RPM，加速度10
   delay_ms(500);
   Emm_V5_Vel_Control(1, 0, 0, 10, 0);delay_ms(10);//速度模式：方向CW，速度1000RPM，加速度10

   #endif

   #ifdef TestMotor2
   Emm_V5_En_Control(2,true,false);delay_ms(10);//电机2使能，关闭多机同步
   Emm_V5_Vel_Control(2, 0, 1000, 10, 0);delay_ms(10);//速度模式：方向CW，速度1000RPM，加速度10
   delay_ms(500);
   Emm_V5_Vel_Control(2, 0, 0, 10, 0);delay_ms(10);//速度模式：方向CW，速度1000RPM，加速度10

   #endif

   #ifdef TestMotor3
   Emm_V5_En_Control(3,true,false);delay_ms(10);//电机3使能，关闭多机同步
   Emm_V5_Vel_Control(3, 0, 1000, 10, 0);delay_ms(10);//速度模式：方向CW，速度1000RPM，加速度10
   delay_ms(500);
   Emm_V5_Vel_Control(3, 0, 0, 10, 0);delay_ms(10);//速度模式：方向CW，速度1000RPM，加速度10

   #endif

/* 启用多机标志*/
   #ifdef TestMultiMotor
   Emm_V5_En_Control(1,true,true);  delay_ms(10);//电机1使能，开启多机同步 
   Emm_V5_En_Control(2,true,true);  delay_ms(10);//电机2使能，开启多机同步
   Emm_V5_En_Control(3,true,true);  delay_ms(10);//电机3使能，开启多机同步
   //位置模式，方向CW，速度1000RPM，加速度0（不使用加减速直接启动），脉冲数3200（16细分下发送3200个脉冲电机转一圈），相对运动
   Emm_V5_Pos_Control(1, 0, 1000, 50, 3200, 0, 1); delay_ms(10);// 多机同步标志位置1
   Emm_V5_Pos_Control(2, 0, 1000, 50, 3200, 0, 1); delay_ms(10);// 多机同步标志位置1
   Emm_V5_Pos_Control(3, 0, 1000, 50, 3200, 0, 1); delay_ms(10);// 多机同步标志位置1
   Emm_V5_Synchronous_motion(0); delay_ms(10);// 广播地址0触发
   #endif

   #ifdef TestServo
   Servo_Down();
   // delay_ms(1000);
   // Servo_Up();
   // delay_ms(1000);
   #endif

   #ifdef TestMagnet
   Magnet_On();
   #endif


   #ifdef TestChess 
   /*10个棋子点*/
	// EmmEN_Init();
   // delay_ms(1000);
   // Servo_Up();
   // delay_ms(1000);

	// control_t(chess_position[0].x,chess_position[0].y);
   // control_t(chess_position[1].x,chess_position[1].y);
   // control_t(chess_position[2].x,chess_position[2].y);
   // control_t(chess_position[3].x,chess_position[3].y);
   // control_t(chess_position[4].x,chess_position[4].y);
   // control_t(chess_position[5].x,chess_position[5].y);
   // control_t(chess_position[6].x,chess_position[6].y);
   // control_t(chess_position[7].x,chess_position[7].y);
   // control_t(chess_position[8].x,chess_position[8].y);
   // control_t(chess_position[9].x,chess_position[9].y);

   /*9个棋盘点*/
   // control_t(board_position[0].x,board_position[0].y);
   // control_t(board_position[1].x,board_position[1].y);
   // control_t(board_position[2].x,board_position[2].y);
   // control_t(board_position[3].x,board_position[3].y);
   // control_t(board_position[4].x,board_position[4].y);
   // control_t(board_position[5].x,board_position[5].y);
   // control_t(board_position[6].x,board_position[6].y);
   // control_t(board_position[7].x,board_position[7].y);
   // control_t(board_position[8].x,board_position[8].y);
   
   /*旋转9个棋盘点*/
   // control_t(board_rotate_position[0].x,board_rotate_position[0].y);
   // control_t(board_rotate_position[1].x,board_rotate_position[1].y);
   // control_t(board_rotate_position[2].x,board_rotate_position[2].y);
   // control_t(board_rotate_position[3].x,board_rotate_position[3].y);
   // control_t(board_rotate_position[4].x,board_rotate_position[4].y);
   // control_t(board_rotate_position[5].x,board_rotate_position[5].y);
   // control_t(board_rotate_position[6].x,board_rotate_position[6].y);
   // control_t(board_rotate_position[7].x,board_rotate_position[7].y);
   // control_t(board_rotate_position[8].x,board_rotate_position[8].y);

   // Place_Chess(1,1);
   // Return_Chess(1,9);

   #endif

   #ifdef TestChessToBoard
   // Emm_V5_En_Control(03,false,false);//失能电机3
   // Emm_V5_Origin_Set_O(03,true);//电机3记忆棋盘位置
   Place_Chess(1,1);
   #endif




}
/*
 *@brief:师傅给的代码，嘻嘻不如我的
*/
void SiFU_Test(void)
{
//   Motor_Enable(01);
//   Motor_SetPosition(01, 00, 
//      10000, 0, 4200) ;
}

