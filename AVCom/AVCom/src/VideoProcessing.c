#include "../include/VideoProcessing.h"
#include "../include/NvdecService.h"
#include "../include/AudioCapture.h"

HRESULT STDMETHODCALLTYPE videoProcessing_QueryInterface(IVideoProcessing* _this, REFIID refiid, _COM_Outptr_ void** data)
{
	return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE videoProcessing_AddRef(IVideoProcessing* _this)
{
	return ((VideoProcessing*)_this)->refCount++;
}

HRESULT STDMETHODCALLTYPE videoProcessing_CreateInstance(IVideoProcessing* _this, REFIID refiid, _COM_Outptr_ void** data)
{
	*data = NULL;
	if (IsEqualIID(refiid, &IID_INvdecService)) {
		NvdecService* this = *data;
		this = (INvdecService*)malloc(sizeof(NvdecService));
		this->lpVtbl = &INvdecService_Vtbl;
		if (!this)
			return E_OUTOFMEMORY;

		this->refCount = 0;
		for (int i = 0; i < 3; i++)
		{
			av_init_packet(&this->fltPck[i]);
			this->fltPck[i].data = NULL;
			this->fltPck[i].size = 0;

			av_init_packet(&this->pck[i]);
			this->pck[i].data = NULL;
			this->pck[i].size = 0;

			this->avTrpBuffer[i] = NULL;
		}

		this->avInFmt = NULL;
		this->avFmtContext = NULL;
		this->avBSFContext = NULL;
		this->avFmtContext = NULL;
		this->avCurrIndex = 0;
		this->decFrame = NULL;
		this->rgbFrame = NULL;
		this->readSyncThread = NULL;
		this->cuContext = NULL;
		this->cuParser = NULL;
		this->cuDecoder = NULL;
		this->cuPck.payload = NULL;
		this->cuPck.payload_size = 0;
		this->pBackBuffer = NULL;
		this->pDevice = NULL;
		this->pSurface = NULL;
		this->avBtStmFilter = NULL;
		this->cuResource = NULL;
		this->cuResult = CUDA_SUCCESS;
		this->dstArray = NULL;
		this->audio_raw_data = NULL;
		this->audio_raw_size = NULL;
		this->isTaskrdy = CreateEvent(NULL, TRUE, FALSE, NULL);

		this->jobDone = FALSE;
		this->isDisplayTriggered = FALSE;

		this->lpVtbl->AddRef(this);

		*data = this;
		return S_OK;
	}

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
}
HRESULT STDMETHODCALLTYPE videoProcessing_Release(IVideoProcessing* _this)
{
	VideoProcessing* this = (VideoProcessing*)_this;
	this->refCount--;
	if (!this->refCount)
	{
		if (this)
			free(this);
	}
	return S_OK;
}
HRESULT STDMETHODCALLTYPE videoProcessing_initCudaEnv(IVideoProcessing* _this, int ordinal)
{
	CUresult result = CUDA_SUCCESS;
	char err_str[50];
	VideoProcessing* this = (VideoProcessing*)_this;
	result = cuInit(0);
	if (result)
	{
#ifdef _DEBUG
		cuGetErrorName(result, &err_str);
		printf("Fatal error at cudaDriver. %s", err_str);
		return E_ABORT;
#endif // _DEBUG
	}
}
HRESULT STDMETHODCALLTYPE videoProcessing_getCudaDevice(IVideoProcessing* _this, CUdevice dev)
{
	return E_NOTIMPL;
}