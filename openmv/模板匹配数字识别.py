thresholds_red = [(100, 17, 29, 127, -128, 127)] #红色阈值
import sensor, image, time, os, tf, math, uos, gc
from pyb import LED
from pyb import UART
from pid import PID
from pyb import Pin
import pyb

rho_pid = PID(p=0.3, i=0.1)#y=kx+b  b截距 0.9
theta_pid = PID(p=0.1, i=0)#y=kx+b  k斜率  0.05

uart = UART(3, 115200) #初始化UART3串口  P4-TX P5-RX

LED(1).on()
LED(2).on()
LED(3).on()

#初始化摄像头
#sensor.reset()
#sensor.set_hmirror(1)            #水平翻转
#sensor.set_vflip(1)              #竖直翻转
#sensor.set_pixformat(sensor.RGB565)
#sensor.set_framesize(sensor.QVGA) #320x240
#sensor.set_windowing((240, 240))       # Set 240x240 window.
#sensor.skip_frames(time=2000)          # Let the camera adjust.

#数字识别--------------------------------------------------------------------------------------------
num_quantity = 8
num_model = []
for n in range(1,num_quantity+1):
    num_model.append(image.Image(str(n) + '.pgm'))

img_colorful = sensor.alloc_extra_fb(160,120,sensor.RGB565) #声明一个彩色画布，用于屏幕显示
img_to_matching = sensor.alloc_extra_fb(35,45,sensor.GRAYSCALE) #声明小尺寸画布，用于模板匹配
threshold_black = [(0,70)]

scale = 1 #缩放比例变量

#敏感区设置----------------------------------------------------------------------------------------


redRoi = (0,0,80,60)
leftArea = (0,15,12,15)
rightArea = (68,15,12,15)
forwardArea = (0,0,80,12)#(30,0,20,10)  (15,0,50,10)
#t1Area = (33,0,7,7)
#t2Area = (42,0,7,7)

#********************************************************************************
def Tnode():                    #是否为T型节点
    T_Blobs = img.find_blobs([(100,100)],roi=forwardArea,area_threshold=10, pixels_threshold=10)
    left_Blobs = img.find_blobs([(100,100)],roi=leftArea,area_threshold=10, pixels_threshold=10)
    right_Blobs = img.find_blobs([(100,100)],roi=rightArea,area_threshold=10, pixels_threshold=10)
    if (T_Blobs):
        for T_blob in T_Blobs:
            img.draw_rectangle(T_blob.rect())
            img.draw_cross(T_blob.cx(),T_blob.cy())
        T = 0 #敏感区有红线必不是T字形
    else:
        T = 1 #敏感区没有红线可能为T字形

    if(left_Blobs):
        left = 1
    else:
        left = 0

    if(right_Blobs):
        right = 1
    else:
        right = 0

    if(T == 1 and left == 1 and right == 1):
        return 1
    else:
        return 0

#********************************************************************************
def anode():
    left_Blobs = img.find_blobs([(100,100)],roi=leftArea,area_threshold=10, pixels_threshold=10)
    if (left_Blobs):
        return 1
    else:
        return 0
#********************************************************************************
def bnode():
    right_Blobs = img.find_blobs([(100,100)],roi=rightArea,area_threshold=10, pixels_threshold=10)
    if (right_Blobs):
        return 1
    else:
        return 0
#********************************************************************************
def SHInode():                   #是否为十字型节点
    T_Blobs = img.find_blobs([(100,100)],roi=forwardArea,area_threshold=10, pixels_threshold=10)
    left_Blobs = img.find_blobs([(100,100)],roi=leftArea,area_threshold=10, pixels_threshold=10)
    right_Blobs = img.find_blobs([(100,100)],roi=rightArea,area_threshold=10, pixels_threshold=10)
    if (T_Blobs):
        for T_blob in T_Blobs:
            img.draw_rectangle(T_blob.rect())
            img.draw_cross(T_blob.cx(),T_blob.cy())
        T = 0
    else:
        T = 1

    if(left_Blobs):
        left = 1
    else:
        left = 0

    if(right_Blobs):
        right = 1
    else:
        right = 0

    if(T == 0 and left == 1 and right == 1):
        return 1
    else:
        return 0
#********************************************************************************
def Freeze():
    Freeze_Blobs = img.find_blobs([(100,100)],roi=forwardArea,area_threshold=10, pixels_threshold=10)
    if(Freeze_Blobs):
        return 0
    else:
        return 1


cnt = 0
data = 0
mode = 0 #mode = 1 识别数字 mode = 2 巡线模式                            #一会儿去试试串口通信
SHI_flag = 0
T_flag = 0
isQQVGA = 0
isQQQVGA = 0
left_flag = 0
right_flag = 0
number_left = 0
number_right = 0
#Freeze_flag = 0

while True:

    if(uart.any()):
        data = uart.read()
        print(data) #test
        if(data == b'\x01' and cnt == 0):
            mode = b'\x01'
            cnt = 1
            isQQQVGA = 0 #进 数字识别模式 重置 巡线分辨率标志
        if(data == b'\x02' and cnt == 1):
            mode = b'\x02'
            cnt = 0
            isQQVGA = 0   #进 巡线模式 重置 数字识别分辨率标志


    if mode == b'\x01':

        if(isQQVGA == 0):
            LED(1).on()
            LED(2).on()
            LED(3).on()

            sensor.reset()
            sensor.set_hmirror(1)            #水平翻转
            sensor.set_vflip(1)              #竖直翻转
            sensor.set_pixformat(sensor.GRAYSCALE)
            sensor.set_framesize(sensor.QQVGA) #160*120
            sensor.set_contrast(3)

            isQQVGA = 1

        else:
            #------------------------------------------------------------------------------------------
            img = sensor.snapshot()
            img_colorful.draw_image(img,0,0)
            blobs = img.find_blobs(threshold_black)
            if blobs:
                for blob in blobs:
                    if blob.pixels() > 50 and 100 > blob.h() > 10 and blob.w() > 3 :#色块尺寸过滤
                        scale = 40 / blob.h() #通过色块尺寸，计算需要的缩放比例
                        img_to_matching.draw_image(img,0,0,roi=(blob.x()-2,blob.y()-2,blob.w()+4,blob.h()+4),x_scale=scale,y_scale=scale) #对找到的黑色色块进行调整与缩放
                        for n in range(0,num_quantity):
                            r = img_to_matching.find_template(num_model[n], 0.7, step=2, search=image.SEARCH_EX)
                            if (r and blob.cx() < 80): #如果匹配成功
                                number_left = n+1

                            if (r and blob.cx() > 80):
                                number_right = n+1

                            if (number_left != 0 and number_right != 0):
                                uart.write("O")
                                uart.write(str(0))
                                uart.write("S")
                                uart.write(str(0))
                                uart.write("T")
                                uart.write(str(0))
                #                    uart.write("F")
                #                    uart.write(str(0))
                                uart.write("a")
                                uart.write(str(0))
                                uart.write("b")
                                uart.write(str(0))
                                uart.write("A")
                                uart.write(str(number_left))
                                uart.write("B")
                                uart.write(str(number_right))
                                #mode = 2
                                print(number_left,number_right) #test
                                number_left = 0
                                number_right = 0
                                #print(str(labels[i]),mode)

    if mode == b'\x02':

        if(isQQQVGA == 0):
            #初始化摄像头
            LED(1).on()
            LED(2).on()
            LED(3).on()
            sensor.reset()
            sensor.set_hmirror(1)            #水平翻转
            sensor.set_vflip(1)              #竖直翻转
            sensor.set_pixformat(sensor.RGB565)
            sensor.set_framesize(sensor.QQQVGA) #80x60
#            sensor.skip_frames(time = 1000)
            isQQQVGA = 1

        else:
            img = sensor.snapshot().binary(thresholds_red)
            img.draw_rectangle(leftArea,(25,95,255))
            img.draw_rectangle(rightArea,(25,95,255))
            img.draw_rectangle(redRoi,(175,25,125))
            img.draw_rectangle(forwardArea,(25,95,255))
#            img.draw_rectangle(t1Area,(0,255,0))
#            img.draw_rectangle(t2Area,(0,255,0))

            line = img.get_regression([(100,100)],roi=redRoi, robust = True)#画线

            if (line):#如果找到了线
                rho_err = abs(line.rho())-img.width()/2 #计算线条的b与中心的相对距离
                if line.theta()>90:
                    theta_err = line.theta()-180
                else:
                    theta_err = line.theta()
                img.draw_line(line.line(), color = 127)


                if line.magnitude()>8:#8
                    rho_output = rho_pid.get_pid(rho_err,1)
                    theta_output = theta_pid.get_pid(theta_err,1)
                    output = rho_output + theta_output
                    #print(rho_output,theta_output)
                    if(output<0):
                        output = abs(output) + 100
                    OUTPUT = str(round(output))
                    if(SHInode()):
                        SHI_flag = 1
                        #mode = 1
                    else:
                        SHI_flag = 0
                    if(Tnode()):
                        T_flag = 1
                        #mode = 1
                    else:
                        T_flag = 0
#                    if(Freeze()):
#                        Freeze_flag = 1
#                    else:
#                        Freeze_flag = 0
                    if(anode()):
                        left_flag = 1
                    else:
                        left_flag = 0
                    if(bnode()):
                        right_flag = 1
                    else:
                        right_flag = 0
                    uart.write("O")
                    uart.write(OUTPUT)
                    uart.write("S")
                    uart.write(str(SHI_flag))
                    uart.write("T")
                    uart.write(str(T_flag))
#                    uart.write("F")
#                    uart.write(str(Freeze_flag))
                    uart.write("a")
                    uart.write(str(left_flag))
                    uart.write("b")
                    uart.write(str(right_flag))
                    uart.write("A")
                    uart.write(str(number_left))
                    uart.write("B")
                    uart.write(str(number_right))
                    #print(OUTPUT,SHI_flag,T_flag,left_flag,right_flag,number_left,number_right)

            else: #如果没找到线
                uart.write("O")
                uart.write(str(0))
                uart.write("S")
                uart.write(str(0))
                uart.write("T")
                uart.write(str(0))
#                uart.write("F")
#                uart.write(str(0))
                uart.write("a")
                uart.write(str(0))
                uart.write("b")
                uart.write(str(0))
                uart.write("A")
                uart.write(str(9))
                uart.write("B")
                uart.write(str(9))
                #print(0,0,0,0,0,9)
                #print(1,t1_flag,t2_flag)








