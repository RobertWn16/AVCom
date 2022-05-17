#pragma once
#include <xaudio2.h>
#include "dll_err.h"

typedef interface IAudioCapture IAudioCapture;
DEFINE_GUID(IID_IAudioCapture, 0xb41cca3f, 0x2a73, 0x4fbc, 0xaf, 0xe6,
    0xb7, 0xe6, 0xac, 0x6c, 0x32, 0x97);

#if defined(__cplusplus) && !defined(CINTERFACE)
MIDL_INTERFACE("B41CCA3F-2A73-4FBC-AFE6-B7E6AC6C3297")
IAudioCapture : IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE CreateInstance(REFIID, _COM_Outptr_ void**);
    /*Open and check input of audio stream.
    @Error codes:
    S_OK - success.
    E_INVALIDARG - the input audio stream is not found or is not valid.
    ERROR_NOT_FOUND - the input is found but is not valid.
    ERROR_DATA_NOT_ACCEPTED - the input contains audio data but the audio decoder
                            can't recognize the codec.
    ERROR_UNSUPPORTED_TYPE - the decoder is found but is not supported on current version.

    @Params:
    [in]url - path to the current audio stream.

    @Remarks:
    *In this version supported audio codecs: AAC, MPEG. */
    virtual HRESULT STDMETHODCALLTYPE XAudioCapture(const char* url) = 0;

    /*Init and create XAudio2 engine.
    @Error codes:
    S_OK - success.

    @Params

    @Remarks
    Supported minimum OS: Windows XP and later.*/
    virtual HRESULT STDMETHODCALLTYPE initXAudio() = 0;

    /*Starts and play the the audio stream on the default output device.
    @Error codes:
    S_OK - success.
    E_NOTIMPL - the current file format is not implemented.
    E_OUTOFMEMORY - out of space to alloc triggger callbacks.
    Reference at line 268 IAudioCapture.c.

    @Params:

    @Remarks:
    The start method will be executed on another thread.*/
    virtual HRESULT STDMETHODCALLTYPE start() = 0;

    /*Plays the current audio stream.
    @Error codes:
    S_OK - success.

    @Params:
    [in]this - pointer to the current object.*/
    virtual HRESULT STDMETHODCALLTYPE play() = 0;

    /*Pause the current audio stream
    @Error codes:
    S_OK - success.

    @Params:
    [in]this - pointer to the current object.*/
    virtual HRESULT STDMETHODCALLTYPE pause() = 0;
    virtual HRESULT STDMETHODCALLTYPE stop() = 0;

    /*Seek the audio stream to the specified second.
    @Error codes:
    S_OK - success.
    ERROR_SEEK - fails to seek current stream.

    @Params:
    [in] - pointer to the current object.
    [in] - specified second where the audio stream should seek.

    @Remarks:
    The audio stream may not seek to exactly frame second but this
    incovenient is not observable.
    */
    virtual HRESULT STDMETHODCALLTYPE seek(int64_t second) = 0;
};

#else
typedef struct IAudioCaptureVtbl {
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
    HRESULT(STDMETHODCALLTYPE* QueryInterface)(IAudioCapture* this, REFIID refiid, _COM_Outptr_ void** data);

    /*Increment reference count of current object.

    @Eror codes:
    S_OK - success.
    @Params:
    [in]this - pointer to the current object.
    */
    HRESULT(STDMETHODCALLTYPE* AddRef)(IAudioCapture* this);

    /*Decrement reference count of the current object. If reference count is 0 then object will be freed.

    @Error codes:
    S_OK - success.
    @Params:
    [in]this - pointer to the current object.

    @Remarks:
    If the refernce count is equal to 0 then the object will be freed. The current object is not set
    to null and the user need to initialize the object with NULL.
    */
    HRESULT(STDMETHODCALLTYPE* Release)(IAudioCapture* this);

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
    IID - IUnknown.
    The user need to set data param to null otherwise the function will fail.*/
    HRESULT(STDMETHODCALLTYPE* CreateInstance)(IAudioCapture* this, REFIID refiid, _COM_Outptr_ void** data);

    /*Open and check input of audio stream.
    @Error codes:
    S_OK - success.
    E_INVALIDARG - the input audio stream is not found or is not valid.
    ERROR_NOT_FOUND - the input is found but is not valid.
    ERROR_DATA_NOT_ACCEPTED - the input contains audio data but the audio decoder
                            can't recognize the codec.
    ERROR_UNSUPPORTED_TYPE - the decoder is found but is not supported on current version.
    @Params:
    [in]this - pointer to the current object.
    [in]url - path to the current audio stream.

    @Remarks:
    In this version supported audio codecs: AAC, MPEG. */
    HRESULT(STDMETHODCALLTYPE* XAudioCapture)(IAudioCapture* this, const char* url);

    /*Init and create XAudio2 engine.
    @Error codes:
    S_OK - success.

    @Params:
    [in]this - pointer to the current object.

    @Remarks:
    Supported minimum OS: Windows XP and later.

    @Remarks:
    The user needs to call XAudioCapture method otherwise the method will fail.*/
    HRESULT(STDMETHODCALLTYPE* initXAudio)(IAudioCapture* this);

    /*Starts and play the the audio stream on the default output device.
    @Error codes:
    S_OK - success.
    E_NOTIMPL - the current file format is not implemented.
    E_OUTOFMEMORY - out of space to alloc triggger callbacks
    Reference at line 268 IAudioCapture.c

    @Params:
    [in]this -> pointer to the current object.

    @Remarks:
    The start method will be executed on another thread.*/
    HRESULT(STDMETHODCALLTYPE* start)(IAudioCapture* this);

    /*Plays the current audio stream.
    @Error codes:
    S_OK - success.

    @Params:
    [in]this - pointer to the current object.*/
    HRESULT(STDMETHODCALLTYPE* play)(IAudioCapture* this);

    /*Pause the current audio stream
    @Error codes:
    S_OK - success.

    @Params:
    [in]this - pointer to the current object.*/
    HRESULT(STDMETHODCALLTYPE* pause)(IAudioCapture* this);
    HRESULT(STDMETHODCALLTYPE* stop)(IAudioCapture*);

    /*Seek the audio stream to the specified second.
    @Error codes:
    S_OK - succes.
    ERROR_SEEK - fails to seek current stream.

    @Params:
    [in]this - pointer to the current object.
    [in]second - specified second where the audio stream should seek.

    @Remarks:
    The audio stream may not seek to exactly frame second but this
    incovenient is not observable.
    */
    HRESULT(STDMETHODCALLTYPE* seek)(IAudioCapture* this, INT64 second);
}IAudioCaptureVtbl;

interface IAudioCapture
{
    CONST_VTBL IAudioCaptureVtbl* lpVtbl;
};

#endif // __cplusplus
