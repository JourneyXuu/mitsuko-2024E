#ifndef _CHESS_TASK_H__
#define _CHESS_TASK_H__

#include "main.h"
#include "gpio.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"


typedef struct{
    uint8_t last_chess_ID;
    uint8_t last_board_ID;
    uint8_t total_chess_count;//总共放置的棋子总数
    uint8_t data_refreshed;//数据刷新标志位
  }Task_State;

  extern volatile uint8_t rotate_flag;

extern volatile Task_State task_state;
extern volatile Task_State task_state2;

void Chess_Task(void);


void Chess_Task1(uint8_t chess_ID);
void Chess_Task2(uint8_t chess_ID,uint8_t board_ID);
void Chess_Task3(uint8_t chess_ID,uint8_t board_ID);
void Chess_Task4(uint8_t board_ID);
void Chess_Task5(uint8_t board_ID);
void Chess_Task6(uint8_t board_ID1, uint8_t board_ID2);


#endif

