#pragma once
#include "../Interfaces/IcudaGpuMat.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus
	HRESULT STDMETHODCALLTYPE cudaMat_QueryInterface(IcudaGpuMat*, REFIID, _COM_Outptr_ void** data);
	HRESULT STDMETHODCALLTYPE cudaMat_AddRef(IcudaGpuMat*);
	HRESULT STDMETHODCALLTYPE cudaMat_Release(IcudaGpuMat*);

	HRESULT STDMETHODCALLTYPE cudaMat_Alloc(IcudaGpuMat*, int, size_t, size_t);
	HRESULT STDMETHODCALLTYPE cudaMat_Realloc(IcudaGpuMat*, int, size_t, size_t);
	HRESULT STDMETHODCALLTYPE cudaMat_cloneGPUToArray(IcudaGpuMat*, IcudaGpuMat*);
	HRESULT STDMETHODCALLTYPE cudaMat_cloneGPUMat(IcudaGpuMat*, IcudaGpuMat*);
	//HRESULT(STDMEHTODCALLTYPE* cloneCPU)(IcudaGpuMat*, ICpuMat*);

	HRESULT STDMETHODCALLTYPE cudaMat_convertYUVtoRGB(IcudaGpuMat*, int, IcudaGpuMat*);
	HRESULT STDMETHODCALLTYPE cudaMat_convertRGBtoRGBA(IcudaGpuMat*, IcudaGpuMat*);

	size_t STDMETHODCALLTYPE cudaMat_getWidth(IcudaGpuMat*);
	size_t STDMETHODCALLTYPE cudaMat_getHeigth(IcudaGpuMat*);
	size_t STDMETHODCALLTYPE cudaMat_getPitch(IcudaGpuMat*);
#ifdef __cplusplus
}
#endif // __cplusplus

typedef struct cudaGpuMat {
	CONST_VTBL IcudaGpuMatVtbl* lpVtbl;
	int refCount;

	CUdeviceptr data;
	CUdeviceptr cuLuma;
	CUdeviceptr cuChroma;

	int noOfChannels;
	size_t width;
	size_t heigth;
	size_t pitch;
	size_t size;
	size_t surfaceHeigth;
	int bit_depth_luma;

}cudaGpuMat;

static IcudaGpuMatVtbl IcudaGpuMat_Vtbl = { cudaMat_QueryInterface,
	cudaMat_AddRef,
	cudaMat_Release,
	cudaMat_Alloc,
	cudaMat_Realloc,
	cudaMat_cloneGPUToArray,
	cudaMat_cloneGPUMat,
	cudaMat_convertYUVtoRGB,
	cudaMat_convertRGBtoRGBA,
	cudaMat_getWidth,
	cudaMat_getHeigth,
	cudaMat_getPitch,
};