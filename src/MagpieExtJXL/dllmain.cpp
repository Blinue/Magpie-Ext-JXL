#include "pch.h"
#include "MagpieExtApi.h"
#include "Logger.h"
#include "StrHelper.h"
#include <jxl/codestream_header.h>
#include <jxl/color_encoding.h>
#include <jxl/encode.h>
#include <jxl/encode_cxx.h>
#include <jxl/thread_parallel_runner.h>
#include <jxl/thread_parallel_runner_cxx.h>
#include <jxl/types.h>

using namespace MagpieExtJXL;

MagpieHostApi* HostApi;

static bool WriteFile(HANDLE hFile, std::span<const uint8_t> data) noexcept {
	DWORD written;
	if (!::WriteFile(hFile, data.data(), (DWORD)data.size(), &written, nullptr)) {
		Logger::Get().Win32Error("WriteFile 失败");
		return false;
	}

	if (written < data.size()) {
		Logger::Get().Error("WriteFile 未写入全部数据");
		return false;
	}

	return true;
}

HRESULT EncodeJxl(
	const MagpieOptionValue* /*optionValues*/,
	uint32_t /*optionValueCount*/,
	const char* fileName,
	uint32_t imageWidth,
	uint32_t imageHeight,
	const uint8_t* pixelData,
	uint64_t pixelDataSize,
	uint32_t rowPitch
) noexcept {
	JxlEncoderPtr encoder = JxlEncoderMake(nullptr);
	JxlThreadParallelRunnerPtr runner = JxlThreadParallelRunnerMake(
		nullptr, JxlThreadParallelRunnerDefaultNumWorkerThreads());

	JxlEncoderStatus status =
		JxlEncoderSetParallelRunner(encoder.get(), JxlThreadParallelRunner, runner.get());
	if (status != JXL_ENC_SUCCESS) {
		Logger::Get().Error("JxlEncoderSetParallelRunner 失败");
		return E_FAIL;
	}

	JxlBasicInfo basicInfo;
	JxlEncoderInitBasicInfo(&basicInfo);
	basicInfo.xsize = imageWidth;
	basicInfo.ysize = imageHeight;
	basicInfo.bits_per_sample = 16;
	basicInfo.uses_original_profile = JXL_TRUE;
	status = JxlEncoderSetBasicInfo(encoder.get(), &basicInfo);
	if (status != JXL_ENC_SUCCESS) {
		Logger::Get().Error("JxlEncoderSetBasicInfo 失败");
		return E_FAIL;
	}

	JxlColorEncoding colorEncoding = {};
	JxlColorEncodingSetToSRGB(&colorEncoding, false);

	status = JxlEncoderSetColorEncoding(encoder.get(), &colorEncoding);
	if (status != JXL_ENC_SUCCESS) {
		Logger::Get().Error("JxlEncoderSetColorEncoding 失败");
		return E_FAIL;
	}

	JxlEncoderFrameSettings* frameSettings =
		JxlEncoderFrameSettingsCreate(encoder.get(), nullptr);
	if (!frameSettings) {
		Logger::Get().Error("JxlEncoderFrameSettingsCreate 失败");
		return E_FAIL;
	}

	status = JxlEncoderSetFrameLossless(frameSettings, JXL_TRUE);
	if (status != JXL_ENC_SUCCESS) {
		Logger::Get().Error("JxlEncoderSetFrameLossless 失败");
		return E_FAIL;
	}

	JxlPixelFormat pixelFormat = { 3, JXL_TYPE_UINT16, JXL_NATIVE_ENDIAN, rowPitch };
	status = JxlEncoderAddImageFrame(
		frameSettings,
		&pixelFormat,
		pixelData,
		pixelDataSize
	);
	if (status != JXL_ENC_SUCCESS) {
		Logger::Get().Error("JxlEncoderAddImageFrame 失败");
		return E_FAIL;
	}

	JxlEncoderCloseInput(encoder.get());

	wil::unique_hfile hFile(CreateFile2(
		StrHelper::UTF8ToUTF16(fileName).c_str(),
		GENERIC_WRITE,
		0,
		CREATE_ALWAYS,
		nullptr
	));
	if (!hFile) {
		Logger::Get().Win32Error("CreateFile2 失败");
		return E_FAIL;
	}

	std::vector<uint8_t> buffer(1024);
	uint8_t* nextOut = buffer.data();
	uint64_t availOut = buffer.size();
	while (true) {
		status = JxlEncoderProcessOutput(encoder.get(), &nextOut, &availOut);
		if (status == JXL_ENC_SUCCESS) {
			// 最后一次写入
			if (nextOut != buffer.data()) {
				WriteFile(hFile.get(), std::span(buffer.data(), nextOut - buffer.data()));
			}
			
			break;
		} else if (status != JXL_ENC_NEED_MORE_OUTPUT) {
			Logger::Get().Error("JxlEncoderProcessOutput 失败");
			return E_FAIL;
		}

		// 缓冲区满时写入文件
		if (nextOut == buffer.data() + buffer.size()) {
			WriteFile(hFile.get(), buffer);
			// 增加缓冲区容量
			buffer.resize(buffer.size() * 2);
			nextOut = buffer.data();
			availOut = buffer.size();
		}
	}

	return S_OK;
}

static void MagpieExtEntryCpp(MagpieHostApi* hostApi) {
	HostApi = hostApi;

	Logger::Get().Initialize(hostApi);

	MagpieImageEncoderDesc imageEncoderDesc = {
		.id = "jxl",
		.displayName = "JXL",
		.description = "JXL 是新一代图像格式",
		.fileExtension = "jxl",
		.encode = &EncodeJxl
	};

	MagpieExtensionDesc extDesc = {
		.displayName = "JXL Image Encoder",
		.author = "Blinue",
		.homePageUrl = "https://github.com/Blinue/Magpie-Ext-JXL",
		.description = "JXL 图像编码器",
		.imageEncoderDescs = &imageEncoderDesc,
		.imageEncoderCount = 1
	};

	hostApi->registerExtension(&extDesc);
}

extern "C" __declspec(dllexport) void MAGPIE_EXT_ENTRY(MagpieHostApi* hostApi) {
	MagpieExtEntryCpp(hostApi);
}
