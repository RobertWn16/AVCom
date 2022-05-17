#ifndef _H_ICUDAGPUMAT_H_
#define _H_ICUDAGPUMAT_H_

#include <cuda.h>
#include <nppi.h>
#include <initguid.h>
#include "dll_err.h"
#pragma warning(disable : 4996)

#define gpuMat_YUV 0
#define gpuMat_R8B8G8 1
#define gpuMat_R8B8G8A8 2

#define gpuMat_NV128u 40

DEFINE_GUID(IID_IcudaGpuMat, 0x656b833d, 0xe9b6, 0x49b8, 0xb7, 0x1d,
	0xc4, 0x1b, 0xd1, 0xd8, 0xdd, 0xbd);

typedef interface IcudaGpuMat IcudaGpuMat;

#if defined(__cplusplus) && !defined(CINTERFACE) 
MIDL_INTERFACE("656B833D-E9B6-49B8-B71D-C41BD1D8DDBD")
IcudaGpuMat : public IUnknown
{
public:
	BEGIN_INTERFACE

		/*IcudaGpuMat object is assigned to GPU VRAM(cuda) specifying the width, heigth and number of channels.

		@Error codes:
		S_OK - success.
		E_OUTOFMEMORY - out of memory on GPU.

		@Params:
		[in]flags - number of channels depending on image format.
					Flags can be found in github readme.
		[in]width - input width of buffer.
		[in]heigth - input heigth of buffer.
		@Remarks:
		Not all image formats are supported.
		*/
		virtual HRESULT STDMETHODCALLTYPE Alloc(int flags, size_t width, size_t heigth) = 0;

	/*If IcudaGpuMat object is in use then the gpu memory assigned to this is freed.
	The user now can assign new dimension and format to the current object.

	@Error codes:
	S_OK - success.
	E_OUTOFMEMORY - out of memory on GPU.

	@Params:
	[in]flags - number of channels depending on image format.
				Flags can be found in github readme.
	[in]width - input width of buffer.
	[in]heigth - input heigth of buffer.
	@Remarks:
	Not all image formats are supported. */
	virtual HRESULT STDMETHODCALLTYPE Realloc(int flags, size_t width, size_t heigth) = 0;

	/*Not fully tested. Don't use for now*/
	virtual HRESULT STDMETHODCALLTYPE cloneGPUToArray(IcudaGpuMat* dest) = 0;

	/*Copy the source container to destination container.

	@Error codes:
	S_OK - success.
	E_INVALIDARG - the dimensions of destination container aren't equal to source or destination container is invalid.
	E_OUTOFMEMORY - out of memory on GPU.

	@Params:
	[out]dest - destination container where data is transfered.

	@Remarks
	The dimensions of source container and destination container must be equal.
	*/
	virtual HRESULT STDMETHODCALLTYPE cloneGpuMat(IcudaGpuMat* dest) = 0;

	/*Convert the current YUV container to the RGB container

	@Error codes:
	S_OK - success.
	E_INVALIDARG - the width and heigth of destination container aren't equal to source or destination container is invalid.
	E_OUTOFMEMORY - out of memory on GPU.

	@Params:
	[out]dest - destination container where data is transfered.

	@Remarks
	The width and heigth of source container and destination container must be equal. */
	virtual HRESULT STDMETHODCALLTYPE convertYUVtoRGB(int flagsPixelFormat, IcudaGpuMat* dest) = 0;

	/*Convert the current RGB container to the RGBA container

	@Error codes:
	S_OK - success.
	E_INVALIDARG - the width and heigth of destination container aren't equal to source or destination container is invalid.
	E_OUTOFMEMORY - out of memory on GPU.

	@Params:
	[in] flagsPixelFormat - yuv pixel format. For supported yuv format check github readme.
	[out]dest - destination container where data is transfered.

	@Remarks
	The width and heitgh of source container and destination container must be equal.*/
	virtual HRESULT STDMETHODCALLTYPE convertRGBtoRGBA(IcudaGpuMat* dest) = 0;

	/*Get the current width of container.

	@Error codes:

	@Params:

	@Remarks:*/
	virtual size_t STDMETHODCALLTYPE getWidth() = 0;

	/*Get the current heigth of container.
	@Error codes:

	@Params:

	@Remarks:*/
	virtual size_t STDMETHODCALLTYPE getHeigth() = 0;

	/*Get the current pitch of container.
	@Error codes:

	@Params:

	@Remarks:*/
	virtual size_t STDMETHODCALLTYPE getPitch() = 0;

	END_INTERFACE
};
#else

typedef struct IcudaGpuMatVtbl {
	/*Query an interface object and increment reference count. In case of success the method will return S_OK.

	@Error codes:
	S_OK - succes.
	E_NOINTERFACE - the queried interface not exist based on IID.

	@Params:
	[in]this - pointer to the current object.
	[in]refiid - IID of the queried interface.
	[out]data - pointer to the queried interface.

	@Remarks:
	IID - IUnknown.*/
	HRESULT(STDMETHODCALLTYPE* QueryInterface)(IcudaGpuMat*, REFIID, _COM_Outptr_ void** data);

	/*Increment reference count of current object.
	@Eror codes:
	S_OK - success.

	@Params:
	[in]this - pointer to the current object.
	*/
	HRESULT(STDMETHODCALLTYPE* AddRef)(IcudaGpuMat*);

	/*Decrement reference count of the current object. If reference count is 0 then object will be freed.

	@Error codes:
	S_OK - success.
	@Params:
	[in]this - pointer to the current object.

	@Remarks:
	If the refernce count is equal to 0 then the object will be freed. The current object is not set
	to null and the user need to initialize the object with NULL.
	*/
	HRESULT(STDMETHODCALLTYPE* Release)(IcudaGpuMat*);

	/*IcudaGpuMat object is assigned to GPU VRAM(cuda) specifying the width, heigth and number of channels.

	@Error codes:
	S_OK - success.
	E_OUTOFMEMORY - out of memory on GPU.

	@Params:
	[in]this - pointer to the current object.
	[in]flags - number of channels depending on image format.
				Flags can be found in github readme.
	[in]width - input width of buffer.
	[in]heigth - input heigth of buffer.
	@Remarks:
	Not all image formats are supported.
	*/
	HRESULT(STDMETHODCALLTYPE* Alloc)(IcudaGpuMat* this, int flags, size_t, size_t);

	/*If IcudaGpuMat object is in use then the gpu memory assigned to this is freed.
	The user now can assign new dimension and format to the current object.

	@Error codes:
	S_OK - success.
	E_OUTOFMEMORY - out of memory on GPU.

	@Params:
	[in]this - pointer to the current object.
	[in]flags - number of channels depending on image format.
				Flags can be found in github readme.
	[in]width - input width of buffer.
	[in]heigth - input heigth of buffer.
	@Remarks:
	Not all image formats are supported. */
	HRESULT(STDMETHODCALLTYPE* Realloc)(IcudaGpuMat* this, int flags, size_t width, size_t heigth);

	/*Not fully tested. Don't use for now*/
	HRESULT(STDMETHODCALLTYPE* cloneGPUToArray)(IcudaGpuMat*, IcudaGpuMat*);

	/*Copy the source container to destination container.

	@Error codes:
	S_OK - success.
	E_INVALIDARG - the dimensions of destination container aren't equal to source or destination container is invalid.
	E_OUTOFMEMORY - out of memory on GPU.

	@Params:
	[in]this - pointer to the current object.
	[out]dest - destination container where data is transfered.

	@Remarks
	The dimensions of source container and destination container must be equal.
	*/
	HRESULT(STDMETHODCALLTYPE* cloneGPUMat)(IcudaGpuMat* this, IcudaGpuMat* dest);

	//HRESULT(STDMEHTODCALLTYPE* cloneCPU)(IcudaGpuMat*, ICpuMat*);

	/*Convert the current YUV container to the RGB container

	@Error codes:
	S_OK - success.
	E_INVALIDARG - the width and heigth of destination container aren't equal to source or destination container is invalid.
	E_OUTOFMEMORY - out of memory on GPU.

	@Params:
	[in]this - pointer to the current object.
	[out]dest - destination container where data is transfered.

	@Remarks
	The width and heigth of source container and destination container must be equal. */
	HRESULT(STDMETHODCALLTYPE* convertYUVtoRGB)(IcudaGpuMat* this, int flagsPixelFormat, IcudaGpuMat* dest);

	/*Convert the current RGB container to the RGBA container

	@Error codes:
	S_OK - success.
	E_INVALIDARG - the width and heigth of destination container aren't equal to source or destination container is invalid.
	E_OUTOFMEMORY - out of memory on GPU.

	@Params:
	[in]this - pointer to the current object.
	[in] flagsPixelFormat - yuv pixel format. For supported yuv format check github readme.
	[out]dest - destination container where data is transfered.

	@Remarks
	The width and heitgh of source container and destination container must be equal.*/
	HRESULT(STDMETHODCALLTYPE* convertRGBtoRGBA)(IcudaGpuMat* this, IcudaGpuMat* des);

	/*Get the current width of container.

	@Error codes:

	@Params:
	[in]this -  pointer to the current object

	@Remarks:*/
	size_t(STDMETHODCALLTYPE* getWidth)(IcudaGpuMat* this);

	/*Get the current heigth of container.
	@Error codes:

	@Params:
	[in]this -  pointer to the current object

	@Remarks:*/
	size_t(STDMETHODCALLTYPE* getHeigth)(IcudaGpuMat* this);

	/*Get the current pitch of container.
	@Error codes:

	@Params:
	[in]this -  pointer to the current object

	@Remarks:*/
	size_t(STDMETHODCALLTYPE* getPitch)(IcudaGpuMat* this);

}IcudaGpuMatVtbl;

interface IcudaGpuMat
{
	CONST_VTBL IcudaGpuMatVtbl* lpVtbl;
};
#endif // __cplusplus

#endif // !_H_IcudaGpuMat_H_

