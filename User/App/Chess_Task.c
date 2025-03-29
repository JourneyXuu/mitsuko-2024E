#include "Chess_Task.h"
#include "bsp_uart.h"
#include "control.h" 
#include "Tjc.h"
#include "K230.h"

// 全局状态变量
volatile uint8_t black_placed = 0;
volatile uint8_t white_placed = 0;
volatile uint8_t total_placed = 0;
 //任务执行状态
volatile uint8_t isTaskRunning; //0-空闲，1-运行中

//串口屏接收的结构体
volatile Task_State task_state ={
  .last_chess_ID = 0xFF,
  .last_board_ID = 0xFF,
  .total_chess_count = 0,
  .data_refreshed = 0
};

//人机对弈的结构体
volatile Task_State task_state2 ={
  .last_chess_ID = 0xFF,
  .last_board_ID = 0xFF,
  .total_chess_count = 0,
  .data_refreshed = 0
};

//是否旋转45度，第三题标志位
volatile uint8_t rotate_flag = 0;


void Chess_Task(void)
{
  
  /* USER CODE BEGIN Chess_Task */

      if (!isTaskRunning) //队列不为空且没有任务在运行时执行任务
      { 
        isTaskRunning = 1; // 标记任务运行中
      }
        // 执行任务
        switch(Task_Flag)
        {
          case 0x01://任务1：任意棋子到5号棋盘
              Chess_Task1(Selected_Chess);
          break;
          case 0x02://任务2：分布放置两黑两白
              Chess_Task2(Selected_Chess,Selected_Board);
          break;
          case 0x03://任务3：旋转45度，分布放置两黑两白
              Chess_Task3(Selected_Chess,Selected_Board);
          break;
          case 0x04://任务4:执黑棋先手对弈
              Chess_Task4(Step_Board);
          break;
          case 0x05://任务5:执白起后手对弈
              Chess_Task5(Step_Board);
          break;
          case 0x06://
              Chess_Task6(Step_Board,Step_Board2);
          break;
          default:
            break;
        }

        isTaskRunning = 0; // 标记任务完成

  /* USER CODE END Chess_Task */
}

void Chess_Task1(uint8_t chess_ID)
{
  Place_Chess(chess_ID,5);
	Task_Flag = 0;
}


/*
 @brief  任务2：分布放置4个棋子(两黑两白)
*/
void Chess_Task2(uint8_t chess_ID,uint8_t board_ID)
{
  if( task_state.data_refreshed ||
      (task_state.last_chess_ID!= chess_ID) || (task_state.last_board_ID!= board_ID ) )
    {
      Place_Chess(chess_ID,board_ID);
      //更新状态
      task_state.last_chess_ID = chess_ID;
      task_state.last_board_ID = board_ID;
      task_state.total_chess_count ++;
      task_state.data_refreshed = 0; //清除标志位，该标志位会在串口回调函数中刷新
    }
  if(task_state.total_chess_count > 4)
  {
    task_state.last_chess_ID = 0xFF,
    task_state.last_board_ID = 0xFF;
    task_state.total_chess_count = 0;
      Task_Flag = 0;
  }
}


//倾斜45度的第二题
void Chess_Task3(uint8_t chess_ID,uint8_t board_ID)
{
  rotate_flag = 1;
  if( task_state.data_refreshed ||
    (task_state.last_chess_ID!= chess_ID) || (task_state.last_board_ID!= board_ID ) )
  {
    Place_Chess(chess_ID,board_ID);
    //更新状态
    task_state.last_chess_ID = chess_ID;
    task_state.last_board_ID = board_ID;
    task_state.total_chess_count ++;
    task_state.data_refreshed = 0; //清除标志位，该标志位会在串口回调函数中刷新
  }
  
  if(task_state.total_chess_count > 4)
  {
    task_state.last_chess_ID = 0xFF,
    task_state.last_board_ID = 0xFF;
    task_state.total_chess_count = 0;
      Task_Flag = 0;
  }

}

uint8_t black_chess_ID_i = 1;   //黑子先手时，选择黑子的索引
/*
 *@brief:单片机执黑子先手，人机对弈
 */
void Chess_Task4(uint8_t board_ID)
{
  if( task_state2.data_refreshed &&
     (task_state2.last_board_ID!= board_ID ) )
  {
    Place_Chess(black_chess_ID_i,board_ID);
    black_chess_ID_i++;
    //更新状态
    task_state2.last_board_ID = board_ID;
    task_state2.total_chess_count ++;
    task_state2.data_refreshed = 0; //清除标志位，该标志位会在串口回调函数中刷新
  }
  if(task_state2.total_chess_count >= 5)
  {
      task_state2.total_chess_count = 0;
      Task_Flag = 0;//退出任务
      black_chess_ID_i = 0;
  }
  if(break_flag == 1)
  {
    Task_Flag = 0;
    black_chess_ID_i = 0;
    break_flag = 0;
  }

}


uint8_t white_chess_ID_i = 6;   //白子先手时，选择白子的索引
/*
 *@breif:单片机执白棋后手，人机对弈
*/
void Chess_Task5(uint8_t board_ID)
{
  if( task_state2.data_refreshed && 
    (task_state2.last_board_ID!= board_ID ) && (task_state2.last_board_ID!= 0x05 ) )
  {
    Place_Chess(white_chess_ID_i,board_ID);
    white_chess_ID_i++;
    //更新状态
    task_state2.last_board_ID = board_ID;
    task_state2.total_chess_count ++;
    task_state2.data_refreshed = 0; //清除标志位，该标志位会在串口回调函数中刷新
  }
  if(task_state2.total_chess_count >= 5)
  {
      task_state2.total_chess_count = 0;
      Task_Flag = 0;//退出任务
      white_chess_ID_i = 0;
  }
  if(break_flag == 1 )
  {
    Task_Flag = 0;
    white_chess_ID_i = 0;
    break_flag = 0;
  }
}

/*
 * @brief: 任务6：人将下过的一颗棋子改变位置，装置能够自动发现并将棋子放置回原位置
 * @note: 把移动过的棋子从一个棋盘移动到另一个棋盘
 */
// void Chess_Task6(uint8_t board_ID1, uint8_t board_ID2)
void Chess_Task6(uint8_t board_ID1, uint8_t board_ID2)
{
  if( task_state2.data_refreshed )
  {
    Return_Chess(board_ID1,board_ID2);
    task_state2.last_chess_ID = board_ID1;
    task_state2.last_board_ID = board_ID2;
    task_state2.data_refreshed = 0; //清除标志位，该标志位会在串口回调函数中刷新
  }
  if(break_flag == 1 )
  {
    Task_Flag = 0;
    white_chess_ID_i = 0;
    break_flag = 0;
  }

}


