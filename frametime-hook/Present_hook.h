#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "detours.h"
#include <dxgi.h>
#include <dxgi1_2.h>
#include <thread>
#include <chrono>
#include "vulkan/vulkan_core.h"

#pragma comment(lib, "detours.lib")
// Global 
void InitNamedPipe();
DWORD WINAPI InstanceThread(LPVOID lpvParam);
void HookPresent();
void UnHook();

// Dx
typedef HRESULT(__stdcall* CreateFactory)(REFIID, _COM_Outptr_ void**);
typedef HRESULT(__stdcall* CreateSwapChainForHwnd)(IDXGIFactory2*, _In_ IUnknown*, _In_ HWND, _In_ const DXGI_SWAP_CHAIN_DESC1*,
    _In_opt_ const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, _In_opt_ IDXGIOutput*, _COM_Outptr_ IDXGISwapChain1**);
typedef HRESULT(__stdcall* CreateSwapChain)(IDXGIFactory*, _In_ IUnknown*, _In_ DXGI_SWAP_CHAIN_DESC*, _COM_Outptr_ IDXGISwapChain**);
typedef HRESULT(__stdcall* Present)(IDXGISwapChain*, UINT, UINT);
typedef HRESULT(__stdcall* Present1)(IDXGISwapChain1*, UINT, UINT, _In_ const DXGI_PRESENT_PARAMETERS*);

HRESULT MyCreateFactory(REFIID riid, _COM_Outptr_ void** ppFactory);
HRESULT MyCreateSwapChainForHwnd(IDXGIFactory2* pfactory2, _In_ IUnknown* pDevice, _In_ HWND hWnd, _In_ const DXGI_SWAP_CHAIN_DESC1* pDesc,
    _In_opt_ const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, _In_opt_ IDXGIOutput* pRestrictToOutput, _COM_Outptr_ IDXGISwapChain1** ppSwapChain);
HRESULT MyCreateSwapChain(IDXGIFactory* pfactory, _In_ IUnknown* pDevice, _In_ DXGI_SWAP_CHAIN_DESC* pDesc, _COM_Outptr_ IDXGISwapChain** ppSwapChain);
HRESULT MyPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
HRESULT MyPresent1(IDXGISwapChain1* pSwapChain1, UINT SyncInterval, UINT Flags, _In_ const DXGI_PRESENT_PARAMETERS* pPresentParameters);


//GL
typedef BOOL(__stdcall* GLSwapBuffers)(HDC);

BOOL MySwapBuffers(HDC hdc);

//Vk
typedef VkResult(VKAPI_PTR* QueuePresent)(VkQueue, const VkPresentInfoKHR*);
typedef PFN_vkVoidFunction(VKAPI_CALL* GetInstanceProcAddr)(VkInstance, const char*);
typedef PFN_vkVoidFunction(VKAPI_CALL* GetDeviceProcAddr)(VkDevice, const char*);

VkResult MyQueuePresent(VkQueue queue, const VkPresentInfoKHR* presentInfo);
PFN_vkVoidFunction MyGetInstanceProcAddr(VkInstance instance, const char* pName);
PFN_vkVoidFunction MyGetDeviceProcAddr(VkDevice device, const char* pName);