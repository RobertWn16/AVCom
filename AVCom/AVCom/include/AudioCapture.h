#ifndef _H_AUDIOCAPTURE_H_
#define _H_AUDIOCAPTURE_H_

#include "../Interfaces/IAudioCapture.h"
#pragma warning (disable : 4996)

#define XAUDIO2_AV_FLTP 2
#define XAUDIO2_AV_S16p 1

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
	HRESULT STDMETHODCALLTYPE audiocapture_QueryInterface(IAudioCapture*, REFIID, _COM_Outptr_ void**);
	HRESULT STDMETHODCALLTYPE audiocapture_AddRef(IAudioCapture*);
	HRESULT STDMETHODCALLTYPE audiocapture_Release(IAudioCapture*);
	HRESULT STDMETHODCALLTYPE audiocapture_CreateInstance(IAudioCapture*, REFIID, _COM_Outptr_ void**);

	HRESULT STDMETHODCALLTYPE audiocapture_XAudioCapture(IAudioCapture*, char*);
	HRESULT STDMETHODCALLTYPE audiocapture_initXAudio(IAudioCapture*);
	HRESULT STDMETHODCALLTYPE audiocapture_start(IAudioCapture*);
	HRESULT STDMETHODCALLTYPE audiocapture_play(IAudioCapture*);
	HRESULT STDMETHODCALLTYPE audiocapture_pause(IAudioCapture*);
	HRESULT STDMETHODCALLTYPE audiocapture_stop(IAudioCapture*);
	HRESULT STDMETHODCALLTYPE audiocapture_seek(IAudioCapture*, INT64);

#ifdef __cplusplus
}
#endif // __cplusplus

typedef struct AudioCapture {
	CONST_VTBL IAudioCaptureVtbl* lpVtbl;
	int refCount;

	AVFormatContext* audioContext;
	AVInputFormat* audioInpFmt;
	AVCodec* audioCodec;
	AVPacket pck;
	AVFrame* audioFrame;
	int audioCodecId;
	int audioStreamIndex;
	uint8_t noOfChannels;

	IXAudio2VoiceCallback* xVoiceCallback;

	IXAudio2* pxAudio2;
	IXAudio2MasteringVoice* pMasterVoice;
	IXAudio2SourceVoice* pSourceVoice;
	XAUDIO2_BUFFER xa2Buffer;

	HANDLE seekEvent;
}AudioCapture;

static const IAudioCaptureVtbl IAudioCapture_Vtbl = { audiocapture_QueryInterface,
	audiocapture_AddRef,
	audiocapture_Release,
	audiocapture_CreateInstance,
	audiocapture_XAudioCapture,
	audiocapture_initXAudio,
	audiocapture_start,
	audiocapture_play,
	audiocapture_pause,
	audiocapture_stop,
	audiocapture_seek
};


#endif // !_H_AUDIOCAPTURE_H_

