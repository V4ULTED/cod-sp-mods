#include <windows.h>
#include <d3d9.h>
#include <iostream>

#include "Project-utils/utils/utils/hooks.h"
#include "overlay.h"


static constexpr int IDIRECT3D9_CREATEDEVICE_INDEX   = 16;
static constexpr int IDIRECT3DDEVICE9_ENDSCENE_INDEX  = 42;

using Direct3DCreate9_t = IDirect3D9 * (WINAPI*)(UINT);
using CreateDevice_t = HRESULT(WINAPI*)(
    IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD,
    D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
using EndScene_t = HRESULT(WINAPI*)(IDirect3DDevice9*);

static utils::hook::detour g_CreateDeviceDetour;
static utils::hook::detour g_EndSceneDetour;
static bool g_EndSceneHooked = false;


static void DrawWatermark(IDirect3DDevice9* device)
{
    IDirect3DSurface9* backBuffer = nullptr;
    if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)) || !backBuffer)
        return;

    HDC hdc = nullptr;
    if (SUCCEEDED(backBuffer->GetDC(&hdc)) && hdc)
    {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(160, 160, 160)); // gray

        HFONT font = CreateFontA(
            16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");
        HFONT oldFont = font ? static_cast<HFONT>(SelectObject(hdc, font)) : nullptr;

        TextOutA(hdc, 8, 8, "T5SP-MOD V1", 11);

        if (font)
        {
            SelectObject(hdc, oldFont);
            DeleteObject(font);
        }

        backBuffer->ReleaseDC(hdc);
    }

    backBuffer->Release();
}


static HRESULT WINAPI Hooked_EndScene(IDirect3DDevice9* device)
{
    DrawWatermark(device);

    auto original = g_EndSceneDetour.get<EndScene_t>();
    return (*original)(device);
}


static HRESULT WINAPI Hooked_CreateDevice(
    IDirect3D9* self, UINT adapter, D3DDEVTYPE deviceType, HWND hFocusWindow,
    DWORD behaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters,
    IDirect3DDevice9** ppReturnedDeviceInterface)
{
    auto original = g_CreateDeviceDetour.get<CreateDevice_t>();
    HRESULT hr = (*original)(self, adapter, deviceType, hFocusWindow,
                             behaviorFlags, pPresentationParameters,
                             ppReturnedDeviceInterface);

    if (SUCCEEDED(hr) && !g_EndSceneHooked && ppReturnedDeviceInterface && *ppReturnedDeviceInterface)
    {
        void** vtable = *reinterpret_cast<void***>(*ppReturnedDeviceInterface);
        void* endSceneAddr = vtable[IDIRECT3DDEVICE9_ENDSCENE_INDEX];

        g_EndSceneDetour.create(endSceneAddr, reinterpret_cast<void*>(Hooked_EndScene));
        g_EndSceneDetour.enable();
        g_EndSceneHooked = true;

        std::cout << "[OVERLAY] EndScene hooked, watermark active\n";
    }

    return hr;
}

// entry point
void InitOverlay()
{
    HMODULE realD3D9 = LoadLibraryA("C:\\Windows\\System32\\d3d9.dll");
    if (!realD3D9)
    {
        std::cerr << "[OVERLAY] Failed to load real d3d9.dll\n";
        return;
    }

    auto Direct3DCreate9Real = reinterpret_cast<Direct3DCreate9_t>(
        GetProcAddress(realD3D9, "Direct3DCreate9"));
    if (!Direct3DCreate9Real)
    {
        std::cerr << "[OVERLAY] Failed to resolve real Direct3DCreate9\n";
        return;
    }


    IDirect3D9* tempD3D = Direct3DCreate9Real(D3D_SDK_VERSION);
    if (!tempD3D)
    {
        std::cerr << "[OVERLAY] Failed to create temp IDirect3D9\n";
        return;
    }

    void** vtable = *reinterpret_cast<void***>(tempD3D);
    void* createDeviceAddr = vtable[IDIRECT3D9_CREATEDEVICE_INDEX];

    g_CreateDeviceDetour.create(createDeviceAddr, reinterpret_cast<void*>(Hooked_CreateDevice));
    g_CreateDeviceDetour.enable();

    tempD3D->Release();

    std::cout << "[OVERLAY] CreateDevice hooked, waiting for real device...\n";
}