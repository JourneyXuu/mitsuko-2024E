## 视频链接：
[B站视频--功能演示及介绍]https://www.bilibili.com/video/BV18MZYYfE9p/?spm_id_from=333.1387.homepage.video_card.click&vd_source=c5264ed3939a6a674d4069dff1ca5d90
## 项目简介
本系统采用模块化的设计思想，以STM32F407为中央处理器，由庐山派K230视觉模块、曲柄连杆、张大头闭环步进电机、舵机、龙邱电源模块、陶晶池串口屏、电磁铁。
 <img src="https://image.lceda.cn/oshwhub/pullImage/301f0f4a41844ae88c44c4e7e82c1b3b.png" width="400" /> 
 
 github链接
(https://github.com/espressif/esp-moonlight)

## 原理解析（硬件说明）
此处可填写项目的设计原理，将设计的原理拆分解析，示例：

主控：STM32F407VET6（只引出了串口、CAN、I2C、定时器TIM1和ADC1的部分通道）
**主控图片**
拍的丑将就看一下
 <img src="https://image.lceda.cn/oshwhub/pullImage/46fd3b2041404455bac06f1495327d05.png" width="600" /> 
***调试PCB遇到的问题***
1.原本使用时舵机的电源和主控公共用1个5V，发现舵机启动电流过大，会导致舵机不工作，version1的PCB最后外部连接了龙邱的电源给舵机单独供电，现在提交的PCB已经修改为version2。
2.按键画错了，v1的按键焊接时全部拆除
3.预留的串口触摸屏没有用，直接用了陶晶驰的串口屏
 <img src="https://image.lceda.cn/oshwhub/pullImage/bdc44582d6d548869e3c2bbcc5249a47.png" width="400" /> 

**分电板**
 <img src="https://image.lceda.cn/oshwhub/pullImage/d2c97c2276df42a181d81b4b663ba55e.png" width="400" /> 
**分电线**
 <img src="https://image.lceda.cn/oshwhub/pullImage/150a9703ad7b449596349f56238bebaa.jpg" width="200" /> 
 
**恒流源补光板**
***支出输入电压：3V-18V***
灯太亮了，所以做了一个外壳挡挡光
 <img src="https://image.lceda.cn/oshwhub/pullImage/202a2bbb77594916806e3ff46841300d.png" width="300" /> 

***供电结构图***
 <img src="https://image.lceda.cn/oshwhub/pullImage/f8553a05dc6f4f6a96b723bae1e830cb.png" width="400" /> 

## 下位机软件代码

***软件代码结构***
 <img src="https://image.lceda.cn/oshwhub/pullImage/1a6576aa58a54bf78b1468981fb17c75.png" width="800" /> 
 ***IO接线***
 <img src="https://image.lceda.cn/oshwhub/pullImage/0fdf784a515b4d708f7965a3d900d50a.png" width="500" /> 

***串口2控制步进电机***
张大头的步进电机使用串口控制，使用多机同步,电机1、2必须同步
电机1、2使用多圈无限位碰撞回零操作说明
电机3使用单圈就近回零
在上位机中修改回零
 <img src="https://image.lceda.cn/oshwhub/pullImage/84d8ff548c624269b0a8ecc712a46b31.png" width="600" /> 
```
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
```
***串口3解包陶晶驰***
只有检测到包头和包尾的数据才修改数据
这里写的串口DMA接收写的不够好，可以修改为双缓冲区，不过本题很够用了
```
void  TJC_UART_UnPack(uint8_t *dma_buffer)
{
    uint8_t data;
    for (uint16_t i = 0; i < MAX_PACKET_LENGTH; i++) 
    {
        data = dma_buffer[i];   
            switch (tjc_rx_handler.state) {
                case RX_WAIT_HEADER:
                    if (data == PACKET_HEADER) { // 检测到包头
                        tjc_rx_handler.index = 0;// 直接重置索引，不存储包头
                        tjc_rx_handler.state = RX_RECEIVING;
                    }
                    break;

                case RX_RECEIVING:
                    if (data == PACKET_FOOTER) { // 检测到包尾
                        //检测到包尾时不存储包尾，直接进行数据有效性检查。
                        // 数据有效性检查（可添加CRC校验）
                        if (tjc_rx_handler.index >= 2) { // 最小有效包长度判断
                            Task_Flag    = tjc_rx_handler.buffer[0];
                            Selected_Chess = tjc_rx_handler.buffer[1];
                            Selected_Board = tjc_rx_handler.buffer[2];
                            break_flag = tjc_rx_handler.buffer[3];
                        }
                        
                        tjc_rx_handler.state = RX_COMPLETE;
                    } else {
                        if (tjc_rx_handler.index < MAX_PACKET_LENGTH - 1) {
                            tjc_rx_handler.buffer[tjc_rx_handler.index++] = data;
                        } else { // 数据超长处理
                            tjc_rx_handler.state = RX_WAIT_HEADER;
                        }
                    }
                    break;

                case RX_COMPLETE: // 完成一帧接收后复位状态机
                default:
                    tjc_rx_handler.state = RX_WAIT_HEADER;
                    memset(tjc_rx_handler.buffer, 0, sizeof(tjc_rx_handler.buffer));
                    tjc_rx_handler.index = 0;
                    break;
            }
    }

    HAL_UARTEx_ReceiveToIdle_DMA(&huart3, dma_buffer, DMA_BUFFER); // 启用空闲中断接收
    __HAL_DMA_DISABLE_IT(&hdma_usart3_rx, DMA_IT_HT);          

}
```



## 上位机软件代码
K230实际跑起来帧率比较低，可以修改像素
MINIMAX算法
最后一题长按庐山派上的用户按键开启第六题，我们都用的最简单最笨的方法
哈哈，能用就行
```
def minimax(board, depth, is_maximizing):
    computer = 'X'
    player = 'O'

    if check_win(board, computer):
        return 10 - depth
    if check_win(board, player):
        return depth - 10
    if check_draw(board):
        return 0

    if is_maximizing:
        best_score = float('-inf')
        for i in range(SIZE):
            for j in range(SIZE):
                if board[i][j] == ' ':
                    board[i][j] = computer
                    score = minimax(board, depth + 1, False)
                    board[i][j] = ' '
                    best_score = max(score, best_score)
        return best_score
    else:
        best_score = float('inf')
        for i in range(SIZE):
            for j in range(SIZE):
                if board[i][j] == ' ':
                    board[i][j] = player
                    score = minimax(board, depth + 1, True)
                    board[i][j] = ' '
                    best_score = min(score, best_score)
        return best_score
```


## 机械结构
有一个好的机械手可以帮大家省去很多麻烦
本设备使用的十字滑台，由3个步进电机和传送带组成，结构如图3所示。步进电机A和步进电机B保持同速转动，可实现上下方向的移动，步进电机C保持同速转动，可实现左右方向的移动。
 <img src="https://image.lceda.cn/oshwhub/pullImage/1f0d2860927c44bf88c5ecb383a8fe1e.png" width="600" /> 
 舵机连杆操作
 <img src="https://image.lceda.cn/oshwhub/pullImage/32e7cf8dbb1549e5a868d4da2882076b.png" width="300" /> 
## 注意事项
* 准备一个电源来调，免得步进疯了撞来撞去
* 分电板可以自己改改，加个开关
* 代码太多了，自己看看吧

## 鸣谢
没有躺赢狗，大家都是MVP，感谢一起陪伴做比赛，很珍惜这段时间
电控、硬件(up本人)： @xuduoduo
视觉算法:@Gone with the wind
机械:@是祺不是琪


## 实物图
此处可放入组装完成后完整实物图
图1：v1版本的PCB：（目前已修复大部分错误，打板最新的就行）
 <img src="https://image.lceda.cn/oshwhub/pullImage/2cc3aad0afe54e70a56c12bc28f9e6ea.png" width="700" /> 
图2：分电板PCB（没咋改过）
 <img src="https://image.lceda.cn/oshwhub/pullImage/96ceb607c8464161b136a93f2cf13268.png" width="500" /> 
图3：呆唯补光板
 <img src="https://image.lceda.cn/oshwhub/pullImage/476dfedf5d6145e28dea7e41f2b7eee4.png" width="400" /> 
