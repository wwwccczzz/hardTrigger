# 单台海康CT相机硬触发测试

这是一个独立的 STM32F103ZET6 测试工程，适用于正点原子F103ZE开发板，不使用 `LIV_handhold` 的源代码。启动文件、CMSIS头文件和系统时钟文件来自本机安装的 Keil STM32F1设备包，触发程序为独立编写的寄存器级实现。

## 输出信号

- 输出引脚：PA1，也就是 TIM2_CH2。
- 默认模式：10 Hz，周期100 ms，高电平100 us。
- 正式模式：125 Hz，周期8 ms，高电平100 us。
- 输出极性：正常低电平，每周期开始产生一个高脉冲。

默认先测试10 Hz。在 `src/main.c` 中将下面的宏改为 `0U`，重新编译即可切换到125 Hz：

```c
#define CAMERA_TRIGGER_TEST_10HZ 0U
```

## 编译

### 用 Keil 打开和下载

1. 双击 `cameraHardTrigger.uvprojx` 打开工程。
2. 点击工具栏 `Build`，应显示 `0 Error(s), 0 Warning(s)`。
3. STM32通过ST-Link连接电脑，Keil中进入 `Options for Target -> Debug`，选择 `ST-Link Debugger`。
4. 在 `Utilities` 中同样选择 `ST-Link Debugger`，确认Flash算法为STM32F10x 512 KB。
5. 点击 `Download` 将程序烧录到STM32，然后按一次板载RESET。

该工程已按STM32F103ZET6、512 KB Flash、64 KB RAM配置，并使用高密度器件启动文件。程序按正点原子F103ZE板常见的8 MHz外部晶振将系统时钟配置为72 MHz。

程序烧录后可以拔掉ST-Link。正式运行时使用正点原子板载供电接口给开发板供电；也可以从稳定5 V电源接入板上标明的5 V和GND端子。具体使用哪个USB或DC插座应以你的正点原子板型号和丝印为准，禁止把12 V触发电源直接接到PA1、5 V或3.3 V引脚。

### 用批处理编译

双击 `build.bat`，或者在命令行运行：

```powershell
E:\cameraHardTrigger\build.bat
```

成功后生成：

```text
build\camera_hard_trigger.axf
build\camera_hard_trigger.hex
build\camera_hard_trigger.bin
```

## 单相机接线

```text
12V+ -> 1k电阻 -> 相机M8 pin2黄色Line0+
相机M8 pin3白黄Line0- -> NPN集电极C
NPN发射极E -> 12V-
STM32 GND -> 12V-
STM32 PA1 -> 2.2k电阻 -> NPN基极B
NPN基极B -> 10k电阻 -> GND
相机通过USB供电和传图
相机M8 pin1红线、pin8黑线均不连接
```

## MVS设置

```text
Acquisition Mode = Continuous
Trigger Selector = Frame Burst Start
Trigger Mode = On
Trigger Source = Line 0
Trigger Activation = Rising Edge
Acquisition Burst Frame Count = 1
Trigger Delay = 0 us
Trigger Cache Enable = Off
Line Debouncer Time = 0 us
Exposure Mode = Timed
Exposure Auto = Off
Exposure Time = 500 us
Gain Auto = Off
Acquisition Frame Rate Enable = Off
```

## 测试顺序

1. 相机暂不接M8，用示波器或逻辑分析仪测量PA1，应为10 Hz、100 us高脉冲。
2. 全部断电后连接NPN触发电路和一台相机的M8 pin2、pin3。
3. 相机USB连接电脑，在MVS设置参数并开始采集。
4. 最后给STM32上电或按RESET。
5. 运行60秒，10 Hz模式应收到约600帧，帧号应连续。
6. 将模式宏改为`0U`，重新编译和烧录；125 Hz运行60秒应收到约7500帧。

MVS开始取流后，如果画面停住并等待，是外触发模式的正常现象；STM32开始输出脉冲后才会逐帧更新。若完全不更新，先在MVS里把`Trigger Source`临时改成`Software`并点击软件触发，确认相机参数本身可用，再检查PA1和NPN接线。

## 安全要求

- 12V不能连接STM32 PA1、5V或3.3V。
- 12V必须经过1k电阻才能进入相机Line0+。
- 相机继续使用USB供电，M8 pin1红线不接。
- 每次改变接线前必须关闭12V、STM32和相机USB供电。

## 电脑端单相机硬触发验证程序

`pc_test`中提供了使用本机MVS SDK 4.8.1.2编写的独立验证程序。它会直接配置相机并进行两阶段对照：

1. STM32关闭时测量3秒，正确结果为0帧。
2. STM32输出触发脉冲时测量10秒，10 Hz模式应收到约100帧。
3. 程序同时检查相机帧号是否连续。

运行前必须停止MVS取流并关闭MVS客户端，否则SDK无法独占打开相机。

默认10 Hz验证直接双击：

```text
E:\cameraHardTrigger\pc_test\run_10hz_test.bat
```

切换STM32固件到125 Hz以后，双击：

```text
E:\cameraHardTrigger\pc_test\run_125hz_test.bat
```

程序将自动调用VS2022编译器和MVS SDK。如果运行正常，最后会打印：

```text
[通过] 单相机Line0硬触发链路正常。
```
