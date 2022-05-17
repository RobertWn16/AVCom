#include "../include/cudaGpuMat.h"
#include "../include/nvdec_bgr_bgra.cuh"

static float getFactor(int flags) {
	switch (flags) {
	case gpuMat_YUV: return 1.5f;
	case gpuMat_R8B8G8: return 3;
	case gpuMat_R8B8G8A8: return 4;
	default: return 3;
	}
}
static CUresult cuvidMemcpy(CUmemorytype srcType,
	CUdeviceptr src, size_t spitch, CUmemorytype dstType, void* dst, size_t dstPitch, size_t heigth)
{
	CUDA_MEMCPY2D m = { 0 };
	m.srcMemoryType = srcType;
	m.srcDevice = src;
	m.srcPitch = spitch;
	m.dstMemoryType = dstType;
	m.dstDevice =  dst;
	m.dstPitch = dstPitch;
	m.WidthInBytes = dstPitch;
	m.Height = heigth;

	return cuMemcpy2D(&m);
}

HRESULT STDMETHODCALLTYPE cudaMat_QueryInterface(IcudaGpuMat* _this, REFIID ref, _COM_Outptr_ void** data)
{
	*data = NULL;
	if (IsEqualIID(ref, &IID_IUnknown)) {
		return S_OK;
	}
	return E_NOINTERFACE;
}
HRESULT STDMETHODCALLTYPE cudaMat_AddRef(IcudaGpuMat* _this)
{
	return ++((cudaGpuMat*)_this)->refCount;
}
HRESULT STDMETHODCALLTYPE cudaMat_Release(IcudaGpuMat* _this)
{
	cudaGpuMat* this = (cudaGpuMat*)_this;
	this->refCount--;
	if (!this->refCount) {
		if (this->cuLuma)
			cuMemFree(this->cuLuma);

		if (this->cuChroma)
			cuMemFree(this->cuChroma);

		if (this->data)
			cuMemFree(this->data);
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE cudaMat_Alloc(cudaGpuMat* _this, int flagChannels, size_t width,size_t heigth)
{
	HRESULT hResult = S_OK;
	CUresult cuResult = CUDA_SUCCESS;

	_this->data = NULL;
	_this->noOfChannels = getFactor(flagChannels);
	_this->width = width;
	_this->heigth = heigth;
	_this->size = width * heigth * _this->noOfChannels;
	_this->pitch = width * getFactor(flagChannels);
	_this->cuChroma = NULL;
	_this->cuLuma = NULL;

	cuResult = cuMemAlloc(&_this->data, _this->size);
	if (SUCCEEDED(hResult)) {
		if (cuResult) {
			CUDAck(__LINE__, __FILE__, hResult);
			hResult = E_OUTOFMEMORY;
		}
	}

	return hResult;
}
HRESULT STDMETHODCALLTYPE cudaMat_Realloc(cudaGpuMat* _this,int flagChannels, size_t width, size_t heigth)
{
	HRESULT hResult = S_OK;
	CUresult cuResult = CUDA_SUCCESS;

	if (SUCCEEDED(hResult)) {
		if (cuResult == CUDA_SUCCESS) {
			if (_this->data)
				cuResult = cuMemFree(_this->data);
			else
				cuResult = CUDA_ERROR_INVALID_VALUE;
			if (cuResult) {
				hResult = E_INVALIDARG;
			}
			CUDAck(__LINE__, __FILE__, cuResult);
		}
	}

	if (SUCCEEDED(hResult)) {
		hResult = cudaMat_Alloc(_this, flagChannels, width, heigth);
		WIN32ck(__LINE__, __FILE__, hResult);
	}
	return hResult;
}
HRESULT STDMETHODCALLTYPE cudaMat_cloneGPUToArray(cudaGpuMat* _this, cudaGpuMat* gpuMat)
{
	if (gpuMat)
	{
		/*if (gpuMat->data_array) {
			_this->cuResult = cuvidMemcpy(CU_MEMORYTYPE_DEVICE,
				_this->data,
				_this->pitch,
				CU_MEMORYTYPE_ARRAY,
				gpuMat->data_array,
				_this->pitch,
				_this->heigth);
			return _this->cuResult;
		}*/
	}

	return E_INVALIDARG;
}
HRESULT STDMETHODCALLTYPE cudaMat_cloneGPUMat(cudaGpuMat* _this, cudaGpuMat* gpuMat)
{
	HRESULT hResult = S_OK;
	CUresult cuResult = S_OK;

	if (SUCCEEDED(hResult)) {
		if (gpuMat->data) {
			if (gpuMat->pitch != _this->pitch) {
				if (gpuMat->data)
					cuResult = cuMemFree(gpuMat->data);
				else
					cuResult = CUDA_ERROR_INVALID_VALUE;
				if (cuResult) {
					CUDAck(__LINE__, __FILE__, cuResult);
					hResult = E_INVALIDARG;
				}
				if (SUCCEEDED(hResult)) {
					cuResult = cuMemAlloc(gpuMat->data, _this->pitch * _this->heigth);
					if (cuResult) {
						CUDAck(__LINE__, __FILE__, cuResult);
						hResult = E_OUTOFMEMORY;
					}
				}
			}

		}
	}

	if (SUCCEEDED(hResult)) {
		cuResult = cuvidMemcpy(CU_MEMORYTYPE_DEVICE,
			_this->data,
			_this->pitch,
			CU_MEMORYTYPE_DEVICE,
			gpuMat->data,
			_this->pitch,
			_this->heigth);

		if (cuResult) {
			CUDAck(__LINE__, __FILE__, cuResult);
			hResult = E_INVALIDARG;
		}
	}

	return hResult;
}

HRESULT STDMETHODCALLTYPE cudaMat_convertYUVtoRGB(cudaGpuMat* _this, int flagsPixelFormat, cudaGpuMat* dest)
{
	HRESULT hResult = S_OK;
	CUresult cuResult = CUDA_SUCCESS;

	if (flagsPixelFormat == gpuMat_NV128u) {
		Npp8u* npImage[2];
		if (SUCCEEDED(hResult)) {
			if (!_this->cuLuma) {
				cuResult = cuMemAlloc(&_this->cuLuma, _this->width * _this->heigth);
				if (cuResult) {
					CUDAck(__LINE__, __FILE__, cuResult);
					hResult = E_OUTOFMEMORY;
				}
			}
		}

		if (SUCCEEDED(hResult)) {
			if (!_this->cuChroma) {
				cuResult = cuMemAlloc(&_this->cuChroma, _this->width * _this->heigth / 2);
				if (cuResult) {
					CUDAck(__LINE__, __FILE__, cuResult);
					hResult = E_OUTOFMEMORY;
				}
			}
		}

		if (SUCCEEDED(hResult)) {
			cuResult = cuvidMemcpy(CU_MEMORYTYPE_DEVICE, _this->data,
				_this->pitch,
				CU_MEMORYTYPE_DEVICE,
				_this->cuLuma,
				_this->width,
				_this->heigth);
			if (cuResult) {
				CUDAck(__LINE__, __FILE__, cuResult);
				hResult = E_INVALIDARG;
			}
		}

		if (SUCCEEDED(hResult)) {
			cuResult = cuvidMemcpy(CU_MEMORYTYPE_DEVICE,
				_this->data + (_this->pitch * _this->surfaceHeigth),
				_this->pitch,
				CU_MEMORYTYPE_DEVICE,
				_this->cuChroma,
				_this->width,
				_this->heigth >> 1);

			if (cuResult) {
				CUDAck(__LINE__, __FILE__, cuResult);
				hResult = E_INVALIDARG;
			}
		}

		if (SUCCEEDED(hResult)) {
			npImage[0] = (Npp8u*)_this->cuLuma;
			npImage[1] = (Npp8u*)_this->cuChroma;

			if (_this->data && dest->data) {
				NppStatus nppStat;
				NppiSize sz;
				memset(&sz, 0, sizeof(NppiSize));
				sz.width = _this->width;
				sz.height = _this->heigth;

				nppStat = nppiNV12ToRGB_8u_P2C3R(npImage,
					_this->width,
					(Npp8u*)dest->data,
					dest->pitch, sz);
				if (nppStat) {
					CUDAck(__LINE__, __FILE__, nppStat);
					hResult = E_INVALIDARG;
				}
			}
			else {
				cuResult = CUDA_ERROR_INVALID_VALUE;
				hResult = E_INVALIDARG;
				CUDAck(__LINE__, __FILE__, cuResult);
			}
		}
	}
	else
		hResult = E_NOTIMPL;

	return hResult;
}
HRESULT STDMETHODCALLTYPE cudaMat_convertRGBtoRGBA(cudaGpuMat* _this, cudaGpuMat* gpuMat)
{
	if (_this && gpuMat && _this->data && gpuMat->data) {
		bgr_bgra(gpuMat->data,
			_this->data,
			0,
			0,
			_this->width,
			_this->heigth);
		return S_OK;
	}
	else
		WIN32ck(__LINE__, __FILE__, E_INVALIDARG);

	return E_INVALIDARG;
}

size_t STDMETHODCALLTYPE cudaMat_getWidth(cudaGpuMat* _this)
{
	return _this->width;
}
size_t STDMETHODCALLTYPE cudaMat_getHeigth(cudaGpuMat* _this)
{
	return _this->heigth;
}
size_t STDMETHODCALLTYPE cudaMat_getPitch(cudaGpuMat* _this)
{
	return _this->pitch;
}