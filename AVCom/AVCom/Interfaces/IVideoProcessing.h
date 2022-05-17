#include "INvdecService.h"
#include "IAudioCapture.h"

typedef interface IVideoProcessing IVideoProcessing;

DEFINE_GUID(CLSID_IVideoProcessing, 0xb44d5758, 0xc7cd, 0x4df8, 0xa2, 0x85,
	0x65, 0x92, 0x6b, 0x39, 0x19, 0x0d);
DEFINE_GUID(IID_IVideoProcessing, 0x6d4ae06c, 0xae4c, 0x4e6f, 0xae, 0x07,
	0xb4, 0x25, 0x7a, 0x53, 0x3c, 0x81);

#if defined(__cplusplus) && !defined(CINTERFACE)
MIDL_INTERFACE("6D4AE06C-AE4C-4E67-AE07-B4257A533C81")
IVideoProcessing : public IUnknown
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
	IID - IUnknown, IAudioCapture, INvdecService.
	The user need to set data param to null otherwise the function will fail.*/
	virtual HRESULT STDMETHODCALLTYPE CreateInstance(REFIID refiid, _COM_Outptr_ void** data);

	/*Initialize the cuda driver. Set driver index to 0.
	@Error codes:
	S_OK - success.
	E_ABORT - fatal error. Probably nvidia gpu doesn't exist.
	
	@Params:
	[in]driverIndex - set to 0 always.
	
	@Remarks:
	Before using all INvdecService functionality the user need to call this method.*/
	virtual HRESULT STDMETHODCALLTYPE initCudaEnv(int driverIndex) = 0;
	virtual HRESULT STDMETHODCALLTYPE getCudaDevice() = 0;
};
#else
typedef struct IVideoProcessingVtbl {
	HRESULT(STDMETHODCALLTYPE* QueryInterface)(IVideoProcessing*, REFIID, _COM_Outptr_ void**);
	HRESULT(STDMETHODCALLTYPE* AddRef)(IVideoProcessing*);
	HRESULT(STDMETHODCALLTYPE* Release)(IVideoProcessing*);

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
	IID - IUnknown, IAudioCapture, INvdecService.
	The user need to set data param to null otherwise the function will fail.*/

	HRESULT(STDMETHODCALLTYPE* CreateInstance)(IVideoProcessing*);
	/*Initialize the cuda driver. Set driver index to 0.
	@Error codes:
	S_OK - success.
	E_ABORT - fatal error. Probably nvidia gpu doesn't exist.

	@Params:
	[in]this - pointer to the current object.
	[in]driverIndex - set to 0 always.

	@Remarks:
	Before using all INvdecService functionality the user need to call this method.*/
	HRESULT(STDMETHODCALLTYPE* initCudaEnv)(IVideoProcessing* this, int driverIndex);
	HRESULT(STDMETHODCALLTYPE* getCudaDevice)(IVideoProcessing*, CUdevice);

}IVideoProcessingVtbl;

interface IVideoProcessing {
	CONST_VTBL IVideoProcessingVtbl* lpVtbl;
};
#endif // __cplusplus
