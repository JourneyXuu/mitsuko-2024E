# 立创·庐山派-K230-CanMV开发板资料与相关扩展板软硬件资料官网全部开源
# 开发板官网：www.lckfb.com
# 技术支持常驻论坛，任何技术问题欢迎随时交流学习
# 立创论坛：www.jlc-bbs.com/lckfb
# 关注bilibili账号：【立创开发板】，掌握我们的最新动态！
# 不靠卖板赚钱，以培养中国工程师为己任

import time, os, sys
from machine import UART
from machine import Pin
from machine import FPIOA
from media.sensor import *
from media.display import *
from media.media import *
picture_width = 400
picture_height = 240
sensor_id = 2
sensor = None
board = [
     [" "," "," "],
     [" "," "," "],
     [" "," "," "],
]
count_time = 0
button = Pin(53, Pin.IN, Pin.PULL_DOWN)  # 使用下拉电阻
corner = [(0,0),(0,0),(0,0),(0,0)]
SIZE = 3
fpioa = FPIOA()
fpioa.set_function(3, FPIOA.UART1_TXD)
fpioa.set_function(4, FPIOA.UART1_RXD)
uart = UART(UART.UART1, baudrate=115200, bits=UART.EIGHTBITS, parity=UART.PARITY_NONE, stop=UART.STOPBITS_ONE)
# 检查赢了吗
def update_board(board, rois, img_gray):
    """
    更新棋盘状态。
    :param board: 当前棋盘状态
    :param rois: 棋盘区域的 ROI 列表
    :param img_gray: 灰度图像
    """
    for y in range(len(rois)):
        for x in range(len(rois[y])):
            gray = img_gray.get_statistics(roi=rois[y][x]).mean()
            if gray < 90:
                board[y][x] = "X"
            elif gray > 180:
                board[y][x] = "O"
            else:
                board[y][x] = " "
def test_6():
    previous_board = [
        [" ", " ", " "],
        [" ", " ", " "],
        [" ", " ", " "],
    ]
    from_y, from_x, to_y, to_x = 0, 0, 0, 0
    while True:
        # 捕获图像并更新棋盘状态
        print("test_6")
        img_gray = sensor.snapshot().to_grayscale()  # 假设使用 OpenMV 捕获灰度图像
        x_max = 80
        y_max = 20
        w_max = 166
        h_max = 168
        row = 0
        rois = generate_centered_rois(x_max,y_max,w_max,h_max,10)
        for y in range(len(rois)):
            for x in range(len(rois[y])):
                gray = img_gray.get_statistics(roi=rois[y][x]).mean()
                if gray < 90:
                    board[y][x] = "X"
                elif gray > 180:
                    board[y][x] = "O"
                else:
                    board[y][x] = " "
        # 检测棋子移动
        img.draw_rectangle(x_max,y_max,w_max,h_max, color=(1, 147, 230), thickness=3)  # 绘制线段
        move = detect_move(board, previous_board)
        if move:
            (from_y, from_x), (to_y, to_x) = move
            print(f"棋子从 ({[(2 - from_y) * 3 + (2 - from_x) + 1]}) 移动到 ({[(2 - to_y) * 3 + (2 - to_x) + 1]})")
        # 更新上一帧的棋盘状态
        img.draw_cross(int(rois[from_y][from_x][0]+10), int(rois[from_y][from_x][1]+10), size=20, color=1)
        img.draw_cross(int(rois[to_y][to_x][0]+10), int(rois[to_y][to_x][1]+10), size=20, color=0)
        Display.show_image(img, x=int((DISPLAY_WIDTH - picture_width) / 2), y=int((DISPLAY_HEIGHT - picture_height) / 2))
        previous_board = [row.copy() for row in board]
        uart.write(bytes([0xAA]))
        uart.write(bytes([(2 - to_y) * 3 + (2 - to_x) + 1]))
        uart.write(bytes([(2 - from_y) * 3 + (2 - from_x) + 1]))
        uart.write(bytes([0x55]))
        time.sleep_ms(1)
        # 延时
        time.sleep(1)  # 延时 100ms
def detect_move(current_board, previous_board):
    """
    检测棋子移动。
    :param current_board: 当前棋盘状态
    :param previous_board: 上一帧棋盘状态
    :return: 移动的起始位置和目标位置，如果没有移动则返回 None
    """
    from_pos = None  # 棋子移动的起始位置
    to_pos = None    # 棋子移动的目标位置

    # 遍历棋盘
    for y in range(len(current_board)):
        for x in range(len(current_board[y])):
            # 如果当前位置从非空变为空，说明棋子被移走
            if previous_board[y][x] != " " and current_board[y][x] == " ":
                from_pos = (y, x)
            # 如果当前位置从空变为非空，说明棋子被放置
            elif previous_board[y][x] == " " and current_board[y][x] != " ":
                to_pos = (y, x)

    # 如果检测到移动
    if from_pos and to_pos:
        return from_pos, to_pos
    else:
        return None
def check_win(board, player):
    # Check rows and columns
    for i in range(SIZE):
        if all(board[i][j] == player for j in range(SIZE)) or \
           all(board[j][i] == player for j in range(SIZE)):
            return True
    # Check diagonals
    if all(board[i][i] == player for i in range(SIZE)) or \
       all(board[i][SIZE - 1 - i] == player for i in range(SIZE)):
        return True
    return False

# 检查平局了吗
def check_draw(board):
    return all(board[i][j] != ' ' for i in range(SIZE) for j in range(SIZE))


# 计算策略得分
def minimax(board, depth, is_black_turn):
    # 定义黑棋和白棋的符号
    black = 'X'
    white = 'O'

    # 当前玩家和对手
    current_player = black if is_black_turn else white
    opponent = white if is_black_turn else black

    # 检查游戏是否结束
    if check_win(board, black):
        return (10 - depth, -1, -1)  # 黑棋获胜
    if check_win(board, white):
        return (depth - 10, -1, -1)  # 白棋获胜
    if check_draw(board):
        return (0, -1, -1)  # 平局

    # 检查当前玩家是否可以通过下一步直接获胜
    for i in range(SIZE):
        for j in range(SIZE):
            if board[i][j] == ' ':
                board[i][j] = current_player
                if check_win(board, current_player):
                    board[i][j] = ' '
                    return (9 - depth, i, j) if is_black_turn else (depth - 9, i, j)  # 优先选择胜利
                board[i][j] = ' '

    # 检查对手是否可以通过下一步直接获胜
    for i in range(SIZE):
        for j in range(SIZE):
            if board[i][j] == ' ':
                board[i][j] = opponent
                if check_win(board, opponent):
                    board[i][j] = ' '
                    return (depth - 9, i, j) if is_black_turn else (9 - depth, i, j)  # 优先阻止对手
                board[i][j] = ' '

    # 递归计算最佳得分
    if is_black_turn:
        best_score = float('-inf')
        best_move = (-1, -1)
        for i in range(SIZE):
            for j in range(SIZE):
                if board[i][j] == ' ':
                    board[i][j] = black
                    score, _, _ = minimax(board, depth + 1, False)
                    board[i][j] = ' '
                    if score > best_score:
                        best_score = score
                        best_move = (i, j)
        return best_score, best_move[0], best_move[1]
    else:
        best_score = float('inf')
        best_move = (-1, -1)
        for i in range(SIZE):
            for j in range(SIZE):
                if board[i][j] == ' ':
                    board[i][j] = white
                    score, _, _ = minimax(board, depth + 1, True)
                    board[i][j] = ' '
                    if score < best_score:
                        best_score = score
                        best_move = (i, j)
        return best_score, best_move[0], best_move[1]

# 计算下一步位置
def computer_move(board):
    if board == [
        [" "," "," "],
        [" "," "," "],
        [" "," "," "]
    ]:
        return 1, 1

    # 统计棋盘上黑棋和白棋的数量
    black_count = sum(row.count('X') for row in board)
    white_count = sum(row.count('O') for row in board)

    # 判断当前是黑棋还是白棋的回合
    is_black_turn = black_count == white_count  # 黑棋先下，数量相等时轮到黑棋

    # 调用 minimax 函数
    score, i, j = minimax(board, 0, is_black_turn)
    print(f"Computer places {'X' if is_black_turn else 'O'} at ({i}, {j})")
    return i, j
def check_turn(board):
    x_count = sum(row.count("X") for row in board)
    o_count = sum(row.count("O") for row in board)
    return "X" if x_count == o_count else "O"
def generate_centered_rois(x, y, w, h, k):
    rois = []

    # 计算每个 ROI 区域之间的间距
    b = max(w, h) // 3

    # 计算整个 3x3 矩阵的宽度和高度
    total_width = 3 * b
    total_height = 3 * b

    # 计算左上角的起始点，使矩阵居中
    start_x = x + (w - total_width) // 2
    start_y = y + (h - total_height) // 2

    # 生成 3x3 ROI 矩阵
    for i in range(3):
        row = []
        for j in range(3):
            # 计算 ROI 的中心点坐标
            x_center = start_x + j * b + b // 2
            y_center = start_y + i * b + b // 2

            # 计算 ROI 的左上角坐标
            x_roi = x_center - k // 2
            y_roi = y_center - k // 2

            # 检查 ROI 是否超出矩形范围
            if x_roi < x:
                x_roi = x  # 对齐到矩形左边界
            if y_roi < y:
                y_roi = y  # 对齐到矩形上边界
            if x_roi + k > x + w:
                x_roi = x + w - k  # 对齐到矩形右边界
            if y_roi + k > y + h:
                y_roi = y + h - k  # 对齐到矩形下边界

            # 将 ROI 添加到当前行
            row.append((x_roi, y_roi, k, k))
        # 将当前行添加到 ROI 矩阵
        rois.append(row)

    return rois
def calculate_midpoints(corner):
    """
    将矩形分成 9 宫格，并返回每个格子的中点坐标。
    :param corner: 四个顶点的列表，每个顶点是一个 (x, y) 元组。
    :return: 一个包含 9 个中点坐标的列表，每个中点是一个 (x, y) 元组。
    """
    # 提取矩形的四个顶点
    (x1, y1), (x2, y2), (x3, y3), (x4, y4) = corner

    # 计算矩形的边界
    min_x = min(x1, x2, x3, x4)
    max_x = max(x1, x2, x3, x4)
    min_y = min(y1, y2, y3, y4)
    max_y = max(y1, y2, y3, y4)

    # 计算水平和垂直方向上的分割点
    delta_x = (max_x - min_x) / 3
    delta_y = (max_y - min_y) / 3

    # 计算 9 个格子的中点
    midpoints = []
    for i in range(3):  # 行
        for j in range(3):  # 列
            # 当前格子的中点坐标
            mid_x = min_x + (j + 0.5) * delta_x
            mid_y = min_y + (i + 0.5) * delta_y
            midpoints.append((mid_x, mid_y))

    return midpoints

# 显示模式选择：可以是 "VIRT"、"LCD" 或 "HDMI"
DISPLAY_MODE = "LCD"

# 根据模式设置显示宽高
if DISPLAY_MODE == "VIRT":
    # 虚拟显示器模式
    DISPLAY_WIDTH = ALIGN_UP(1920, 16)
    DISPLAY_HEIGHT = 1080
elif DISPLAY_MODE == "LCD":
    # 3.1寸屏幕模式
    DISPLAY_WIDTH = 800
    DISPLAY_HEIGHT = 480
elif DISPLAY_MODE == "HDMI":
    # HDMI扩展板模式
    DISPLAY_WIDTH = 1920
    DISPLAY_HEIGHT = 1080
else:
    raise ValueError("未知的 DISPLAY_MODE，请选择 'VIRT', 'LCD' 或 'HDMI'")

try:
    # 构造一个具有默认配置的摄像头对象
    sensor = Sensor(id=sensor_id)
    # 重置摄像头sensor
    sensor.reset()

    # 无需进行镜像翻转
    # 设置水平镜像
    # sensor.set_hmirror(False)
    # 设置垂直翻转
    # sensor.set_vflip(False)

    # 设置通道0的输出尺寸为1920x1080
    sensor.set_framesize(width=picture_width, height=picture_height, chn=CAM_CHN_ID_0)
    # 设置通道0的输出像素格式为RGB565
    sensor.set_pixformat(Sensor.RGB565, chn=CAM_CHN_ID_0)

    # 根据模式初始化显示器
    if DISPLAY_MODE == "VIRT":
        Display.init(Display.VIRT, width=DISPLAY_WIDTH, height=DISPLAY_HEIGHT, fps=60)
    elif DISPLAY_MODE == "LCD":
        Display.init(Display.ST7701, width=DISPLAY_WIDTH, height=DISPLAY_HEIGHT, to_ide=True)
    elif DISPLAY_MODE == "HDMI":
        Display.init(Display.LT9611, width=DISPLAY_WIDTH, height=DISPLAY_HEIGHT, to_ide=True)

    # 初始化媒体管理器
    MediaManager.init()
    # 启动传感器
    sensor.run()
    while True:
        os.exitpoint()
        # 捕获通道0的图像
        img = sensor.snapshot(chn=CAM_CHN_ID_0)
        img_gray = img.to_grayscale(copy = True)
        count = 0  # 初始化线段计数器'
        rois = []
        x_max = 80
        y_max = 30
        w_max = 166
        h_max = 178
        rois = generate_centered_rois(x_max,y_max,w_max,h_max,10)
        if button.value() == 1:
                test_6()
        else:
            for y in range(len(rois)):
                for x in range(len(rois[y])):
                    gray = img_gray.get_statistics(roi=rois[y][x]).mean()
                    print(gray)
                    if gray < 100:
                        board[y][x] = "X"
                    elif gray > 180:
                        board[y][x] = "O"
                    else:
                        board[y][x] = " "
            img.draw_rectangle(x_max,y_max,w_max,h_max, color=(1, 147, 230), thickness=3)  # 绘制线段
            for line in board:
                print(line)
            if check_win(board, 'O'):
                print("你赢啦!")
            elif check_win(board, 'X'):
                print("我赢啦！")
            elif check_draw(board):
                print("平局啦！")
            elif True:
                # 计算下一步棋子放在哪里
                line,row = computer_move(board)
                print(line)
                print(row)
                count_time = count_time +1
                print("count_time = "+ str(count_time))
                img.draw_cross(int(rois[line][row][0]+10), int(rois[line][row][1]+10), size=20, color=1)
                Display.show_image(img, x=int((DISPLAY_WIDTH - picture_width) / 2), y=int((DISPLAY_HEIGHT - picture_height) / 2))
                if count_time > 2:
                    uart.write(bytes([0xAA]))
                    uart.write(bytes([(2-line)*3+(2-row)+1]))
                    uart.write(bytes([0x55]))
                    time.sleep_ms(1)
                    print("good")
                    count_time = 0
except KeyboardInterrupt as e:
    print("用户停止: ", e)
except BaseException as e:
    print(f"异常: {e}")
finally:
    # 停止传感器运行
    if isinstance(sensor, Sensor):
        sensor.stop()
    # 反初始化显示模块
    Display.deinit()
    os.exitpoint(os.EXITPOINT_ENABLE_SLEEP)
    time.sleep_ms(100)
    # 释放媒体缓冲区
    MediaManager.deinit()
