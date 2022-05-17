#include "../include/VideoProcessing.h"
#include <tchar.h>

#pragma comment (lib, "avformat")
#pragma comment (lib, "avcodec")
#pragma comment (lib, "avdevice")
#pragma comment (lib, "avutil")
#pragma comment (lib, "swresample")
#pragma comment (lib, "cuda")
#pragma comment (lib, "nvcuvid")
#pragma comment (lib, "nppim")
#pragma comment (lib, "nppicc")
#pragma comment (lib, "nppisu")
#pragma comment (lib, "d3d9")
#pragma comment (lib, "d3d11")
#pragma comment (lib, "dxgi")
#pragma comment (lib, "dxguid")

static IClassFactory videoFactoryObj;
HMODULE videoModule;
DWORD OutstandingObjects;
DWORD LockCount;
static const TCHAR	GUID_Format[] = _T("{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}");

HRESULT STDMETHODCALLTYPE factoryvideoprocessing_CreateInstance(IClassFactory*, IUnknown*, REFIID, void**);
HRESULT STDMETHODCALLTYPE factoryvideoprocessing_QueryInterface(IClassFactory*, REFIID, void**);
ULONG STDMETHODCALLTYPE factoryvideoprocessing_AddRef(IClassFactory*);
ULONG STDMETHODCALLTYPE factoryvideoprocessing_Release(IClassFactory*);
HRESULT STDMETHODCALLTYPE factoryvideoprocessing_LockServer(IClassFactory*, BOOL);

static IClassFactoryVtbl IClassFactory_Vtbl = {factoryvideoprocessing_QueryInterface,
factoryvideoprocessing_AddRef,
factoryvideoprocessing_Release,
factoryvideoprocessing_CreateInstance,
factoryvideoprocessing_LockServer };

ULONG STDMETHODCALLTYPE factoryvideoprocessing_AddRef(IClassFactory* this)
{
    InterlockedIncrement(&OutstandingObjects);
    return(1);
}

HRESULT STDMETHODCALLTYPE factoryvideoprocessing_QueryInterface(IClassFactory* this, REFIID factoryGuid, void** ppv)
{
    if (IsEqualIID(factoryGuid, &IID_IUnknown) || IsEqualIID(factoryGuid, &IID_IClassFactory))
    {
        this->lpVtbl->AddRef(this);
        *ppv = this;

        return S_OK;
    }
    *ppv = 0;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE factoryvideoprocessing_Release(IClassFactory* this)
{
    return(InterlockedDecrement(&OutstandingObjects));
}

HRESULT STDMETHODCALLTYPE factoryvideoprocessing_CreateInstance(IClassFactory* this, IUnknown* punkOuter, REFIID vTableGuid, void** objHandle)
{
    HRESULT hResult = S_OK;
    register IVideoProcessing* thisobj = NULL;
    *objHandle = 0;

    if (punkOuter)
        hResult = CLASS_E_NOAGGREGATION;
    else
    {
        if (!(thisobj = (IVideoProcessing*)malloc(sizeof(VideoProcessing))))
            hResult = E_OUTOFMEMORY;
        else {
            ((VideoProcessing*)thisobj)->refCount = 1;
            thisobj->lpVtbl = &IVideoProcessing_Vtbl;
        }
        *objHandle = thisobj;
    }

    return S_OK;
}
HRESULT STDMETHODCALLTYPE factoryvideoprocessing_LockServer(IClassFactory* this, BOOL flock)
{
    if (flock) InterlockedIncrement(&LockCount);
    else InterlockedDecrement(&LockCount);

    return NOERROR;
}

static void stringFromCLSID(LPTSTR buffer, REFCLSID ri)
{
    wsprintf(buffer, &GUID_Format[0],
        ((REFCLSID)ri)->Data1, ((REFCLSID)ri)->Data2, ((REFCLSID)ri)->Data3, ((REFCLSID)ri)->Data4[0],
        ((REFCLSID)ri)->Data4[1], ((REFCLSID)ri)->Data4[2], ((REFCLSID)ri)->Data4[3],
        ((REFCLSID)ri)->Data4[4], ((REFCLSID)ri)->Data4[5], ((REFCLSID)ri)->Data4[6],
        ((REFCLSID)ri)->Data4[7]);
}

#ifdef _WIN64
#pragma comment(linker, "/export:DllRegisterServer=_DllRegisterServer")
#else
#pragma comment(linker, "/export:DllRegisterServer=__DllRegisterServer@0")
#endif

HRESULT WINAPI _DllRegisterServer()
{
    DWORD dwLastError = ERROR_SUCCESS;
    HKEY hKey = NULL;
    DWORD dwSize = 0;
    wchar_t wszFilename[MAX_PATH];
    wchar_t wszInstallPath[MAX_PATH];

    if (!dwLastError)
    {
        if (!GetModuleFileNameW(videoModule, wszFilename, MAX_PATH))
        {
            dwLastError = GetLastError();
        }
    }
    if (!dwLastError)
    {
        TCHAR videoProcessingKey[1000];
        TCHAR clsidText[500];
        stringFromCLSID(clsidText, (REFCLSID)(&CLSID_IVideoProcessing));
        wsprintf(videoProcessingKey, L"SOFTWARE\\Classes\\CLSID\\%s\\InProcServer32", clsidText);

        dwLastError = RegCreateKeyExW(
            HKEY_LOCAL_MACHINE,
            videoProcessingKey,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_WRITE | KEY_WOW64_64KEY,
            NULL,
            &hKey,
            NULL
        );
        if (hKey == NULL || hKey == INVALID_HANDLE_VALUE)
        {
            hKey = NULL;
        }
        if (hKey)
        {
            dwLastError = RegSetValueExW(
                hKey,
                NULL,
                0,
                REG_SZ,
                wszFilename,
                (wcslen(wszFilename) + 1) * sizeof(wchar_t)
            );
            dwLastError = RegSetValueExW(
                hKey,
                L"ThreadingModel",
                0,
                REG_SZ,
                L"Apartment",
                10 * sizeof(wchar_t)
            );
            RegCloseKey(hKey);
        }
    }

    return dwLastError == 0 ? (NOERROR) : (HRESULT_FROM_WIN32(dwLastError));
}

#ifdef _WIN64
#pragma comment(linker, "/export:DllUnregisterServer=_DllUnregisterServer")
#else
#pragma comment(linker, "/export:DllUnregisterServer=__DllUnregisterServer@0")
#endif
HRESULT WINAPI _DllUnregisterServer()
{
    DWORD dwLastError = ERROR_SUCCESS;
    HKEY hKey = NULL;
    DWORD dwSize = 0;
    wchar_t wszFilename[MAX_PATH];

    if (!dwLastError)
    {
        if (!GetModuleFileNameW(videoModule, wszFilename, MAX_PATH))
        {
            dwLastError = GetLastError();
        }
    }
    if (!dwLastError)
    {
        TCHAR videoProcessingKey[1000];
        TCHAR clsidText[500];
        stringFromCLSID(clsidText, (REFCLSID)(&CLSID_IVideoProcessing));
        wsprintf(videoProcessingKey, L"SOFTWARE\\Classes\\CLSID\\%s", clsidText);

        dwLastError = RegOpenKeyW(
            HKEY_LOCAL_MACHINE,
            videoProcessingKey,
            &hKey
        );
        if (hKey == NULL || hKey == INVALID_HANDLE_VALUE)
        {
            hKey = NULL;
        }
        if (hKey)
        {
            dwLastError = RegDeleteTreeW(
                hKey,
                0
            );
            RegCloseKey(hKey);
            if (!dwLastError)
            {
                RegDeleteKeyW(
                    HKEY_LOCAL_MACHINE,
                    videoProcessingKey
                );
            }
        }
    }

    return dwLastError == 0 ? (NOERROR) : (HRESULT_FROM_WIN32(dwLastError));
}

#ifdef _WIN64
#pragma comment(linker, "/export:DllGetClassObject=_DllGetClassObject")
#else
#pragma comment(linker, "/export:DllGetClassObject=__DllGetClassObject@0")
#endif
HRESULT PASCAL _DllGetClassObject(REFCLSID objGuid, REFIID factoryGuid, void** factoryHandle)
{
	register HRESULT hr;

	if (IsEqualCLSID(objGuid, &CLSID_IVideoProcessing))
	{
         hr = factoryvideoprocessing_QueryInterface(&videoFactoryObj, factoryGuid, factoryHandle);
	}
	else
	{
		*factoryHandle = 0;
		hr = CLASS_E_CLASSNOTAVAILABLE;
	}

	return S_OK;
}

#ifdef _WIN64
#pragma comment(linker, "/export:DllCanUnloadNow=_DllCanUnloadNow")
#else
#pragma comment(linker, "/export:DllCanUnloadNow=__DllCanUnloadNow@0")
#endif
HRESULT PASCAL _DllCanUnloadNow(void)
{
	return((OutstandingObjects | LockCount) ? S_FALSE : S_OK);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        // Clear static counts
        OutstandingObjects = LockCount = 0;

        // Initialize my IClassFactory with the pointer to its VTable
        videoFactoryObj.lpVtbl = (IClassFactoryVtbl*)&IClassFactory_Vtbl;
        videoModule = instance;

        // We don't need to do any thread initialization
        DisableThreadLibraryCalls(instance);
    }break;
    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }
	return(1);
}