#include "../Interfaces/INvdecService.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
	HRESULT STDMETHODCALLTYPE backend_QueryInterface(INvdecService*, REFIID, _COM_Outptr_ void** data);
	HRESULT STDMETHODCALLTYPE backend_AddRef(INvdecService*);
	HRESULT STDMETHODCALLTYPE backend_Release(INvdecService*);
	HRESULT STDMETHODCALLTYPE backend_CreateInstance(INvdecService* , REFIID, _COM_Outptr_ void** data);

	HRESULT STDMETHODCALLTYPE backend_initD3D9(INvdecService*);
	HRESULT STDMETHODCALLTYPE backend_initXAudio(INvdecService*);
	HRESULT STDMETHODCALLTYPE backend_VideoCapture(INvdecService*, char*, int, int);
	HRESULT STDMETHODCALLTYPE backend_readSync(INvdecService*);
	HRESULT STDMETHODCALLTYPE backend_read(INvdecService*);
	inline unsigned int STDMETHODCALLTYPE backend_getFrameWidth(INvdecService*);
	inline unsigned int STDMETHODCALLTYPE backend_getFrameHeigth(INvdecService*);
	inline long int STDMETHODCALLTYPE backend_getFrameSize(INvdecService*);
	inline AVPacket* STDMETHODCALLTYPE backend_getFrame(INvdecService*);

	HRESULT STDMETHODCALLTYPE backend_selectGpuDevice(INvdecService*, int);
	HRESULT STDMETHODCALLTYPE backend_decode(INvdecService*, void*, IcudaGpuMat*);
	HRESULT STDMETHODCALLTYPE backend_decodeSync(INvdecService*, IcudaGpuMat*);

	HRESULT STDMETHODCALLTYPE backend_mapToD3D9Tex(INvdecService*, IcudaGpuMat*);
	HRESULT STDMETHODCALLTYPE backend_presentFrame(INvdecService*, HWND, RECT);

	HRESULT STDMETHODCALLTYPE backend_play(INvdecService*);
	HRESULT STDMETHODCALLTYPE backend_pause(INvdecService*);
	HRESULT STDMETHODCALLTYPE backend_seekFront(INvdecService*, int64_t seconds);

	inline double STDMETHODCALLTYPE backend_getDuration(INvdecService*);
	inline double STDMETHODCALLTYPE backend_getFramerate(INvdecService*);

#ifdef __cplusplus
}
#endif // __cplusplus

typedef struct NvdecService {
	CONST_VTBL INvdecServiceVtbl* lpVtbl;
	int refCount;

	//FFMPEG context for demuxing the content
	AVFormatContext* avFmtContext;
	SwrContext* swrContext;
	AVInputFormat* avInFmt;
	AVPacket pck[3];
	AVPacket fltPck[3];
	AVPacket* avTrpBuffer[3];
	AVPacket* audioBuffer[3];
	AVBitStreamFilter* avBtStmFilter;
	AVBSFContext* avBSFContext;
	AVCodec* audioCodec;
	AVFrame* audioFrame;
	uint16_t* audio_raw_data;
	int audio_raw_size;

	double avFramerate;
	double videoDuration;
	int avCurrIndex;
	BOOL isFrameReady;
	int decType;
	int avCodecId;
	int audioCodecId;
	int videoStreamIndex;
	int audioStreamIndex;
	int avCode;

	//NVDEC PARAMS FOR HW ACCELERATION DECODING
	CUcontext cuContext;
	CUdevice device;
	CUvideoparser cuParser;
	CUvideodecoder cuDecoder;
	CUdeviceptr cuFrame;
	CUgraphicsResource cuResource;
	CUVIDPARSERPARAMS cuParserParams;
	CUVIDSOURCEDATAPACKET cuPck;
	cudaVideoCodec cuvidCodec;
	cudaArray_t dstArray;

	int bith_depth_luma8;
	int maxNumOfSurfaces;
	int ordinal;

	IcudaGpuMat* decFrame;
	IcudaGpuMat* rgbFrame;
	IcudaGpuMat* rgbaFrame;

	int surfaceWidth;
	int surfaceHeigth;
	unsigned int frame_width;
	unsigned int frame_heigth;

	CUresult cuResult;
	HRESULT hResult;

	//NPPI API buffer;
	Npp8u* npFrame[2];

	//DIRECTX PRESENTERS
	IDirect3DDevice9* pDevice;
	IDirect3DSurface9* pBackBuffer;
	IDirect3DSurface9* pSurface;

	//Events
	HANDLE isTaskrdy;
	BOOL jobDone;
	BOOL isDisplayTriggered;

	//Handles
	HANDLE readSyncThread;
	HANDLE masterClock;

}NvdecService;

static const INvdecServiceVtbl INvdecService_Vtbl = { backend_QueryInterface,
	backend_AddRef,
	backend_Release,
	backend_CreateInstance,
	backend_initD3D9,
	backend_VideoCapture,
	backend_readSync,
	backend_read,
	backend_getFrameWidth,
	backend_getFrameHeigth,
	backend_getFrameSize,
	backend_getFrame,
	backend_selectGpuDevice,
	backend_decode,
	backend_decodeSync,
	backend_mapToD3D9Tex,
	backend_presentFrame,
	backend_play,
	backend_pause,
	backend_seekFront,
	backend_getDuration,
	backend_getFramerate
};