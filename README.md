# ESP32-OpenMV-car-2021-
主控为ESP32，摄像头为OpenMV写了双功能，在行驶途中可以巡线模式与数字识别模式之间的切换，底层用了freeRTOS(ESP32为freeRTOS操作系统模拟arduino环境)，可以多线程运行多个任务

The main controller is ESP32, and the camera is designed with dual functions of OpenMV. It can switch between patrol mode and digital recognition mode during driving. The underlying layer uses freeRTOS (ESP32 simulates Arduino environment with freeRTOS operating system), which can run multiple tasks with multiple threads.
