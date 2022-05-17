#ifndef _H_VIDEOPROCESSING_H_
#define _H_VIDEOPROCESSING_H_

#include "../Interfaces/IVideoProcessing.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
	HRESULT STDMETHODCALLTYPE videoProcessing_QueryInterface(IVideoProcessing*, REFIID, _COM_Outptr_ void**);
	HRESULT STDMETHODCALLTYPE videoProcessing_AddRef(IVideoProcessing*);
	HRESULT STDMETHODCALLTYPE videoProcessing_Release(IVideoProcessing*);
	HRESULT STDMETHODCALLTYPE videoProcessing_initCudaEnv(IVideoProcessing*, int);
	HRESULT STDMETHODCALLTYPE videoProcessing_getCudaDevice(IVideoProcessing*, CUdevice);
	HRESULT STDMETHODCALLTYPE videoProcessing_CreateInstance(IVideoProcessing*, REFIID refiid, _COM_Outptr_ void** data);
#ifdef __cplusplus
}
#endif // __cpluspls

typedef struct VideoProcessing {
	CONST_VTBL IVideoProcessingVtbl* lpVtbl;
	int refCount;
	CUdevice device;
}VideoProcessing;

static IVideoProcessingVtbl IVideoProcessing_Vtbl = { videoProcessing_QueryInterface,
videoProcessing_AddRef,
videoProcessing_Release,
videoProcessing_CreateInstance,
videoProcessing_initCudaEnv,
videoProcessing_getCudaDevice
};

#endif // !_H_AUDIOCAPTURE_H_

