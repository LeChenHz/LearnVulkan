#include "Present_hook.h"
#include "Utils.h"

std::once_flag flag;
HANDLE mNamedPipeline = INVALID_HANDLE_VALUE;

bool useDxgi1_2 = false;
CreateFactory TrueCreateFactory;
CreateSwapChainForHwnd TrueCreateSwapChainForHwnd;
CreateSwapChain TrueCreateSwapChain;
Present TruePresent;
Present1 TruePresent1;

GLSwapBuffers TrueSwapBuffers;

GetInstanceProcAddr TrueGetInstanceProcAddr;
GetDeviceProcAddr TrueGetDeviceProcAddr;
QueuePresent TrueQueuePresent;


void InitNamedPipe()
{
    std::thread _t([&]
        {
            DWORD pid = GetCurrentProcessId();
            std::string pipelineName = "\\\\.\\pipe\\FrameTime" + std::to_string(pid);
            HANDLE mThread = NULL;
            DWORD mThreadId = 0;
            while (1) {
                mNamedPipeline = CreateNamedPipeA(pipelineName.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                    PIPE_UNLIMITED_INSTANCES, BUFSIZE, BUFSIZE, 0, NULL);

                if (mNamedPipeline == INVALID_HANDLE_VALUE)
                {
                    printf("Create pipe failed");
                    return;
                }

                auto connected = ConnectNamedPipe(mNamedPipeline, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);;
                if (connected)
                {
                    mThread = CreateThread(
                        NULL,
                        0,
                        InstanceThread,
                        (LPVOID)mNamedPipeline,
                        0,
                        &mThreadId);

                    if (mThread == NULL)
                    {
                        printf(("CreateThread failed, err=%d.\n"), GetLastError());
                        return;
                    }
                    else
                    {
                        CloseHandle(mThread);
                    }
                }
                else
                {
                    CloseHandle(mNamedPipeline);
                }

            }
        }
    );
    _t.detach();
}

DWORD WINAPI InstanceThread(LPVOID lpvParam)
{
    HANDLE mNamedPipeline = (HANDLE)lpvParam;
    while (1)
    {
        BOOL success;
        BYTE tmpBuffer[BUFSIZE];
        DWORD bytesRead;
        success = ReadFile(mNamedPipeline, tmpBuffer, BUFSIZE, &bytesRead, NULL);
        //MessageBoxA(NULL, "Debug read dll", "Info", 0);
        if (!success || bytesRead == 0)
        {
            printf("ReadFile failed");
            break;
        }

        int res = OnStateChanged(tmpBuffer);
        DWORD bytesWritten;

        if (res == 0)
        {
            char reply[] = "success";
            success = WriteFile(mNamedPipeline, reply, sizeof(reply), &bytesWritten, NULL);
        }
        else
        {
            char reply = char(res);
            success = WriteFile(mNamedPipeline, &reply, sizeof(reply), &bytesWritten, NULL);
        }
        if (!success || bytesWritten == 0)
        {
            printf("WriteFile failed");
            break;
        }

        FlushFileBuffers(mNamedPipeline);
        DisconnectNamedPipe(mNamedPipeline);
        CloseHandle(mNamedPipeline);
    }
    return 1;
}


HRESULT MyPresent1(IDXGISwapChain1* pSwapChain1, UINT SyncInterval, UINT Flags, _In_ const DXGI_PRESENT_PARAMETERS* pPresentParameters)
{
    std::call_once(flag, InitNamedPipe);
    AddFrameTime(GetFrameTime());
    return TruePresent1(pSwapChain1, SyncInterval, Flags, pPresentParameters);
}

HRESULT MyPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    std::call_once(flag, InitNamedPipe);
    AddFrameTime(GetFrameTime());
    return TruePresent(pSwapChain, SyncInterval, Flags);
}


// gl
BOOL MySwapBuffers(HDC hdc)
{
    std::call_once(flag, InitNamedPipe);
    AddFrameTime(GetFrameTime());
    return TrueSwapBuffers(hdc);
}

// vk
VkResult MyQueuePresent(VkQueue queue, const VkPresentInfoKHR* presentInfo)
{
    std::call_once(flag, InitNamedPipe);
    AddFrameTime(GetFrameTime());
    return TrueQueuePresent(queue, presentInfo);
}

HRESULT MyCreateSwapChainForHwnd(IDXGIFactory2* pfactory2, _In_ IUnknown* pDevice, _In_ HWND hWnd, _In_ const DXGI_SWAP_CHAIN_DESC1* pDesc,
    _In_opt_ const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, _In_opt_ IDXGIOutput* pRestrictToOutput, _COM_Outptr_ IDXGISwapChain1** ppSwapChain)
{
    HRESULT res = TrueCreateSwapChainForHwnd(pfactory2, pDevice, hWnd, pDesc, pFullscreenDesc, pRestrictToOutput, ppSwapChain);
    IDXGISwapChain1* swapChain1 = *ppSwapChain;
    if (swapChain1 == nullptr)
    {
        printf("Get swapChain1 Failed");
    }

    LONG error;
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    void** swapChain1_vtable = *reinterpret_cast<void***>(swapChain1);
    TruePresent1 = Present1(swapChain1_vtable[22]);
    DetourAttach(&(PVOID&)TruePresent1, MyPresent1);
    error = DetourTransactionCommit();

    if (error != NO_ERROR) {
        printf("Error detouring Present1\n");

    }
    return res;
}

HRESULT MyCreateSwapChain(IDXGIFactory* pfactory, _In_ IUnknown* pDevice, _In_ DXGI_SWAP_CHAIN_DESC* pDesc, _COM_Outptr_ IDXGISwapChain** ppSwapChain)
{
    HRESULT res = TrueCreateSwapChain(pfactory, pDevice, pDesc, ppSwapChain);
    IDXGISwapChain* swapChain = *ppSwapChain;
    if (swapChain == nullptr)
    {
        printf("Get swapChain Failed");
    }

    LONG error;
    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    void** swapChain1_vtable = *reinterpret_cast<void***>(swapChain);
    TruePresent1 = Present1(swapChain1_vtable[8]);
    DetourAttach(&(PVOID&)TruePresent, MyPresent);
    error = DetourTransactionCommit();

    if (error != NO_ERROR) {
        printf("Error detouring Present\n");

    }

    return res;
}

bool factorInitialized = false;
HRESULT MyCreateFactory(REFIID riid, _COM_Outptr_ void** ppFactory)
{
    HRESULT res = TrueCreateFactory(riid, ppFactory);

    IDXGIFactory* factory = reinterpret_cast<IDXGIFactory*>(*ppFactory);
    if (factory == nullptr)
    {
        printf("Get Factory Failed");
    }

    IDXGIFactory2* factory2 = CastComObject<IDXGIFactory2>(factory);
    LONG error;
    if (factory2 != nullptr)
    {
        useDxgi1_2 = true;
        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        if (factorInitialized)
        {
            DetourDetach(&(PVOID&)TrueCreateSwapChainForHwnd, MyCreateSwapChainForHwnd);
            error = DetourTransactionCommit();
            if (error != NO_ERROR) {
                printf("DetourDetach CreateSwapChainForHwnd() failed.\n");
            }
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
        }
        void** factory2_vtable = *reinterpret_cast<void***>(factory2);
        TrueCreateSwapChainForHwnd = (CreateSwapChainForHwnd)factory2_vtable[15];
        DetourAttach(&(PVOID&)TrueCreateSwapChainForHwnd, MyCreateSwapChainForHwnd);
        error = DetourTransactionCommit();

        if (error != NO_ERROR) {
            printf("error detouring CreateSwapChainForHwnd\n");

        }

        factory2->Release();
        factorInitialized = true;
    }
    else
    {
        DetourRestoreAfterWith();
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        if (factorInitialized)
        {
            DetourDetach(&(PVOID&)TrueCreateSwapChain, MyCreateSwapChain);
            error = DetourTransactionCommit();
            if (error != NO_ERROR) {
                printf("DetourDetach CreateSwapChain() failed.\n");
            }
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
        }
        void** factory_vtable = *reinterpret_cast<void***>(factory);
        TrueCreateSwapChain = (CreateSwapChain)factory_vtable[10];
        DetourAttach(&(PVOID&)TrueCreateSwapChain, MyCreateSwapChain);
        error = DetourTransactionCommit();

        if (error != NO_ERROR) {
            printf("error detouring CreateSwapChain\n");

        }
        factorInitialized = true;
    }
    return res;
}

PFN_vkVoidFunction MyGetInstanceProcAddr(VkInstance instance, const char* pName)
{
    if (strstr(pName, "vkGetDeviceProcAddr"))
    {
        //MessageBoxA(NULL, "get vkGetDeviceProcAddr!", "Info", 0);
        TrueGetDeviceProcAddr = (GetDeviceProcAddr)TrueGetInstanceProcAddr(instance, pName);

        LONG error;
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)TrueGetDeviceProcAddr, MyGetDeviceProcAddr);
        error = DetourTransactionCommit();
        if (error != NO_ERROR) {
            printf("error detouring GetDeviceProcAddr\n");
        }
    }
    return TrueGetInstanceProcAddr(instance, pName);
}
PFN_vkVoidFunction MyGetDeviceProcAddr(VkDevice device, const char* pName)
{
    if (strstr(pName, "vkQueuePresentKHR"))
    {
        //MessageBoxA(NULL, "get vkQueuePresentKHR!", "Info", 0);
        TrueQueuePresent = (QueuePresent)TrueGetDeviceProcAddr(device, pName);

        LONG error;
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&(PVOID&)TrueQueuePresent, MyQueuePresent);
        error = DetourTransactionCommit();
        if (error != NO_ERROR) {
            printf("error detouring vkQueuePresentKHR\n");
        }
    }
    return TrueGetDeviceProcAddr(device, pName);
}

void HookPresent()
{
    LONG error;

    DetourRestoreAfterWith();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    TrueCreateFactory = (CreateFactory)DetourFindFunction("dxgi.dll", "CreateDXGIFactory");
    TrueSwapBuffers = (GLSwapBuffers)DetourFindFunction("gdi32.dll", "SwapBuffers");
    TrueGetInstanceProcAddr = (GetInstanceProcAddr)DetourFindFunction("vulkan-1.dll", "vkGetInstanceProcAddr");

    if (TrueCreateFactory)
    {
        DetourAttach(&(PVOID&)TrueCreateFactory, MyCreateFactory);
    }
    if (TrueSwapBuffers)
    {
        DetourAttach(&(PVOID&)TrueSwapBuffers, MySwapBuffers);
    }
    if (TrueGetInstanceProcAddr)
    {
        DetourAttach(&(PVOID&)TrueGetInstanceProcAddr, MyGetInstanceProcAddr);
    }
    error = DetourTransactionCommit();

    if (error == NO_ERROR) {
        printf("hook" DETOURS_STRINGIFY(DETOURS_BITS) ".dll: "
            " Detoured CreateFactory().\n");

    }
    else {
        printf("hook" DETOURS_STRINGIFY(DETOURS_BITS) ".dll: "
            " Error detouring CreateFactory(): %ld\n", error);
    }

}

void UnHook()
{
    LONG error;
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    if (TrueCreateFactory != NULL)
    {
        if (useDxgi1_2)
        {
            DetourDetach(&(PVOID&)TruePresent1, MyPresent1);
            DetourDetach(&(PVOID&)TrueCreateSwapChainForHwnd, MyCreateSwapChainForHwnd);
        }
        else
        {
            DetourDetach(&(PVOID&)TruePresent, MyPresent);
            DetourDetach(&(PVOID&)TrueCreateSwapChain, MyCreateSwapChain);
        }
        DetourDetach(&(PVOID&)TrueCreateFactory, MyCreateFactory);
    }

    if (TrueSwapBuffers != NULL)
    {
        DetourDetach(&(PVOID&)TrueSwapBuffers, MySwapBuffers);
    }

    if (TrueGetInstanceProcAddr != NULL)
    {
        DetourDetach(&(PVOID&)TrueQueuePresent, MyQueuePresent);
        DetourDetach(&(PVOID&)TrueGetDeviceProcAddr, MyGetDeviceProcAddr);
        DetourDetach(&(PVOID&)TrueGetInstanceProcAddr, MyGetInstanceProcAddr);
    }
    error = DetourTransactionCommit();
    printf("hook" DETOURS_STRINGIFY(DETOURS_BITS) ".dll: "
        " Removed hooks: %ld\n", error);

    CloseHandle(mNamedPipeline);

}