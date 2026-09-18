#include <Windows.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include "MvCameraControl.h"

struct FrameMeasurement
{
	std::uint64_t frameCount = 0; // 测量窗口内成功收到的图像帧数。
	std::uint64_t timeoutCount = 0; // 测量窗口内等待图像超时的次数。
	std::uint64_t missingFrameCount = 0; // 根据相机帧号推算的中间缺帧数。
	unsigned int firstFrameNumber = 0; // 测量窗口内第一帧的相机帧号。
	unsigned int lastFrameNumber = 0; // 测量窗口内最后一帧的相机帧号。
	double elapsedSeconds = 0.0; // 实际测量时间，单位为秒。
};

/**
 * @brief 以十六进制形式打印MVS接口错误码，便于对应SDK文档排查。
 * @note 创建时间：2026-09-14-16。
 */
static void print_mvs_error(const char* operation, int errorCode)
{
	std::cerr << "[失败] " << operation << "，错误码=0x"
		<< std::hex << std::uppercase << static_cast<unsigned int>(errorCode)
		<< std::dec << std::nouppercase << '\n';
}

/**
 * @brief 设置必须成功的枚举型相机参数，失败时输出节点和值。
 * @note 创建时间：2026-09-14-16。
 */
static bool set_required_enum(void* cameraHandle, const char* nodeName, const char* valueName)
{
	const int result = MV_CC_SetEnumValueByString(cameraHandle, nodeName, valueName); // 当前枚举节点的设置结果。
	if (result != MV_OK)
	{
		std::cerr << "[失败] 设置 " << nodeName << " = " << valueName << '\n';
		print_mvs_error(nodeName, result);
		return false;
	}

	std::cout << "[参数] " << nodeName << " = " << valueName << '\n';
	return true;
}

/**
 * @brief 设置必须成功的浮点型相机参数，失败时输出节点和值。
 * @note 创建时间：2026-09-14-16。
 */
static bool set_required_float(void* cameraHandle, const char* nodeName, float value)
{
	const int result = MV_CC_SetFloatValue(cameraHandle, nodeName, value); // 当前浮点节点的设置结果。
	if (result != MV_OK)
	{
		std::cerr << "[失败] 设置 " << nodeName << " = " << value << '\n';
		print_mvs_error(nodeName, result);
		return false;
	}

	std::cout << "[参数] " << nodeName << " = " << value << '\n';
	return true;
}

/**
 * @brief 设置必须成功的整数型相机参数，失败时输出节点和值。
 * @note 创建时间：2026-09-14-16。
 */
static bool set_required_integer(void* cameraHandle, const char* nodeName, std::int64_t value)
{
	const int result = MV_CC_SetIntValueEx(cameraHandle, nodeName, value); // 当前整数节点的设置结果。
	if (result != MV_OK)
	{
		std::cerr << "[失败] 设置 " << nodeName << " = " << value << '\n';
		print_mvs_error(nodeName, result);
		return false;
	}

	std::cout << "[参数] " << nodeName << " = " << value << '\n';
	return true;
}

/**
 * @brief 尝试设置布尔型辅助参数，不支持该节点时给出提示并继续测试。
 * @note 创建时间：2026-09-14-16。
 */
static void set_optional_boolean(void* cameraHandle, const char* nodeName, bool value)
{
	const int result = MV_CC_SetBoolValue(cameraHandle, nodeName, value); // 当前可选布尔节点的设置结果。
	if (result == MV_OK)
	{
		std::cout << "[参数] " << nodeName << " = " << (value ? "true" : "false") << '\n';
		return;
	}

	std::cout << "[提示] 相机未接受可选参数 " << nodeName
		<< "，继续验证，错误码=0x" << std::hex << std::uppercase
		<< static_cast<unsigned int>(result) << std::dec << std::nouppercase << '\n';
}

/**
 * @brief 将CT相机配置为Line0上升沿、一次触发一帧的硬触发模式。
 * @note 创建时间：2026-09-14-16。
 */
static bool configure_hardware_trigger(void* cameraHandle)
{
	if (!set_required_enum(cameraHandle, "TriggerMode", "Off"))
	{
		return false;
	}
	if (!set_required_enum(cameraHandle, "AcquisitionMode", "Continuous"))
	{
		return false;
	}
	if (!set_required_enum(cameraHandle, "TriggerSelector", "FrameBurstStart"))
	{
		return false;
	}
	if (!set_required_integer(cameraHandle, "AcquisitionBurstFrameCount", 1))
	{
		return false;
	}
	if (!set_required_enum(cameraHandle, "TriggerSource", "Line0"))
	{
		return false;
	}
	if (!set_required_enum(cameraHandle, "TriggerActivation", "RisingEdge"))
	{
		return false;
	}
	if (!set_required_float(cameraHandle, "TriggerDelay", 0.0F))
	{
		return false;
	}
	if (!set_required_enum(cameraHandle, "LineSelector", "Line0"))
	{
		return false;
	}
	if (!set_required_integer(cameraHandle, "LineDebouncerTime", 0))
	{
		return false;
	}
	if (!set_required_enum(cameraHandle, "ExposureMode", "Timed"))
	{
		return false;
	}
	if (!set_required_enum(cameraHandle, "ExposureAuto", "Off"))
	{
		return false;
	}
	if (!set_required_float(cameraHandle, "ExposureTime", 500.0F))
	{
		return false;
	}
	if (!set_required_enum(cameraHandle, "GainAuto", "Off"))
	{
		return false;
	}

	set_optional_boolean(cameraHandle, "TriggerCacheEnable", false);
	set_optional_boolean(cameraHandle, "AcquisitionFrameRateEnable", false);

	return set_required_enum(cameraHandle, "TriggerMode", "On");
}

/**
 * @brief 打印USB相机型号、序列号和用户名称，确认程序打开的是目标设备。
 * @note 创建时间：2026-09-14-16。
 */
static void print_usb_camera_info(const MV_CC_DEVICE_INFO* deviceInfo, unsigned int index)
{
	const MV_USB3_DEVICE_INFO& usbInfo = deviceInfo->SpecialInfo.stUsb3VInfo; // 当前USB3 Vision相机的设备描述。
	std::cout << "[相机 " << index << "] 型号="
		<< reinterpret_cast<const char*>(usbInfo.chModelName)
		<< "，序列号=" << reinterpret_cast<const char*>(usbInfo.chSerialNumber)
		<< "，用户名称=" << reinterpret_cast<const char*>(usbInfo.chUserDefinedName) << '\n';
}

/**
 * @brief 清空输入行并等待操作者按回车，使断触发和有触发阶段边界明确。
 * @note 创建时间：2026-09-14-16。
 */
static void wait_for_enter()
{
	std::string inputLine; // 接收操作者回车输入的临时字符串。
	std::getline(std::cin, inputLine);
}

/**
 * @brief 在指定时间内取图，统计帧数、超时数和相机帧号缺口。
 * @note 创建时间：2026-09-14-16。
 */
static FrameMeasurement measure_frames(void* cameraHandle, double durationSeconds)
{
	FrameMeasurement measurement; // 本次测量累计的帧统计结果。
	const auto startTime = std::chrono::steady_clock::now(); // 测量窗口开始的单调时钟时间。
	const auto endTime = startTime + std::chrono::duration<double>(durationSeconds); // 测量窗口结束时间。
	unsigned int previousFrameNumber = 0; // 上一次收到的相机帧号，用于计算帧号缺口。

	while (std::chrono::steady_clock::now() < endTime)
	{
		MV_FRAME_OUT frame = {}; // SDK内部图像缓存及本帧元数据。
		const int result = MV_CC_GetImageBuffer(cameraHandle, &frame, 200); // 等待一帧图像，最多阻塞200毫秒。
		if (result == MV_OK)
		{
			const unsigned int frameNumber = frame.stFrameInfo.nFrameNum; // 当前图像的相机硬件帧号。
			if (measurement.frameCount == 0)
			{
				measurement.firstFrameNumber = frameNumber;
			}
			if (previousFrameNumber != 0 && frameNumber > previousFrameNumber + 1U)
			{
				measurement.missingFrameCount += frameNumber - previousFrameNumber - 1U;
			}

			previousFrameNumber = frameNumber;
			measurement.lastFrameNumber = frameNumber;
			++measurement.frameCount;

			const int freeResult = MV_CC_FreeImageBuffer(cameraHandle, &frame); // 归还SDK图像缓存，避免缓存节点耗尽。
			if (freeResult != MV_OK)
			{
				print_mvs_error("MV_CC_FreeImageBuffer", freeResult);
				break;
			}
		}
		// MVS错误常量采用无符号十六进制定义，统一转换后比较以兼容MinGW的严格符号检查。
		else if (static_cast<unsigned int>(result) == MV_E_NODATA)
		{
			++measurement.timeoutCount;
		}
		else
		{
			print_mvs_error("MV_CC_GetImageBuffer", result);
			break;
		}
	}

	const auto finishTime = std::chrono::steady_clock::now(); // 测量实际结束的单调时钟时间。
	measurement.elapsedSeconds = std::chrono::duration<double>(finishTime - startTime).count();
	return measurement;
}

/**
 * @brief 打印一个测量阶段的帧率和帧号统计。
 * @note 创建时间：2026-09-14-16。
 */
static void print_measurement(const char* phaseName, const FrameMeasurement& measurement)
{
	const double measuredFps = measurement.elapsedSeconds > 0.0
		? static_cast<double>(measurement.frameCount) / measurement.elapsedSeconds
		: 0.0; // 按实际测量时长计算的接收帧率。
	std::cout << std::fixed << std::setprecision(2)
		<< "[" << phaseName << "] 时间=" << measurement.elapsedSeconds
		<< " s，收到=" << measurement.frameCount
		<< " 帧，帧率=" << measuredFps
		<< " fps，超时=" << measurement.timeoutCount
		<< " 次，帧号缺口=" << measurement.missingFrameCount << '\n';
}

/**
 * @brief 执行单台海康USB相机的断触发和有触发对照测试。
 * @note 创建时间：2026-09-14-16。默认验证10 Hz，可用第一个参数指定期望频率。
 */
int main(int argumentCount, char* argumentValues[])
{
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	const double expectedHz = argumentCount >= 2 ? std::atof(argumentValues[1]) : 10.0; // STM32应输出的期望触发频率。
	const double activeSeconds = argumentCount >= 3 ? std::atof(argumentValues[2]) : 10.0; // 有触发阶段的测量时长。
	if (expectedHz <= 0.0 || activeSeconds <= 0.0)
	{
		std::cerr << "用法：hikrobot_trigger_verify.exe [期望频率Hz] [测试秒数]\n";
		return 2;
	}

	int exitCode = 1; // 程序最终返回码，只有全部验证通过时改为0。
	void* cameraHandle = nullptr; // MVS SDK相机句柄，由创建到销毁统一管理。
	bool sdkInitialized = false; // 标记SDK是否已经初始化，控制退出时反初始化。
	bool deviceOpened = false; // 标记相机是否已经打开，控制退出时关闭设备。
	bool grabbingStarted = false; // 标记取流是否已经启动，控制退出时停止取流。

	const int initializeResult = MV_CC_Initialize(); // 初始化MVS SDK的结果。
	if (initializeResult != MV_OK)
	{
		print_mvs_error("MV_CC_Initialize", initializeResult);
		return exitCode;
	}
	sdkInitialized = true;

	do
	{
		MV_CC_DEVICE_INFO_LIST deviceList = {}; // 枚举得到的USB相机列表。
		const int enumerateResult = MV_CC_EnumDevices(MV_USB_DEVICE, &deviceList); // 仅枚举USB3 Vision相机，避免误开网口设备。
		if (enumerateResult != MV_OK)
		{
			print_mvs_error("MV_CC_EnumDevices", enumerateResult);
			break;
		}
		if (deviceList.nDeviceNum == 0)
		{
			std::cerr << "[失败] 没有发现USB相机。请检查USB3线、驱动，并关闭MVS客户端。\n";
			break;
		}

		std::cout << "发现 " << deviceList.nDeviceNum << " 台USB相机，本次打开索引0。\n";
		for (unsigned int index = 0; index < deviceList.nDeviceNum; ++index)
		{
			const MV_CC_DEVICE_INFO* deviceInfo = deviceList.pDeviceInfo[index]; // 当前枚举索引对应的设备信息指针。
			if (deviceInfo != nullptr)
			{
				print_usb_camera_info(deviceInfo, index);
			}
		}

		const int createResult = MV_CC_CreateHandle(&cameraHandle, deviceList.pDeviceInfo[0]); // 为第一台USB相机创建SDK句柄。
		if (createResult != MV_OK)
		{
			print_mvs_error("MV_CC_CreateHandle", createResult);
			break;
		}

		const int openResult = MV_CC_OpenDevice(cameraHandle); // 以SDK默认独占方式打开相机。
		if (openResult != MV_OK)
		{
			print_mvs_error("MV_CC_OpenDevice", openResult);
			std::cerr << "请先停止MVS取流并关闭MVS，再运行本程序。\n";
			break;
		}
		deviceOpened = true;

		if (!configure_hardware_trigger(cameraHandle))
		{
			break;
		}

		const int startResult = MV_CC_StartGrabbing(cameraHandle); // 启动等待Line0硬触发的取流状态。
		if (startResult != MV_OK)
		{
			print_mvs_error("MV_CC_StartGrabbing", startResult);
			break;
		}
		grabbingStarted = true;

		std::cout << "\n第一阶段：关闭STM32电源，使PA1停止输出，然后按回车。\n";
		wait_for_enter();
		const int clearIdleResult = MV_CC_ClearImageBuffer(cameraHandle); // 清除操作前可能残留在SDK队列中的图像。
		if (clearIdleResult != MV_OK)
		{
			print_mvs_error("MV_CC_ClearImageBuffer", clearIdleResult);
			break;
		}
		const FrameMeasurement idleMeasurement = measure_frames(cameraHandle, 3.0); // STM32无输出时的三秒对照数据。
		print_measurement("无触发阶段", idleMeasurement);

		std::cout << "\n第二阶段：给STM32上电并按RESET，确认PA1已接入NPN基极电阻，然后按回车。\n";
		wait_for_enter();
		const int clearActiveResult = MV_CC_ClearImageBuffer(cameraHandle); // 清除阶段切换时可能缓存的旧图像。
		if (clearActiveResult != MV_OK)
		{
			print_mvs_error("MV_CC_ClearImageBuffer", clearActiveResult);
			break;
		}
		const FrameMeasurement activeMeasurement = measure_frames(cameraHandle, activeSeconds); // STM32输出脉冲时的帧统计数据。
		print_measurement("硬触发阶段", activeMeasurement);

		const double measuredFps = activeMeasurement.elapsedSeconds > 0.0
			? static_cast<double>(activeMeasurement.frameCount) / activeMeasurement.elapsedSeconds
			: 0.0; // 有触发阶段的实际接收帧率。
		const double relativeError = std::fabs(measuredFps - expectedHz) / expectedHz; // 实测帧率相对期望频率的偏差。
		const bool idlePassed = idleMeasurement.frameCount == 0; // 无触发时零帧才说明相机确实在等待Line0。
		const bool activePassed = activeMeasurement.frameCount > 0 && relativeError <= 0.15; // 允许短测量窗口存在15%的帧率偏差。
		const bool frameSequencePassed = activeMeasurement.missingFrameCount == 0; // 相机帧号连续代表SDK侧没有观察到丢帧。

		std::cout << "\n========== 验证结论 ==========\n";
		std::cout << "无脉冲零帧：" << (idlePassed ? "通过" : "失败") << '\n';
		std::cout << "有脉冲帧率接近 " << expectedHz << " Hz：" << (activePassed ? "通过" : "失败") << '\n';
		std::cout << "帧号连续：" << (frameSequencePassed ? "通过" : "失败") << '\n';

		if (idlePassed && activePassed && frameSequencePassed)
		{
			std::cout << "[通过] 单相机Line0硬触发链路正常。\n";
			exitCode = 0;
		}
		else
		{
			std::cout << "[未通过] 请根据上面的阶段结果检查触发频率、接线或相机参数。\n";
		}
	} while (false);

	if (grabbingStarted)
	{
		const int stopResult = MV_CC_StopGrabbing(cameraHandle); // 停止SDK取流并释放相机采集资源。
		if (stopResult != MV_OK)
		{
			print_mvs_error("MV_CC_StopGrabbing", stopResult);
		}
	}
	if (deviceOpened)
	{
		const int closeResult = MV_CC_CloseDevice(cameraHandle); // 关闭相机设备连接。
		if (closeResult != MV_OK)
		{
			print_mvs_error("MV_CC_CloseDevice", closeResult);
		}
	}
	if (cameraHandle != nullptr)
	{
		const int destroyResult = MV_CC_DestroyHandle(cameraHandle); // 销毁MVS相机句柄。
		if (destroyResult != MV_OK)
		{
			print_mvs_error("MV_CC_DestroyHandle", destroyResult);
		}
	}
	if (sdkInitialized)
	{
		const int finalizeResult = MV_CC_Finalize(); // 反初始化MVS SDK全局资源。
		if (finalizeResult != MV_OK)
		{
			print_mvs_error("MV_CC_Finalize", finalizeResult);
		}
	}

	std::cout << "按回车退出。\n";
	wait_for_enter();
	return exitCode;
}
