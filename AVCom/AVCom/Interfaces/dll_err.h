#pragma once
#include <Windows.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" 
{
#endif // __cplusplus

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavcodec/bsf.h>
#include <libavdevice/avdevice.h>
#include <libswresample/swresample.h>
#include <cuda_runtime_api.h>
#include <cuda.h>

#ifdef __cplusplus
}
#endif // __cplusplus


static void FFMPEGLog(int line, char* file, int __avError)
{
#ifdef _DEBUG
	char av_buffer[500];
	av_strerror(__avError, av_buffer, sizeof(av_buffer));
	printf("\n \n%s in %s at line %d\n\n", av_buffer, file, line);
#endif // _DEBUG
}

static void Win32Log(int line, char* file, HRESULT errResult)
{
	if (errResult) {
#if ((defined(_WIN32) || defined(_WIN64)) && (defined(_DEBUG)))
		DWORD win32Code = ERROR_SUCCESS;
		char win32_buffer[500];
		if ((errResult & 0xFFFF0000) == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 0))
			win32Code = HRESULT_CODE(errResult);

		if (errResult == S_OK)
			win32Code = ERROR_SUCCESS;

		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			win32Code,
			LANG_NEUTRAL,
			win32_buffer,
			sizeof(win32_buffer),
			NULL);

		printf("\n\nWin32 Error: %s in %s at line %d\n\n", win32_buffer, file, line);
#endif // WIN32 || WIN && _DEBUG
	}
}

static void CUDALog(int line, char* file, CUresult cuResult)
{
	printf("\n\nCUDA error: %d, at line %d\n\n", cuResult, line);
}

#define ck(line, file, __avError) FFMPEGLog(line, file ,__avError);
#define WIN32ck(line, file, errResult) Win32Log(line, file, errResult);
#define CUDAck(line, file, cuResult) CUDALog(line, file, cuResult);