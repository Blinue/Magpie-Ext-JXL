#include "pch.h"
#include <jxl/codestream_header.h>
#include <jxl/color_encoding.h>
#include <jxl/encode.h>
#include <jxl/encode_cxx.h>
#include <jxl/thread_parallel_runner.h>
#include <jxl/thread_parallel_runner_cxx.h>
#include <jxl/types.h>
#include <sstream>
#include <wrl/client.h>
#include <wincodec.h>
#include <fmt/printf.h>

using Microsoft::WRL::ComPtr;

BOOL APIENTRY DllMain(
	HMODULE /*hModule*/,
	DWORD  reasonForCall,
	LPVOID /*lpReserved*/
) {
	switch (reasonForCall) {
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

static bool LoadFromFile(
	const wchar_t* fileName,
	uint32_t& width,
	uint32_t& height,
	std::vector<uint8_t>& pixels
) noexcept {
	ComPtr<IWICImagingFactory2> wicImgFactory;
	HRESULT hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&wicImgFactory)
	);
	if (FAILED(hr)) {
		return false;
	}

	// 读取图像文件
	ComPtr<IWICBitmapDecoder> decoder;
	hr = wicImgFactory->CreateDecoderFromFilename(
		fileName, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, decoder.GetAddressOf());
	if (FAILED(hr)) {
		return false;
	}

	ComPtr<IWICBitmapFrameDecode> frame;
	hr = decoder->GetFrame(0, frame.GetAddressOf());
	if (FAILED(hr)) {
		return false;
	}

	WICPixelFormatGUID sourceWicFormat;
	hr = frame->GetPixelFormat(&sourceWicFormat);
	if (FAILED(hr)) {
		return false;
	}

	
	ComPtr<IWICBitmapSource> source;
	if (sourceWicFormat == GUID_WICPixelFormat64bppRGBAHalf) {
		source = std::move(frame);
	} else {
		// 转换格式
		ComPtr<IWICFormatConverter> formatConverter;
		hr = wicImgFactory->CreateFormatConverter(formatConverter.GetAddressOf());
		if (FAILED(hr)) {
			return false;
		}

		hr = formatConverter->Initialize(frame.Get(), GUID_WICPixelFormat64bppRGBAHalf,
			WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom);
		if (FAILED(hr)) {
			return false;
		}

		source = std::move(formatConverter);
	}

	source->GetSize(&width, &height);

	uint32_t stride = width * 8;
	uint32_t size = stride * height;
	pixels.resize(size);
	hr = source->CopyPixels(nullptr, stride, size, pixels.data());
	if (FAILED(hr)) {
		auto e = fmt::sprintf("HRESULT: 0x%X", hr);
		return false;
	}

	return true;
}

/**
 * Compresses the provided pixels.
 *
 * @param pixels input pixels
 * @param xsize width of the input image
 * @param ysize height of the input image
 * @param compressed will be populated with the compressed bytes
 */
bool EncodeJxlOneshot(const std::vector<uint8_t>& pixels, const uint32_t xsize,
	const uint32_t ysize, std::vector<uint8_t>* compressed) {
	JxlEncoderPtr enc = JxlEncoderMake(/*memory_manager=*/nullptr);
	JxlThreadParallelRunnerPtr runner = JxlThreadParallelRunnerMake(
		/*memory_manager=*/nullptr,
		JxlThreadParallelRunnerDefaultNumWorkerThreads());
	if (JXL_ENC_SUCCESS != JxlEncoderSetParallelRunner(enc.get(),
		JxlThreadParallelRunner,
		runner.get())) {
		fprintf(stderr, "JxlEncoderSetParallelRunner failed\n");
		return false;
	}

	JxlBasicInfo basic_info;
	JxlEncoderInitBasicInfo(&basic_info);
	basic_info.xsize = xsize;
	basic_info.ysize = ysize;
	basic_info.bits_per_sample = 16;
	basic_info.exponent_bits_per_sample = 5;
	basic_info.num_extra_channels = 1;
	basic_info.alpha_bits = 16;
	basic_info.uses_original_profile = JXL_FALSE;
	if (JXL_ENC_SUCCESS != JxlEncoderSetBasicInfo(enc.get(), &basic_info)) {
		fprintf(stderr, "JxlEncoderSetBasicInfo failed\n");
		return false;
	}

	JxlColorEncoding color_encoding = {};
	JxlColorEncodingSetToLinearSRGB(&color_encoding, false);
	if (JXL_ENC_SUCCESS !=
		JxlEncoderSetColorEncoding(enc.get(), &color_encoding)) {
		fprintf(stderr, "JxlEncoderSetColorEncoding failed\n");
		return false;
	}

	JxlEncoderFrameSettings* frame_settings =
		JxlEncoderFrameSettingsCreate(enc.get(), nullptr);
	if (!frame_settings) {
		fprintf(stderr, "JxlEncoderFrameSettingsCreate failed\n");
		return false;
	}

	JxlPixelFormat pixel_format = { 4, JXL_TYPE_FLOAT16, JXL_NATIVE_ENDIAN, 0 };
	auto s = JxlEncoderAddImageFrame(frame_settings, &pixel_format,
		static_cast<const void*>(pixels.data()),
		pixels.size());
	if (JXL_ENC_SUCCESS != s) {
		fprintf(stderr, "JxlEncoderAddImageFrame failed\n");
		return false;
	}
	JxlEncoderCloseInput(enc.get());

	compressed->resize(64);
	uint8_t* next_out = compressed->data();
	size_t avail_out = compressed->size() - (next_out - compressed->data());
	JxlEncoderStatus process_result = JXL_ENC_NEED_MORE_OUTPUT;
	while (process_result == JXL_ENC_NEED_MORE_OUTPUT) {
		process_result = JxlEncoderProcessOutput(enc.get(), &next_out, &avail_out);
		if (process_result == JXL_ENC_NEED_MORE_OUTPUT) {
			size_t offset = next_out - compressed->data();
			compressed->resize(compressed->size() * 2);
			next_out = compressed->data() + offset;
			avail_out = compressed->size() - offset;
		}
	}
	compressed->resize(next_out - compressed->data());
	if (JXL_ENC_SUCCESS != process_result) {
		fprintf(stderr, "JxlEncoderProcessOutput failed\n");
		return false;
	}

	return true;
}

/**
 * Writes bytes to file.
 */
bool WriteFile(const std::vector<uint8_t>& bytes, const char* filename) {
	FILE* file;
	fopen_s(&file, filename, "wb");
	if (!file) {
		fprintf(stderr, "Could not open %s for writing\n", filename);
		return false;
	}
	if (fwrite(bytes.data(), sizeof(uint8_t), bytes.size(), file) !=
		bytes.size()) {
		fprintf(stderr, "Could not write bytes to %s\n", filename);
		fclose(file);
		return false;
	}
	if (fclose(file) != 0) {
		fprintf(stderr, "Could not close %s\n", filename);
		return false;
	}
	return true;
}

int APIENTRY wWinMain(
	_In_ HINSTANCE /*hInstance*/,
	_In_opt_ HINSTANCE /*hPrevInstance*/,
	_In_ wchar_t* /*lpCmdLine*/,
	_In_ int /*nCmdShow*/
) {
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	uint32_t width, height;
	std::vector<uint8_t> pixels;
	LoadFromFile(L"", width, height, pixels);
		
	std::vector<uint8_t> compressed;
	if (!EncodeJxlOneshot(pixels, width, height, &compressed)) {
		fprintf(stderr, "Couldn't encode jxl\n");
		return 3;
	}

	if (!WriteFile(compressed, "test.jxl")) {
		fprintf(stderr, "Couldn't write jxl file\n");
		return 4;
	}


}