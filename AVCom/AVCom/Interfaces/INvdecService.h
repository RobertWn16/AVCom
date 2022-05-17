#ifndef _H_INVDECSERVICE_H_
#define _H_INVDECSERVICE_H_

#include <nvcuvid/nvcuvid.h>
#include <nvcuvid/cuviddec.h>
#include <nppi.h>
#include <driver_types.h>
#include <d3d9.h>
#include "IcudaGpuMat.h"

#define NVDEC_DECODING 0
#define NVDEC_DECODING_FILE 1
#define NVDEC_DECODING_WEBCAMERA 2
#ifdef _WIN32
#define NVDEC_DECODING_DEKSTOP 3
#endif // _WIN32

#define NVDEC_END_OFSTREAM -2

DEFINE_GUID(IID_INvdecService, 0xc5586d23, 0x2e7d, 0x40ab, 0xb4, 0x51,
	0xce, 0x3f, 0x5a, 0x63, 0xde, 0x25);

#if defined(__cplusplus) && !defined(CINTERFACE)
MIDL_INTERFACE("C5586D23-2E7D-40AB-B451-CE3F5A63D325")
INvdecService : public IUnknown
{
public:
	/*Create an instance of an object based on IID and alloc the interface handlers and pointers
	of the object interface.

	@Error codes:
	S_OK - succes.
	E_NOINTERFACE - the queried interface not exist based on IID.

	@Params:
	[in]refiid - IID of the queried interface.
	[out]data - pointer to the queried interface.

	@Remarks:
	IID - IUnknown, IcudaGpuMat.
	The user need to set data param to null otherwise the function will fail.*/
	virtual HRESULT STDMETHODCALLTYPE CreateInstance(REFIID refiid, _COM_Outptr_ void** data);

	/*Initialize D3D9 engine and buffer for presentation of frames.

	@Error codes:
	S_OK - success.

	@Params:

	@Remarks
	This method must be call after VideoCapture method because it needs the size of back buffers.
	*/
	virtual HRESULT STDMETHODCALLTYPE initD3D9() = 0;

	/*Reads metadata about url and initialize cuda parser based on nvdec flags.

	@Error codes:
	S_OK - success.
	ERROR_FILE_NOT_FOUND - the url is not valid or file can't be find.
	E_INVALIDARG - file or stream is found but the format is unrecognized.
	ERROR_UNSUPPORTED_COMPRESSION - the codec is recognized but it can't be parsed.
	E_POINTER - internal error. Please contact administrator.

	@Params:
	[in]url - path to the stream or file
	[in]nvdecflags - specify if the content is received through network, local disk or desktop.
	[in]gpuOrdinal - index of the NVIDIA GPU.

	@Remarks:
	nvdecflags - NVDEC_WEBCAMERA(webcam), NVDEC_DECODING_FILE(disk file), NVDEC_DECODING_DESKTOP(dshow capture).
	NVDEC_DECODING_NETWORK(network) and NVDEC_DECODIING_DESKTOP is not fully tested and don't use for now.
	*/
	virtual HRESULT STDMETHODCALLTYPE VideoCapture(char* url, int nvdecflags, int gpuOrdinal) = 0;

	/*Read the video stream with the orginal framerate.

	@Error codes:
	S_OK - success.

	@Params:

	@Remarks:
	This method will be executed and won't start until the decodeSync method is called.
	*/
	virtual HRESULT STDMETHODCALLTYPE readSync() = 0;

	/*Read the current video stream at cpu max speed.
	@Error codes:
	S_OK - success.

	@Params:

	@Remarks:
	This method will be executed and won't start until the decodeSync method is called.*/
	virtual HRESULT STDMETHODCALLTYPE read() = 0;

	/*Get the current width of frame.

	@Error codes:

	@Params:

	@Remarks: Returns the current width of frame through read or readSync method*/
	virtual inline unsigned int STDMETHODCALLTYPE getFrameWidth() = 0;

	/*Get the current heigth of frame.

	@Error codes:

	@Params:

	@Remarks: Returns the current heigth of frame through read or readSync method*/
	virtual inline unsigned int STDMETHODCALLTYPE getFrameHeigth() = 0;

	/*Get the current size of frame in bytes.

	@Error codes:

	@Params:

	@Remarks:*/
	virtual inline long int STDMETHODCALLTYPE getFrameSize() = 0;

	/*Get the pointer to the current frame data(compressed).
	@Error codes:

	@Params:

	@Remarks:*/
	virtual AVPacket* STDMETHODCALLTYPE getFrame() = 0;

	/*Select NVIDIA gpu based on index.

	@Error codes:
	S_OK - success.
	ERROR_INVALID_DEVICE_OBJECT_PARAMETER - index of cuda gpu device is invalid.
	E_INVALIDARG - nvidia device is found but is not compatibile.

	@Params
	[in]ordinal - index of NVIDIA gpu.

	@Remarks:
	Function is called already in VideoCapture and for safety don't use.
	*/
	virtual HRESULT STDMETHODCALLTYPE selectGpuDevice(int ordinal) = 0;

	/*Decode the context at max gpu speed(except live transmissions). Recommend for conversion of videos.

	@Error codes:
	S_OK - success.
	E_UNEXPECTED - unexpected driver error.
	E_INVALIDARG - the data transfered to decoder is invalid or unsupported.

	@Params:
	[in]compressedFrame -  from ffmpeg demuxing usually obtained with getFrame method.
	[out]decodedFrame - decoded frame assigned to VRAM. Decoded frame is a R8G8B8A8 image.

	@Remarks:
	The user must set flag of decodedFrame to gpuMat_R8B8G8A8 in Alloc method.*/
	virtual HRESULT STDMETHODCALLTYPE decode(void* compressedFrame, IcudaGpuMat* decodedFrame) = 0;

	/*Decode the context at video current framerate. Recommend for video players.

	@Error codes:
	S_OK - success.
	E_UNEXPECTED - unexpected driver error.
	E_INVALIDARG - the data transfered to decoder is invalid or unsupported.

	@Params:
	[out]decodedFrame - decoded frame assigned to VRAM. Decoded frame is a R8G8B8A8 image.

	@Remarks:
	Don't use this method for live transmission use decode instead and read method.*/
	virtual HRESULT STDMETHODCALLTYPE decodeSync(IcudaGpuMat* decodedFrame) = 0;

	/*Maps a R8B8G8A8 container to D3D9 surface(texture).

	@Error codes:
	S_OK - success.
	ERROR_RESOURCE_DATA_NOT_FOUND - the backbuffer is not register. Check initD3D9() returned result.

	@Params:
	[in]gpuTexture - decoded frame is register to D3D9Cuda.

	@Remarks:
	The width, heigth and format must be equal with initial video stream opened in VideoCapture.*/
	virtual HRESULT STDMETHODCALLTYPE MapToD3D9Tex(IcudaGpuMat* gpuTexture) = 0;

	/*Present the current decoded frame in an HWND. The user should specify the region where the frame is presented.

	@Error codes:
	S_OK - success.
	E_INVALIDARG - hwnd is not valid.

	@Params:
	[in]hWnd - current window where the context is presented.
	[in]rect - area where the context should be presented.

	@Remarks:*/
	virtual HRESULT STDMETHODCALLTYPE presentFrame(HWND hWnd, RECT rect) = 0;

	/*Plays the current stream if it is paused.

	@Error codes:
	S_OK - success.

	@Params:

	@Remarks:*/
	virtual inline HRESULT STDMETHODCALLTYPE play() = 0;

	/*Pause the current stream if it is playing.

	@Error codes:
	S_OK - success.

	@Params:

	@Remarks:*/
	virtual inline HRESULT STDMETHODCALLTYPE pause() = 0;

	/*Seek the stream to the specified second.

	@Error codes:
	S_OK - success.
	ERROR_SEEK - error in seeking the stream.
	@Params:

	@Remarks:
	Don't use the seek method for live transmission that's not make any sense.*/
	virtual HRESULT STDMETHODCALLTYPE seek(int64_t) = 0;

	/*Get duration of current stream.

	@Error codes:

	@Params:

	@Remarks:*/
	virtual inline double STDMETHODCALLTYPE getDuration() = 0;

	/*Get framerate of current stream.

	@Error codes:

	@Params:

	@Remarks:*/
	virtual inline double STDMETHODCALLTYPE getFramerate() = 0;
};
#else 
typedef interface INvdecService INvdecService;

typedef struct INvdecServiceVtbl {
	HRESULT(STDMETHODCALLTYPE* QueryInterface)(INvdecService*, REFIID, _COM_Outptr_ void** data);
	HRESULT(STDMETHODCALLTYPE* AddRef)(INvdecService*);
	HRESULT(STDMETHODCALLTYPE* Release)(INvdecService*);

	/*Create an instance of an object based on IID and alloc the interface handlers and pointers
	of the object interface.

	@Error codes:
	S_OK - succes.
	E_NOINTERFACE - the queried interface not exist based on IID.

	@Params:
	[in]this - pointer to the current object.
	[in]refiid - IID of the queried interface.
	[out]data - pointer to the queried interface.

	@Remarks:
	IID - IUnknown, IcudaGpuMat.
	The user need to set data param to null otherwise the function will fail.*/
	HRESULT(STDMETHODCALLTYPE* CreateInstance)(INvdecService* _this, REFIID, _COM_Outptr_ void** data);

	/*Initialize D3D9 engine and buffer for presentation of frames.

	@Error codes:
	S_OK - success.

	@Params:
	[in]this - pointer to the current object.
	@Remarks
	This method must be call after VideoCapture method because it needs the size of back buffers.
	*/
	HRESULT(STDMETHODCALLTYPE* initD3D9)(INvdecService*);

	/*Reads metadata about url and initialize cuda parser based on nvdec flags.

	@Error codes:
	S_OK - success.
	ERROR_FILE_NOT_FOUND - the url is not valid or file can't be find.
	E_INVALIDARG - file or stream is found but the format is unrecognized.
	ERROR_UNSUPPORTED_COMPRESSION - the codec is recognized but it can't be parsed.
	E_POINTER - internal error. Please contact administrator.

	@Params:
	[in]this - pointer to the current object.
	[in]url - path to the stream or file
	[in]nvdecflags - specify if the content is received through network, local disk or desktop.
	[in]gpuOrdinal - index of the NVIDIA GPU.

	@Remarks:
	nvdecflags - NVDEC_WEBCAMERA(webcam), NVDEC_DECODING_FILE(disk file), NVDEC_DECODING_DESKTOP(dshow capture).
	NVDEC_DECODING_NETWORK(network) and NVDEC_DECODIING_DESKTOP is not fully tested and don't use for now.
	*/
	HRESULT(STDMETHODCALLTYPE* VideoCapture)(INvdecService*, char*, int, int);

	/*Read the video stream with the orginal framerate.

	@Error codes:
	S_OK - success.

	@Params:
	[in]this - pointer to the current object.
	@Remarks:
	This method will be executed and won't start until the decodeSync method is called.
	*/
	HRESULT(STDMETHODCALLTYPE* readSync)(INvdecService*);

	/*Read the current video stream at cpu max speed.
	@Error codes:
	S_OK - success.

	@Params:
	[in]this - pointer to the current object.

	@Remarks:
	This method will be executed and won't start until the decodeSync method is called.*/
	HRESULT(STDMETHODCALLTYPE* read)(INvdecService*);

	/*Get the current width of frame.

	@Error codes:

	@Params:
	[in]this - pointer to the current object.

	@Remarks:
	Returns the current width of frame through read or readSync method*/
	unsigned int (STDMETHODCALLTYPE* getFrameWidth)(INvdecService*);

	/*Get the current heigth of frame.

	@Error codes:

	@Params:
	[in]this - pointer to the current object.

	@Remarks: Returns the current heigth of frame through read or readSync method*/
	unsigned int (STDMETHODCALLTYPE* getFrameHeigth)(INvdecService*);

	/*Get the current size of frame in bytes.

	@Error codes:

	@Params:
	[in]this - pointer to the current object.

	@Remarks:*/
	long int (STDMETHODCALLTYPE* getFrameSize)(INvdecService*);

	/*Get the pointer to the current frame data(compressed).
	@Error codes:

	@Params:
	[in]this - pointer to the current object.

	@Remarks:*/
	AVPacket* (STDMETHODCALLTYPE* getFrame)(INvdecService*);

	/*Select NVIDIA gpu based on index.

	@Error codes:
	S_OK - success.
	ERROR_INVALID_DEVICE_OBJECT_PARAMETER - index of cuda gpu device is invalid.
	E_INVALIDARG - nvidia device is found but is not compatibile.

	@Params
	[in]this - pointer to the current object.
	[in]ordinal - index of NVIDIA gpu.

	@Remarks:
	Function is already called in VideoCapture and for safety don't use.*/
	HRESULT(STDMETHODCALLTYPE* selectGpuDevice)(INvdecService*, int);


	/*Decode the context at max gpu speed(except live transmissions). Recommend for conversion of videos.

	@Error codes:
	S_OK - success.
	E_UNEXPECTED - unexpected driver error.
	E_INVALIDARG - the data transfered to decoder is invalid or unsupported.

	@Params:
	[in]this - pointer to the current object.
	[in]compressedFrame -  from ffmpeg demuxing usually obtained with getFrame method.
	[out]decodedFrame - decoded frame assigned to VRAM. Decoded frame is a R8G8B8A8 image.

	@Remarks:
	The user must set flag of decodedFrame to gpuMat_R8B8G8A8 in Alloc method.*/
	HRESULT(STDMETHODCALLTYPE* decode)(INvdecService*, void*, IcudaGpuMat*);

	/*Decode the context at video current framerate. Recommend for video players.

	@Error codes:
	S_OK - success.
	E_UNEXPECTED - unexpected driver error.
	E_INVALIDARG - the data transfered to decoder is invalid or unsupported.

	@Params:
	[in]this - pointer to the current object.
	[out]decodedFrame - decoded frame assigned to VRAM. Decoded frame is a R8G8B8A8 image.

	@Remarks:
	Don't use this method for live transmission use decode instead and read method.*/
	HRESULT(STDMETHODCALLTYPE* decodeSync)(INvdecService* this, IcudaGpuMat* decodedFrame);

	/*Maps a R8B8G8A8 container to D3D9 surface(texture).

	@Error codes:
	S_OK - success.
	ERROR_RESOURCE_DATA_NOT_FOUND - the backbuffer is not register. Check initD3D9() returned result.

	@Params:
	[in]this - pointer to the current object.
	[in]gpuTexture - decoded frame is register to D3D9Cuda.

	@Remarks:
	The width, heigth and format must be equal with initial video stream opened in VideoCapture.*/
	HRESULT(STDMETHODCALLTYPE* MapToD3D9Tex)(INvdecService* this, IcudaGpuMat* gpuTexture);

	/*Present the current decoded frame in an HWND. The user should specify the region where the frame is presented.

	@Error codes:
	S_OK - success.
	E_INVALIDARG - hwnd is not valid.

	@Params:
	[in]this - pointer to the current object.
	[in]hWnd - current window where the context is presented.
	[in]rect - area where the context should be presented.

	@Remarks:*/
	HRESULT(STDMETHODCALLTYPE* presentFrame)(INvdecService* this, HWND hWnd, RECT rect);

	/*Plays the current stream if it is paused.

	@Error codes:
	S_OK - success.

	@Params:
	[in]this - pointer to the current object.

	@Remarks:*/
	HRESULT(STDMETHODCALLTYPE* play)(INvdecService* this);

	/*Pause the current stream if it is playing.

	@Error codes:
	S_OK - success.

	@Params:
	[in]this - pointer to the current object.

	@Remarks:*/
	HRESULT(STDMETHODCALLTYPE* pause)(INvdecService* this);

	/*Seek the stream to the specified second.

	@Error codes:
	S_OK - success.
	ERROR_SEEK - error in seeking the stream.
	@Params:
	[in]this - pointer to the current object.
	[in]second - specified second where the audio stream should seek.

	@Remarks:
	Don't use the seek method for live transmission that's not make any sense.*/
	HRESULT(STDMETHODCALLTYPE* seek_front)(INvdecService* this, int64_t second);

	/*Get duration of current stream.

	@Error codes:

	@Params:
	[in]this - pointer to the current object.

	@Remarks:*/
	double (STDMETHODCALLTYPE* getDuration)(INvdecService* this);

	/*Get framerate of current stream.

	@Error codes:

	@Params:
	[in]this - pointer to the current object.

	@Remarks:*/
	double (STDMETHODCALLTYPE* getFramerate)(INvdecService*);


}INvdecServiceVtbl;

interface INvdecService {
	CONST_VTBL INvdecServiceVtbl* lpVtbl;
};

#endif // __cplusplus

#endif // !_H_INVDECSERVICE_H_

