#include "../include/AudioCapture.h"
AudioCapture* _THIS;

static void __stdcall _OnStreamEnd() {};
static void __stdcall _OnVoiceProcessingPassStart(UINT32 SamplesRequired) {
}
static void __stdcall _OnVoiceProcessingPassEnd() {};
static void __stdcall _OnBufferEnd(void* pBufferContext)
{
	WaitForSingleObject(_THIS->seekEvent, INFINITE);
	_THIS->xa2Buffer.AudioBytes = _THIS->audioFrame->linesize[0] / _THIS->noOfChannels;
	_THIS->xa2Buffer.pAudioData = (BYTE*)_THIS->audioFrame->extended_data[0];
	if (_THIS->audioFrame->linesize[0] > 0)
		_THIS->pSourceVoice->lpVtbl->SubmitSourceBuffer(_THIS->pSourceVoice, &_THIS->xa2Buffer, NULL);

}
static void __stdcall _OnBufferStart(AudioCapture* this)
{
	int avCode = 0;
	while (avCode >= 0) {
		WaitForSingleObject(this->seekEvent, INFINITE);
		int gotFrame = 0;
		if (_THIS->pck.data)
			av_packet_unref(&_THIS->pck);

		avCode = av_read_frame(_THIS->audioContext, &_THIS->pck);
		if (avCode >= 0) {
			if (_THIS->pck.stream_index == _THIS->audioStreamIndex) {
				avCode = avcodec_decode_audio4(_THIS->audioContext->streams[_THIS->audioStreamIndex]->codec,
					_THIS->audioFrame,
					&gotFrame,
					&_THIS->pck);

				if (avCode < 0)
					ck(__LINE__, __FILE__, avCode);
				if (gotFrame > 0)
					break;
			}
		}
		else {
			audiocapture_pause(_THIS);
			ck(__LINE__, __FILE__, avCode);
		}
	}
}
static void __stdcall _OnLoopEnd(void* pBufferContext) {};
static void __stdcall _OnVoiceError(void* pBufferContext, HRESULT Error) {};

HRESULT STDMETHODCALLTYPE audiocapture_QueryInterface(IAudioCapture* _this, REFIID refiid, _COM_Outptr_ void** data)
{
	*data = NULL;
	if (IsEqualIID(refiid, &IID_IUnknown)) {
		return S_OK;
	}
	return E_NOINTERFACE;
}
HRESULT STDMETHODCALLTYPE audiocapture_AddRef(IAudioCapture* _this)
{
	return (((AudioCapture*)_this)->refCount++);
	return S_OK;
}
HRESULT STDMETHODCALLTYPE audiocapture_Release(IAudioCapture* _this)
{
	AudioCapture* this = (AudioCapture*)_this;
	this->refCount--;
	if (!this->refCount) {
		if (this->audioContext) {
			avformat_close_input(&this->audioContext);
			this->audioContext = NULL;
		}

		if (this->audioFrame) {
			av_frame_free(&this->audioFrame);
			this->audioFrame = NULL;
		}

		if (this->xVoiceCallback) {
			free(this->xVoiceCallback);
			this->xVoiceCallback = NULL;
		}

		if (this->pSourceVoice) {
			this->pSourceVoice->lpVtbl->DestroyVoice(this->pSourceVoice);
			this->pSourceVoice = NULL;
		}

		if (this->pMasterVoice) {
			this->pMasterVoice->lpVtbl->DestroyVoice(this->pMasterVoice);
			this->pMasterVoice = NULL;
		}

		if (this->pxAudio2) {
			this->pxAudio2->lpVtbl->Release(this->pxAudio2);
			this->pxAudio2 = NULL;
		}

		if (this) {
			free(this);
			_this = NULL;
		}
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE audiocapture_CreateInstance(IAudioCapture* _this, REFIID refiid, _COM_Outptr_ void** data)
{
	if (IsEqualIID(refiid, &IID_IAudioCapture)) {
		AudioCapture* this = *data;
		this = (IAudioCapture*)malloc(sizeof(AudioCapture));
		this->lpVtbl = (IAudioCaptureVtbl*)&IAudioCapture_Vtbl;
		this->refCount = 0;

		this->audioContext = NULL;
		this->audioInpFmt = NULL;
		this->audioCodec = NULL;
		this->audioFrame = NULL;

		this->pxAudio2 = NULL;
		this->pMasterVoice = NULL;
		this->pSourceVoice = NULL;
		this->xVoiceCallback = NULL;
		memset(&this->xa2Buffer, 0, sizeof(XAUDIO2_BUFFER));

		this->lpVtbl->AddRef(this);
		*data = this;
		return S_OK;
	}
	else
		return E_NOINTERFACE;

}
HRESULT STDMETHODCALLTYPE audiocapture_XAudioCapture(IAudioCapture* _this, char* szFilePath)
{
	AudioCapture* this = (AudioCapture*)_this;
	HRESULT hResult = S_OK;
	int avCode = 0;
	AVStream* audioStream = NULL;

	if (SUCCEEDED(hResult)) {
		avCode = avformat_open_input(&this->audioContext,
			szFilePath,
			this->audioInpFmt,
			NULL);

		if (avCode < 0) {
			ck(__LINE__, __FILE__, avCode);
			hResult = HRESULT_FROM_WIN32(E_INVALIDARG);
		}
	}

	if (SUCCEEDED(hResult)) {
		avCode = avformat_find_stream_info(
			this->audioContext,
			NULL);

		if (avCode < 0) {
			ck(__LINE__, __FILE__, avCode);
			hResult = HRESULT_FROM_WIN32(E_FAIL);
		}
	}

	if (SUCCEEDED(hResult)) {
		this->audioStreamIndex = av_find_best_stream(this->audioContext,
			AVMEDIA_TYPE_AUDIO,
			-1,
			-1,
			NULL,
			0);

		if (this->audioStreamIndex < 0) {
#ifdef _DEBUG
			printf("\n\nAudio stream not found! Release this object!! \n\n");
#endif // _DEBUG
			WIN32ck(__LINE__, __FILE__, hResult);
			hResult = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
		}
	}

	if (SUCCEEDED(hResult)) {
		audioStream = this->audioContext->streams[this->audioStreamIndex];
		this->audioCodec = avcodec_find_decoder(audioStream->codec->codec_id);
		if (!this->audioCodec) {
			ck(__LINE__, __FILE__, AVERROR_DECODER_NOT_FOUND);
			hResult = HRESULT_FROM_WIN32(ERROR_DATA_NOT_ACCEPTED);
		}

#ifdef _DEBUG
		printf("\nStream [%d] params:\n", this->audioStreamIndex);
		printf("\nType: Audio");
		printf("\nCodec ID: %d", audioStream->codec->codec_id);
		printf("\nCoded bitsPerSample: %d", audioStream->codec->bits_per_coded_sample);
		printf("\nSample Rate: %d", audioStream->codec->sample_rate);

#endif // _DEBUG

	}

	if (SUCCEEDED(hResult)) {
		this->audioFrame = av_frame_alloc();
		avCode = avcodec_open2(audioStream->codec, this->audioCodec, NULL);
		if (!this->audioCodec) {
			ck(__LINE__, __FILE__, avCode);
			hResult = HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_TYPE);
		}
		this->audioCodecId = audioStream->codecpar->codec_id;
	}
	return hResult;
}

HRESULT STDMETHODCALLTYPE audiocapture_initXAudio(IAudioCapture* _this)
{
	HRESULT hResult = S_OK;
	AudioCapture* this = (AudioCapture*)_this;
	if (SUCCEEDED(hResult))
		hResult = CoInitialize(0);

	if (SUCCEEDED(hResult) || hResult == S_FALSE) {
		hResult = XAudio2Create(&this->pxAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
		hResult = this->pxAudio2->lpVtbl->CreateMasteringVoice(this->pxAudio2,
			&this->pMasterVoice,
			XAUDIO2_DEFAULT_CHANNELS,
			XAUDIO2_DEFAULT_SAMPLERATE,
			0,
			NULL,
			NULL,
			0
		);
		WIN32ck(__LINE__, __FILE__, hResult);
	}
	return hResult;
}

HRESULT STDMETHODCALLTYPE audiocapture_start(IAudioCapture* _this)
{
	AudioCapture* this = (IAudioCapture*)_this;
	HRESULT hResult = S_OK;
	WAVEFORMATEX waveFmt;
	memset(&waveFmt, 0, sizeof(WAVEFORMATEX));
	AVCodecContext* audioCodecContext = this->audioContext->streams[this->audioStreamIndex]->codec;
	this->noOfChannels = 1;

	waveFmt.cbSize = sizeof(WAVEFORMATEX);
	switch (this->audioCodecId)
	{
	case AV_CODEC_ID_MP2: {
		waveFmt.wFormatTag = WAVE_FORMAT_PCM;
		waveFmt.nChannels = XAUDIO2_AV_S16p;
		waveFmt.wBitsPerSample = 16;
		this->noOfChannels = XAUDIO2_AV_S16p;
	}break;

	case AV_CODEC_ID_AAC: {
		waveFmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
		waveFmt.nChannels = XAUDIO2_AV_FLTP / 2;
		waveFmt.wBitsPerSample = 32;
		this->noOfChannels = XAUDIO2_AV_FLTP;
	}break;
	default:
		hResult = E_NOTIMPL;
		break;
	}

	if (SUCCEEDED(hResult)) {
		waveFmt.nSamplesPerSec = audioCodecContext->sample_rate;
		waveFmt.nBlockAlign = (waveFmt.nChannels * waveFmt.wBitsPerSample) / 8;
		waveFmt.nAvgBytesPerSec = waveFmt.nSamplesPerSec * waveFmt.nBlockAlign;
		this->xVoiceCallback = (IXAudio2VoiceCallback*)malloc(sizeof(IXAudio2VoiceCallback));
		if (!this->xVoiceCallback) {
			hResult = E_OUTOFMEMORY;
			WIN32ck(__LINE__, __FILE__, hResult);
		}
	}


	if (SUCCEEDED(hResult)) {
		static IXAudio2VoiceCallbackVtbl cllVtbl;

		cllVtbl.OnBufferEnd = _OnBufferEnd;
		cllVtbl.OnBufferStart = _OnBufferStart;
		cllVtbl.OnLoopEnd = _OnLoopEnd;
		cllVtbl.OnVoiceProcessingPassEnd = _OnVoiceProcessingPassEnd;
		cllVtbl.OnVoiceProcessingPassStart = _OnVoiceProcessingPassStart;
		cllVtbl.OnVoiceError = _OnVoiceError;
		cllVtbl.OnStreamEnd = _OnStreamEnd;

		this->xVoiceCallback->lpVtbl = (IXAudio2VoiceCallbackVtbl*)&cllVtbl;
	}

	if (SUCCEEDED(hResult)) {
		hResult = this->pxAudio2->lpVtbl->CreateSourceVoice(this->pxAudio2,
			&this->pSourceVoice,
			&waveFmt,
			0,
			XAUDIO2_DEFAULT_FREQ_RATIO,
			this->xVoiceCallback,
			NULL,
			NULL);
		WIN32ck(__LINE__, __FILE__, hResult);
	}

	if (SUCCEEDED(hResult)) {
		hResult = audiocapture_play(this);
		WIN32ck(__LINE__, __FILE__, hResult);
	}
	return hResult;
}

HRESULT STDMETHODCALLTYPE audiocapture_play(IAudioCapture* _this)
{
	AudioCapture* this = (AudioCapture*)_this;
	HRESULT hResult = S_OK;
	if (SUCCEEDED(hResult))
		hResult = this->pSourceVoice->lpVtbl->Start(this->pSourceVoice, 0, XAUDIO2_COMMIT_ALL);

	if (SUCCEEDED(hResult)) {
		av_init_packet(&this->pck);
		while (TRUE) {
			int gotFrame = 0;
			int avCode = 0;
			if (this->pck.data)
				av_packet_unref(&this->pck);

			avCode = av_read_frame(this->audioContext, &this->pck);
			if (avCode >= 0) {
				if (this->pck.stream_index == this->audioStreamIndex) {
					avCode = avcodec_decode_audio4(this->audioContext->streams[this->audioStreamIndex]->codec,
						this->audioFrame,
						&gotFrame,
						&this->pck);

					if (avCode < 0)
						ck(__LINE__, __FILE__, avCode);
					if (gotFrame > 0)
						break;
				}
			}
		}

		this->xa2Buffer.AudioBytes = this->audioFrame->linesize[0];
		this->xa2Buffer.pAudioData = this->audioFrame->extended_data[0];
		_THIS = this;
		hResult = this->pSourceVoice->lpVtbl->SubmitSourceBuffer(this->pSourceVoice, &this->xa2Buffer, NULL);
		WIN32ck(__LINE__, __FILE__, hResult);
	}
	return hResult;

}

HRESULT STDMETHODCALLTYPE audiocapture_pause(IAudioCapture* _this)
{
	AudioCapture* this = (AudioCapture*)_this;
	HRESULT hResult = S_OK;
	hResult = this->pSourceVoice->lpVtbl->Stop(this->pSourceVoice, 0, 0);
	return hResult;
}

HRESULT STDMETHODCALLTYPE audiocapture_stop(IAudioCapture* _this)
{
	return audiocapture_stop(_this);
}

HRESULT STDMETHODCALLTYPE audiocapture_seek(IAudioCapture* _this, int64_t seconds)
{
	AudioCapture* this = (AudioCapture*)_this;
	HRESULT hResult = S_OK;
	int avCode = 0;

	ResetEvent(this->seekEvent);

	AVStream* m_video_stream = _THIS->audioContext->streams[_THIS->audioStreamIndex];
	AVCodecContext* audioCodecContext = _THIS->audioContext->streams[_THIS->audioStreamIndex]->codec;
	int64_t m_target_ts = seconds * AV_TIME_BASE;
	avcodec_flush_buffers(audioCodecContext);
	avCode = av_seek_frame(_THIS->audioContext, -1, m_target_ts, AVSEEK_FLAG_FRAME);
	if (avCode < 0) {
		ck(__LINE__, __FILE__, hResult);
		hResult = HRESULT_FROM_WIN32(ERROR_SEEK);
	}

	SetEvent(this->seekEvent);
	return hResult;
}