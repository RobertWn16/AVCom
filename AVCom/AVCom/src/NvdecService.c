#include "../include/NvdecService.h"
#include "../include/cudaGpuMat.h"
#include <cudaD3D9.h>

#define MINUTE_UNIT 60

static int parser_decode_picture_callback(void* user, CUVIDPICPARAMS* pic)
{
	NvdecService* this = (NvdecService*)user;
	if (this->cuResult)
		return this->cuResult;

	if (!this->cuDecoder) {
#ifdef _DEBUG
		printf("Decoder is not initialized. \n");
#endif // !_DEBUG
		return CUDA_ERROR_NOT_INITIALIZED;
	}

	this->cuResult = cuvidDecodePicture(this->cuDecoder, pic);
	if (this->cuResult) {
		CUDAck(__LINE__, __FILE__, this->cuResult);
		return this->cuResult;
	}
	return 10;
}

static int parser_sequence_callback(void* user, CUVIDEOFORMAT* fmt)
{
	NvdecService* this = (NvdecService*)user;
	if (NULL == this->cuContext) {
#ifdef _DEBUG
		printf("The CUcontext is nullptr, you should initialize it before kicking off the decoder.\n");
#endif // DEBUG
		return CUDA_ERROR_INVALID_CONTEXT;
	}

	//Multithread purpose
	cuCtxSetCurrent(this->cuContext);

#ifdef _DEBUG
	printf("CUVIDEOFORMAT.Coded size: %d x %d \n", fmt->coded_width, fmt->coded_height);
	printf("CUVIDEOFORMAT.Display area: %d %d %d %d \n", fmt->display_area.left, fmt->display_area.top, fmt->display_area.right, fmt->display_area.bottom);
	printf("CUVIDEOFORMAT.Bitrate: %u \n", fmt->bitrate);
	printf("CUVIDEOFORMAT.ChromaFormat %d \n", fmt->chroma_format);
	//printf("CUVIDEOFORMAT.Framerate %d \n", fmt->frame_rate.numerator / fmt->frame_rate.denominator);
#endif // _DEBUG

	//Query NvdecServiceService capabilities by information provided in parser
	CUVIDDECODECAPS decode_caps;
	memset(&decode_caps, 0, sizeof(CUVIDDECODECAPS));
	decode_caps.eCodecType = fmt->codec;
	decode_caps.eChromaFormat = fmt->chroma_format;
	decode_caps.nBitDepthMinus8 = fmt->bit_depth_luma_minus8;

	this->cuResult = cuvidGetDecoderCaps(&decode_caps);

	if (this->cuResult) {
		CUDAck(__LINE__, __FILE__, this->cuResult);
		return this->cuResult;
	}

	if (!decode_caps.bIsSupported) {
#ifdef _DEBUG
		printf("The video file codec is not supported by NvdecServiceODE.\n");
#endif // _DEBUG
		return this->cuResult;
	}

	if (!decode_caps.bIsHistogramSupported) {
#ifdef _DEBUG
		printf("Warning! Color Histogram is not supported on device. \n");
#endif // _DEBUG
	}

	if (decode_caps.nMaxWidth < this->frame_width) {
#ifdef _DEBUG
		printf("Warning! Input width not supported on this gpu. Image will be cropped \n");
#endif // _DEBUG
		this->frame_width = decode_caps.nMaxWidth;
	}

	if (decode_caps.nMaxHeight < this->frame_heigth) {
#ifdef _DEBUG
		printf("Warning! Input heigth not supported on this gpu. Image will be cropped \n");
#endif // _DEBUG
		this->frame_heigth = decode_caps.nMaxHeight;
	}

	/* Create decoder context. */
	CUVIDDECODECREATEINFO create_info = { 0 };
	create_info.CodecType = fmt->codec;
	create_info.ChromaFormat = fmt->chroma_format;
	create_info.OutputFormat = (fmt->bit_depth_luma_minus8) ? cudaVideoSurfaceFormat_P016 : cudaVideoSurfaceFormat_NV12;
	create_info.bitDepthMinus8 = fmt->bit_depth_luma_minus8;
	create_info.DeinterlaceMode = cudaVideoDeinterlaceMode_Weave;
	create_info.ulNumOutputSurfaces = 2;
	create_info.ulNumDecodeSurfaces = this->cuParserParams.ulMaxNumDecodeSurfaces;
	create_info.ulCreationFlags = cudaVideoCreate_PreferCUVID;
	create_info.vidLock = NULL;
	create_info.ulIntraDecodeOnly = 0;
	create_info.ulTargetWidth = fmt->coded_width;
	create_info.ulTargetHeight = fmt->coded_height;
	create_info.ulWidth = fmt->coded_width;
	create_info.ulHeight = fmt->coded_height;

	cuCtxPushCurrent(this->cuContext);
	{
		this->cuResult = cuvidCreateDecoder(&this->cuDecoder, &create_info);
		if (this->cuResult) {
			CUDAck(__LINE__, __FILE__, this->cuResult);
			return this->cuResult;
		}
	}
	cuCtxPopCurrent(NULL);

	cudaGpuMat* decodedframe = (cudaGpuMat*)this->decFrame;
	cudaGpuMat* rgbframe = (cudaGpuMat*)this->rgbFrame;

	decodedframe->width = this->frame_width;
	decodedframe->heigth = this->frame_heigth;
	decodedframe->surfaceHeigth = fmt->coded_height;

	this->rgbFrame->lpVtbl->Alloc(this->rgbFrame, gpuMat_R8B8G8, this->frame_width, this->frame_heigth);

	return this->cuParserParams.ulMaxNumDecodeSurfaces;
}

static int parser_display_picture_callback(void* user, CUVIDPARSERDISPINFO* info)
{
	NvdecService* this = (NvdecService*)user;
	cudaGpuMat* decodedframe = (cudaGpuMat*)this->decFrame;
	cudaGpuMat* rgbframe = (cudaGpuMat*)this->rgbFrame;
	this->isDisplayTriggered = TRUE;

	if (!info) {
#ifdef _DEBUG
		printf("Cannot map the picture; nullptr given. (exiting).\n");
#endif // DEBUG
		return CUDA_ERROR_INVALID_VALUE;
	}

	CUVIDPROCPARAMS vpp = { 0 };
	vpp.progressive_frame = info->progressive_frame;
	vpp.top_field_first = info->top_field_first + 1;
	vpp.unpaired_field = (info->repeat_first_field < 0);
	vpp.second_field = info->repeat_first_field + 1;


	this->cuResult = cuvidMapVideoFrame(this->cuDecoder,
		info->picture_index,
		&decodedframe->data,
		&decodedframe->pitch,
		&vpp);
	if (this->cuResult) {
		CUDAck(__LINE__, __FILE__, this->cuResult);
		return this->cuResult;
	}

	this->cuResult = decodedframe->lpVtbl->convertYUVtoRGB(decodedframe, gpuMat_NV128u, this->rgbFrame);

	if (this->cuResult != CUDA_SUCCESS) {
		CUDAck(__LINE__, __FILE__, this->cuResult);
		return this->cuResult;
	}

	this->cuResult = cuvidUnmapVideoFrame(this->cuDecoder, decodedframe->data);
	if (this->cuResult) {
		CUDAck(__LINE__, __FILE__, this->cuResult);
		return this->cuResult;
	}
	return 1;
}

HRESULT tripleBuffering(NvdecService* _this) {
	HRESULT hResult = S_OK;
	SYSTEMTIME start;
	SYSTEMTIME stop;
	memset(&start, 0, sizeof(SYSTEMTIME));
	memset(&stop, 0, sizeof(SYSTEMTIME));
	GetSystemTime(&start);

	hResult = _this->lpVtbl->read(_this);

	if (SUCCEEDED(hResult)) {
		if (_this->avCodecId == AV_CODEC_ID_MPEG2VIDEO || _this->avCodecId == AV_CODEC_ID_MPEG1VIDEO)
			InterlockedCompareExchangePointer((volatile PVOID*)&_this->avTrpBuffer[2], (PVOID*)&_this->pck[_this->avCurrIndex], _this->avTrpBuffer[2]);
		if (_this->avCodecId == AV_CODEC_ID_H264 || _this->avCodecId == AV_CODEC_ID_HEVC)
			InterlockedCompareExchangePointer((volatile PVOID*)&_this->avTrpBuffer[2], (PVOID*)&_this->fltPck[_this->avCurrIndex], _this->avTrpBuffer[2]); //2nd Back buffer

		InterlockedCompareExchangePointer((volatile PVOID*)&_this->avTrpBuffer[1], (PVOID*)_this->avTrpBuffer[2], _this->avTrpBuffer[1]);
		InterlockedExchange(&_this->isFrameReady, 0);
		double _frequency = ((1.0f / (double)(_this->avFramerate)) * 1000);

		GetSystemTime(&stop);
		int callDelay = stop.wMilliseconds - start.wMilliseconds;
		while ((abs(callDelay) < _frequency)) {
			callDelay = stop.wMilliseconds - start.wMilliseconds;
			if (callDelay >= _frequency) {
				break;
			}
			Sleep(1);
			GetSystemTime(&stop);
		}
		_this->avCurrIndex++;
	}

	return hResult;
}

//NVIDIA SAMPLES METHODS
static unsigned long GetNumDecodeSurfaces(cudaVideoCodec eCodec, unsigned int nWidth, unsigned int nHeight)
{
	if (eCodec == cudaVideoCodec_VP9) {
		return 12;
	}

	if (eCodec == cudaVideoCodec_H264 || eCodec == cudaVideoCodec_H264_SVC || eCodec == cudaVideoCodec_H264_MVC) {
		// assume worst-case of 20 decode surfaces for H264
		return 20;
	}

	if (eCodec == cudaVideoCodec_HEVC) {
		// ref HEVC spec: A.4.1 General tier and level limits
		// currently assuming level 6.2, 8Kx4K
		int MaxLumaPS = 35651584;
		int MaxDpbPicBuf = 6;
		int PicSizeInSamplesY = (int)(nWidth * nHeight);
		int MaxDpbSize;
		if (PicSizeInSamplesY <= (MaxLumaPS >> 2))
			MaxDpbSize = MaxDpbPicBuf * 4;
		else if (PicSizeInSamplesY <= (MaxLumaPS >> 1))
			MaxDpbSize = MaxDpbPicBuf * 2;
		else if (PicSizeInSamplesY <= ((3 * MaxLumaPS) >> 2))
			MaxDpbSize = (MaxDpbPicBuf * 4) / 3;
		else
			MaxDpbSize = MaxDpbPicBuf;
		return MaxDpbSize + 4;
	}
	return 8;
}

//NVIDIA SAMPLES METHODS
inline cudaVideoCodec FFmpeg2NvCodecId(int id)
{
	switch (id) {
	case AV_CODEC_ID_MPEG1VIDEO: return cudaVideoCodec_MPEG1;
	case AV_CODEC_ID_MPEG2VIDEO: return cudaVideoCodec_MPEG2;
	case AV_CODEC_ID_MPEG4: return cudaVideoCodec_MPEG4;
	case AV_CODEC_ID_VC1:return cudaVideoCodec_VC1;
	case AV_CODEC_ID_H264: return cudaVideoCodec_H264;
	case AV_CODEC_ID_HEVC: return cudaVideoCodec_HEVC;
	case AV_CODEC_ID_VP8: return cudaVideoCodec_VP8;
	case AV_CODEC_ID_VP9: return cudaVideoCodec_VP9;
	case AV_CODEC_ID_MJPEG: return cudaVideoCodec_JPEG;
	default: return cudaVideoCodec_NumCodecs;
	}
}

HRESULT CreateParser(INvdecService* _this)
{
	HRESULT hResult = S_OK;
	NvdecService* this = (NvdecService*)_this;
	memset(&this->cuParserParams, 0, sizeof(CUVIDPARSERPARAMS));

	this->cuParserParams.CodecType = FFmpeg2NvCodecId(this->avCodecId);
	this->cuParserParams.pUserData = _this;
	this->cuParserParams.ulMaxDisplayDelay = 2;
	this->cuParserParams.ulMaxNumDecodeSurfaces = GetNumDecodeSurfaces(this->cuParserParams.CodecType,
		this->frame_width, this->frame_heigth);
	this->cuParserParams.pfnDecodePicture = (PFNVIDDECODECALLBACK)parser_decode_picture_callback;
	this->cuParserParams.pfnSequenceCallback = (PFNVIDSEQUENCECALLBACK)parser_sequence_callback;
	this->cuParserParams.pfnDisplayPicture = (PFNVIDDISPLAYCALLBACK)parser_display_picture_callback;
	//this->cuParserParams.pfnGetOperatingPoint;

	this->cuResult = cuvidCreateVideoParser(&this->cuParser, &this->cuParserParams);
	if (this->cuResult) {
		CUDAck(__LINE__, __FILE__, this->cuResult);
		hResult = E_INVALIDARG;
	}
	return hResult;
}

HRESULT STDMETHODCALLTYPE backend_mapToD3D9Tex(NvdecService* this, cudaGpuMat* rgbaFrame) {
	HRESULT hResult = S_OK;
	if (this->isDisplayTriggered) {

		if (SUCCEEDED(hResult)) {
			this->cuResult = cuGraphicsMapResources(1, &this->cuResource, 0);
			if (this->cuResult) {
				CUDAck(__LINE__, __FILE__, this->cuResult);
				hResult = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
			}
		}

		if (SUCCEEDED(hResult)) {
			this->cuResult = cuGraphicsSubResourceGetMappedArray(&this->dstArray,
				this->cuResource,
				0,
				0);

			if (this->cuResult) {
				CUDAck(__LINE__, __FILE__, this->cuResult);
				hResult = HRESULT_FROM_WIN32(ERROR_RESOURCE_DATA_NOT_FOUND);
			}
		}

		CUDA_MEMCPY2D m = { 0 };
		m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
		m.srcDevice = (CUdeviceptr)rgbaFrame->data;
		m.srcPitch = rgbaFrame->pitch;
		m.dstMemoryType = CU_MEMORYTYPE_ARRAY;
		m.dstArray = this->dstArray;
		m.WidthInBytes = rgbaFrame->pitch;
		m.Height = rgbaFrame->heigth;

		if (SUCCEEDED(hResult)) {
			this->cuResult = cuMemcpy2D(&m);
			this->cuResult = cuGraphicsUnmapResources(1, &this->cuResource, 0);
			if (this->cuResult) {
				CUDAck(__LINE__, __FILE__, this->cuResult);
				hResult = E_INVALIDARG;
			}
		}
	}

	return hResult;
}

HRESULT STDMETHODCALLTYPE backend_QueryInterface(INvdecService* _this, REFIID reffd, _COM_Outptr_  void** data) {
	return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE backend_AddRef(INvdecService* _this)
{
	((NvdecService*)_this)->refCount++;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE backend_Release(INvdecService* _this)
{
	NvdecService* this = (NvdecService*)_this;
	this->refCount--;

	if (!this->refCount) {
		this->jobDone = TRUE;
		WaitForSingleObject(this->readSyncThread, INFINITE);

		for (int i = 0; i < 3; i++) {
			if (&this->pck[i]) {
				av_packet_unref(&this->pck[i]);
				memset(&this->pck[i], 0, sizeof(AVPacket));
			}

			if (&this->fltPck[i]) {
				av_packet_unref(&this->pck[i]);
				memset(&this->fltPck[i], 0, sizeof(AVPacket));
			}
		}

		if (this->avFmtContext) {
			avformat_close_input(&this->avFmtContext);
			this->avFmtContext = NULL;
		}

		if (this->decFrame) {
			this->decFrame->lpVtbl->Release(this->decFrame);
			this->decFrame = NULL;
		}

		if (this->rgbFrame) {
			this->rgbFrame->lpVtbl->Release(this->rgbFrame);
			this->rgbFrame = NULL;
		}

		if (this->cuResource) {
			cuGraphicsUnmapResources(1, &this->cuResource, 0);
			cuD3D9UnregisterResource(this->cuResource);
			this->cuResource = NULL;
		}

		if (this->cuParser) {
			cuvidDestroyVideoParser(this->cuParser);
			this->cuParser = NULL;
		}

		if (this->cuDecoder) {
			cuvidDestroyDecoder(this->cuDecoder);
			this->cuDecoder = NULL;
		}

		if (this->cuContext) {
			cuCtxDestroy(this->cuContext);
			this->cuContext = NULL;
		}

		if (this->pBackBuffer) {
			this->pBackBuffer->lpVtbl->Release(this->pBackBuffer);
			this->pBackBuffer = NULL;
		}

		if (this->pSurface) {
			this->pSurface->lpVtbl->Release(this->pSurface);
			this->pSurface = NULL;
		}

		if (this->pDevice) {
			this->pDevice->lpVtbl->Release(this->pDevice);
			this->pDevice = NULL;
		}

		if (this->isTaskrdy) {
			CloseHandle(this->isTaskrdy);
			this->isTaskrdy = NULL;
		}

		if (this->readSyncThread) {
			CloseHandle(this->readSyncThread);
			this->readSyncThread = NULL;
		}

		if (this) {
			free(this);
			_this = NULL;
		}
	}
	return S_OK;
}
HRESULT STDMETHODCALLTYPE backend_CreateInstance(INvdecService* _this, REFIID refiid, _COM_Outptr_ void** data)
{
	if (IsEqualIID(refiid, &IID_IcudaGpuMat)) {
		cudaGpuMat* __this = *data;
		__this = (IcudaGpuMat*)malloc(sizeof(cudaGpuMat));
		__this->lpVtbl = (IcudaGpuMatVtbl*)&IcudaGpuMat_Vtbl;
		if (!__this)
			return E_OUTOFMEMORY;

		__this->width = 0;
		__this->heigth = 0;
		__this->bit_depth_luma = 0;
		__this->cuChroma = NULL;
		__this->cuLuma = NULL;
		__this->refCount = 0;
		__this->data = NULL;
		__this->noOfChannels = 0;
		__this->pitch = 0;
		__this->surfaceHeigth = 0;
		__this->size = 0;

		__this->lpVtbl->AddRef(__this);
		*data = __this;
		cuCtxSetCurrent(((NvdecService*)_this)->cuContext);
		return S_OK;
	}
	return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE backend_selectGpuDevice(INvdecService* _this, int ordinal)
{
	HRESULT hResult = S_OK;

	NvdecService* this = (NvdecService*)_this;
	this->ordinal = ordinal;

	if (SUCCEEDED(hResult)) {
		this->cuResult = cuDeviceGet(&this->device, ordinal);
		if (this->cuResult) {
			CUDAck(__LINE__, __FILE__, this->cuResult);
			hResult = HRESULT_FROM_WIN32(ERROR_INVALID_DEVICE_OBJECT_PARAMETER);
		}
	}

	if (SUCCEEDED(hResult)) {
		this->cuResult = cuCtxCreate(&this->cuContext, 0, this->device);
		if (this->cuResult) {
			CUDAck(__LINE__, __FILE__, this->cuResult);
			hResult = E_INVALIDARG;
		}
	}

	return hResult;
}

HRESULT STDMETHODCALLTYPE backend_initD3D9(NvdecService* this)
{
#ifdef _WIN32
	HRESULT hResult = S_OK;
	IDirect3D9Ex* pD3D = NULL;
	Direct3DCreate9Ex(D3D_SDK_VERSION, &pD3D);
	D3DPRESENT_PARAMETERS d3dpp = { 0 };
	d3dpp.BackBufferWidth = this->frame_width;
	d3dpp.BackBufferHeight = this->frame_heigth;
	d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
	d3dpp.EnableAutoDepthStencil = FALSE;
	d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	d3dpp.FullScreen_RefreshRateInHz = 0;
	d3dpp.Windowed = TRUE;
	d3dpp.hDeviceWindow = NULL;

	if (SUCCEEDED(hResult)) {
		hResult = pD3D->lpVtbl->CreateDevice(pD3D,
			this->ordinal,
			D3DDEVTYPE_HAL,
			NULL,
			D3DCREATE_HARDWARE_VERTEXPROCESSING,
			&d3dpp,
			&this->pDevice);
		WIN32ck(__LINE__, __FILE__, hResult);
	}

	if (SUCCEEDED(hResult)) {
		hResult = pD3D->lpVtbl->Release(pD3D);
		hResult = this->pDevice->lpVtbl->GetBackBuffer(this->pDevice,
			0,
			0,
			D3DPRESENT_BACK_BUFFERS_MAX,
			&this->pBackBuffer);
		WIN32ck(__LINE__, __FILE__, hResult);
	}

	if (SUCCEEDED(hResult)) {
		hResult = this->pDevice->lpVtbl->CreateOffscreenPlainSurface(this->pDevice,
			1280,
			720,
			D3DFMT_A8R8G8B8,
			D3DPOOL_SYSTEMMEM,
			&this->pSurface,
			NULL);
		WIN32ck(__LINE__, __FILE__, hResult);
	}

	if (SUCCEEDED(hResult)) {
		cuCtxPushCurrent(this->cuContext);

		this->cuResult = cuGraphicsD3D9RegisterResource(&this->cuResource,
			this->pBackBuffer,
			CU_GRAPHICS_REGISTER_FLAGS_NONE);
		if (this->cuResult) {
			CUDAck(__LINE__, __FILE__, this->cuResult);
			hResult = E_INVALIDARG;
		}
		cuCtxPopCurrent(NULL);
	}

	if (SUCCEEDED(hResult)) {
		cuCtxPushCurrent(this->cuContext);
		this->cuResult = cuGraphicsResourceSetMapFlags(this->cuResource, CU_GRAPHICS_MAP_RESOURCE_FLAGS_WRITE_DISCARD);
		if (this->cuResult) {
			CUDAck(__LINE__, __FILE__, this->cuResult);
			hResult = E_INVALIDARG;
		}
		cuCtxPopCurrent(NULL);
	}
	return hResult;
#else
	return -2;
#endif // _WIN32

}

HRESULT STDMETHODCALLTYPE backend_VideoCapture(INvdecService* _this, char* szFilePath, int flags, int ordinal)
{
	int avCode = 0;
	HRESULT hResult = S_OK;
	AVStream* stream = NULL;

	NvdecService* this = (NvdecService*)_this;
	this->lpVtbl->CreateInstance(this, &IID_IcudaGpuMat, &this->decFrame);
	this->lpVtbl->CreateInstance(this, &IID_IcudaGpuMat, &this->rgbFrame);

	switch (flags)
	{
	case NVDEC_DECODING_WEBCAMERA:
#ifdef _WIN32
		avdevice_register_all();
		this->avInFmt = av_find_input_format("dshow");
#endif // _WIN32
#ifdef __linux
		avdevice_register_all();
		this->avInFmt = av_find_input_format("v4l2");
#endif // __linux
		this->decType = NVDEC_DECODING;
		break;
	default:
		this->decType = NVDEC_DECODING;
		break;
	}

	if (SUCCEEDED(hResult)) {
		avCode = avformat_open_input(&this->avFmtContext,
			szFilePath,
			this->avInFmt,
			NULL);
		if (avCode < 0) {
			ck(__LINE__, __FILE__, avCode);
			hResult = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
		}
	}

	if (SUCCEEDED(hResult)) {
		avCode = avformat_find_stream_info(
			this->avFmtContext,
			NULL);
		if (avCode < 0)
			ck(__LINE__, __FILE__, avCode);

		this->videoStreamIndex = av_find_best_stream(this->avFmtContext,
			AVMEDIA_TYPE_VIDEO,
			-1,
			-1,
			NULL,
			0);

		if (this->videoStreamIndex < 0) {
#ifdef _DEBUG
			printf("FFMPEG error: Can't find video index! Please check your input!\n");
#endif // _DEBUG
			hResult = E_INVALIDARG;
		}
	}

	if (SUCCEEDED(hResult)) {
		stream = this->avFmtContext->streams[this->videoStreamIndex];

		this->avCodecId = stream->codecpar->codec_id;
		this->videoDuration = ((double)stream->duration * ((double)stream->time_base.num) / (double)stream->time_base.den);

		double preciseFramerate = ((double)stream->codec->framerate.num /
			(double)stream->codec->framerate.den);
		this->avFramerate = preciseFramerate;

#ifdef _DEBUG
		printf("Format General Info... \n");
		printf("Format: %s \n", this->avFmtContext->iformat->long_name);
		printf("Codec ID: %d \n", stream->codecpar->codec_id);
		printf("Width: %d \n", stream->codec->width);
		printf("Heigth: %d \n", stream->codec->height);
		printf("Video framerate: %f \n", (double)stream->codec->framerate.num /
			(double)stream->codec->framerate.den);
		printf("Video duration: %f \n", (double)stream->duration);
		printf("FFmpeg time base: %f\n", (double)stream->time_base.num / (double)stream->time_base.den);
		printf("Video duration in seconds: %f \n", this->videoDuration);
#endif // _DEBUG

		if (this->avCodecId == AV_CODEC_ID_H264 || this->avCodecId == AV_CODEC_ID_HEVC) {
			if (this->avCodecId == AV_CODEC_ID_H264)
				this->avBtStmFilter = av_bsf_get_by_name("h264_mp4toannexb");
			else
				this->avBtStmFilter = av_bsf_get_by_name("hevc_mp4toannexb");
			avCode = av_bsf_alloc(this->avBtStmFilter, &this->avBSFContext);
			if (avCode < 0) {
				ck(__LINE__, __FILE__, avCode);
				hResult = HRESULT_FROM_WIN32(ERROR_UNSUPPORTED_COMPRESSION);
			}

			if (avCode >= 0) {
				this->avBSFContext->par_in = stream->codecpar;
				avCode = av_bsf_init(this->avBSFContext);
				if (avCode < 0) {
					ck(__LINE__, __FILE__, avCode);
					hResult = E_POINTER;
				}
			}
		}
	}

	if (SUCCEEDED(hResult)) {
		this->frame_width = stream->codec->width;
		this->frame_heigth = stream->codec->height;
		hResult = backend_selectGpuDevice(this, ordinal);
	}

	if (SUCCEEDED(hResult))
		hResult = CreateParser(_this);

	return hResult;
}

HRESULT STDMETHODCALLTYPE backend_read(INvdecService* _this)
{
	HRESULT hResult = S_OK;
	NvdecService* this = (NvdecService*)_this;
	this->avCurrIndex = this->avCurrIndex % 3;

	//Eliminate audio codec and etc.
	while (TRUE) {
		if (this->pck[this->avCurrIndex].data)
			av_packet_unref(&this->pck[this->avCurrIndex]);

		if (SUCCEEDED(hResult)) {
			this->avCode = av_read_frame(this->avFmtContext, &this->pck[this->avCurrIndex]);
			if (this->avCode < 0) {
				this->jobDone = TRUE;
				ck(__LINE__, __FILE__, this->avCode);
				hResult = E_NOT_SUFFICIENT_BUFFER;
				break;
			}
		}

		if (SUCCEEDED(hResult)) {
			if (this->pck[this->avCurrIndex].stream_index == this->videoStreamIndex) {
				if (this->avCodecId == AV_CODEC_ID_H264) {
					if (this->fltPck[this->avCurrIndex].data)
						av_packet_unref(&this->fltPck[this->avCurrIndex]);

					this->avCode = av_bsf_send_packet(this->avBSFContext, &this->pck[this->avCurrIndex]);
					this->avCode = av_bsf_receive_packet(this->avBSFContext, &this->fltPck[this->avCurrIndex]);

					if (this->avCode < 0) {
						ck(__LINE__, __FILE__, this->avCode);
						hResult = E_POINTER;
					}
				}
				break;
			}
		}
	}
	return hResult;
}

HRESULT STDMETHODCALLTYPE readProducer(NvdecService* _this) {
	HRESULT hResult = S_OK;
	tripleBuffering(_this);
	SetEvent(_this->isTaskrdy);
	while (!hResult) {
		hResult = tripleBuffering(_this);
		if (_this->jobDone)
			break;
	}
	_this->jobDone = TRUE;
}

HRESULT STDMETHODCALLTYPE backend_readSync(INvdecService* _this) {
	NvdecService* this = (NvdecService*)_this;
	this->isFrameReady = FALSE;

	ResetEvent(this->isTaskrdy);
	this->readSyncThread = CreateThread(NULL,
		0,
		(LPTHREAD_START_ROUTINE)readProducer,
		this,
		0,
		NULL);

	if (this->readSyncThread == INVALID_HANDLE_VALUE)
		return E_HANDLE;
	WaitForSingleObject(this->isTaskrdy, INFINITE);
	return S_OK;
}

inline unsigned int STDMETHODCALLTYPE backend_getFrameWidth(INvdecService* _this)
{
	return (((NvdecService*)_this)->frame_width);
}

inline unsigned int STDMETHODCALLTYPE backend_getFrameHeigth(INvdecService* _this)
{
	return (((NvdecService*)_this)->frame_heigth);
}

inline AVPacket* STDMETHODCALLTYPE backend_getFrame(INvdecService* _this)
{
	NvdecService* this = (NvdecService*)_this;
	if (this->avCodecId == AV_CODEC_ID_H264)
		return &this->fltPck;
	return &this->pck;
}

inline long int STDMETHODCALLTYPE backend_getFrameSize(INvdecService* _this)
{
	return 1;
}

HRESULT STDMETHODCALLTYPE backend_decode(INvdecService* _this, void* data, cudaGpuMat* rgbaFrame)
{
	HRESULT hResult = S_OK;
	NvdecService* this = (NvdecService*)_this;
	cudaGpuMat* rgbframe = (cudaGpuMat*)this->rgbFrame;

	if (data) {
		AVPacket* pck = (AVPacket*)data;
		this->cuPck.payload = pck->data;
		this->cuPck.payload_size = pck->size;
		this->cuPck.flags = CUVID_PKT_TIMESTAMP;

		if (!this->cuPck.payload ||
			!this->cuPck.payload_size)
			this->cuPck.flags = CUVID_PKT_ENDOFSTREAM;

		if (this->cuResult) {
			CUDAck(__LINE__, __FILE__, this->cuResult);
			hResult = E_UNEXPECTED;
		}

		if (SUCCEEDED(hResult)) {
			this->cuResult = cuvidParseVideoData(this->cuParser, &this->cuPck);
			if (this->cuResult) {
				CUDAck(__LINE__, __FILE__, this->cuResult);
				hResult = E_INVALIDARG;
			}
		}

		if (SUCCEEDED(hResult)) {
			this->rgbFrame->lpVtbl->convertRGBtoRGBA(this->rgbFrame, rgbaFrame);
			if (this->cuResult) {
				CUDAck(__LINE__, __FILE__, this->cuResult);
				hResult = E_INVALIDARG;
			}
		}
	}
	return hResult;
}

HRESULT STDMETHODCALLTYPE backend_decodeSync(INvdecService* _this, cudaGpuMat* rgbaFrame) {
	HRESULT hResult = S_OK;
	NvdecService* this = (NvdecService*)_this;

	InterlockedCompareExchangePointer((volatile PVOID*)&this->avTrpBuffer[0], (PVOID*)this->avTrpBuffer[1], this->avTrpBuffer[0]);
	if (SUCCEEDED(hResult)) {
		hResult = this->lpVtbl->decode(_this, this->avTrpBuffer[0], rgbaFrame);
		WIN32ck(__LINE__, __FILE__, hResult);
	}

	while (InterlockedBitTestAndSet(&this->isFrameReady, 0)) {
		if (this->jobDone)
			return this->jobDone;
		Sleep(1);
	};
	return hResult;
}

HRESULT STDMETHODCALLTYPE backend_presentFrame(INvdecService* _this, HWND hwnd, RECT displayArea)
{
	NvdecService* this = (INvdecService*)_this;
	if (hwnd) {
		return this->pDevice->lpVtbl->Present(this->pDevice, &displayArea, NULL, hwnd, NULL);
	}
	return E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE backend_play(INvdecService* _this)
{
	NvdecService* this = (INvdecService*)_this;
	ResumeThread(this->readSyncThread);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE backend_pause(INvdecService* _this)
{
	NvdecService* this = (INvdecService*)_this;
	SuspendThread(this->readSyncThread);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE backend_seekFront(INvdecService* _this, int64_t seconds)
{
	NvdecService* this = (INvdecService*)_this;
	AVStream* m_video_stream = this->avFmtContext->streams[this->videoStreamIndex];
	HRESULT hResult = S_OK;

	int avCode = 0;
	int frame = this->avFramerate * seconds;

	int64_t target_dts_usecs = (int64_t)round(frame
		* (double)m_video_stream->r_frame_rate.den
		/ m_video_stream->r_frame_rate.num * AV_TIME_BASE);

	int64_t first_dts_usecs = (int64_t)round(m_video_stream->first_dts
		* (double)m_video_stream->time_base.num
		/ m_video_stream->time_base.den * AV_TIME_BASE);
	target_dts_usecs += first_dts_usecs;

	avCode = av_seek_frame(this->avFmtContext, -1, target_dts_usecs, AVSEEK_FLAG_FRAME);
	if (avCode < 0) {
		ck(__LINE__, __FILE__, avCode);
		hResult = HRESULT_FROM_WIN32(ERROR_SEEK);
	}

	return hResult;
}

inline double STDMETHODCALLTYPE backend_getDuration(INvdecService* _this)
{
	return (((NvdecService*)_this)->videoDuration);
}

inline double STDMETHODCALLTYPE backend_getFramerate(INvdecService* _this)
{
	return (((NvdecService*)_this)->avFramerate);
}