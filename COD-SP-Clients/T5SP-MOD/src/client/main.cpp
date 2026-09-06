#include <windows.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include "overlay.h"

// Forward DirectX 9 exports directly to the system d3d9.dll
#pragma comment(linker, "/export:Direct3DCreate9=C:\\Windows\\System32\\d3d9.Direct3DCreate9,@1")
#pragma comment(linker, "/export:Direct3DCreate9Ex=C:\\Windows\\System32\\d3d9.Direct3DCreate9Ex,@2")
#pragma comment(linker, "/export:Direct3DShaderValidatorCreate9=C:\\Windows\\System32\\d3d9.Direct3DShaderValidatorCreate9,@3")
#pragma comment(linker, "/export:PSGPError=C:\\Windows\\System32\\d3d9.PSGPError,@4")
#pragma comment(linker, "/export:PSGPSampleTexture=C:\\Windows\\System32\\d3d9.PSGPSampleTexture,@5")
#pragma comment(linker, "/export:D3DPERF_BeginEvent=C:\\Windows\\System32\\d3d9.D3DPERF_BeginEvent,@6")
#pragma comment(linker, "/export:D3DPERF_EndEvent=C:\\Windows\\System32\\d3d9.D3DPERF_EndEvent,@7")
#pragma comment(linker, "/export:D3DPERF_GetStatus=C:\\Windows\\System32\\d3d9.D3DPERF_GetStatus,@8")
#pragma comment(linker, "/export:D3DPERF_QueryRepeatFrame=C:\\Windows\\System32\\d3d9.D3DPERF_QueryRepeatFrame,@9")
#pragma comment(linker, "/export:D3DPERF_SetMarker=C:\\Windows\\System32\\d3d9.D3DPERF_SetMarker,@10")
#pragma comment(linker, "/export:D3DPERF_SetOptions=C:\\Windows\\System32\\d3d9.D3DPERF_SetOptions,@11")
#pragma comment(linker, "/export:D3DPERF_SetRegion=C:\\Windows\\System32\\d3d9.D3DPERF_SetRegion,@12")
#pragma comment(linker, "/export:DebugSetLevel=C:\\Windows\\System32\\d3d9.DebugSetLevel,@13")
#pragma comment(linker, "/export:DebugSetMute=C:\\Windows\\System32\\d3d9.DebugSetMute,@14")

// Tell the compiler that InstallPatches lives in another file (patches.cpp) for you guys that dont know
extern void InstallPatches();


static void LogToFile(const char* msg)
{
    std::ofstream f("t5sp_mod_log.txt", std::ios::app); // might change name of t5sp_mod_log.txt 
    if (f) {
        f << msg << "\n";
    }
    // Also visible in DebugView (Sysinternals) even if the file write fails.
    OutputDebugStringA(msg);
}

// thread
DWORD WINAPI MainThread(LPVOID lpParam)
{
    LogToFile("[*] MainThread started.");


    Sleep(500);

    //  Allocate a console window for the host process
    if (AllocConsole()) {
        FILE* fpDummy;
        freopen_s(&fpDummy, "CONOUT$", "w", stdout);
        freopen_s(&fpDummy, "CONOUT$", "w", stderr);
        freopen_s(&fpDummy, "CONIN$", "r", stdin);

        // Set the window title
        SetConsoleTitleA("T5SP-Mod V1");

        // Print your verification message
        std::cout << "[*] Proxy DLL attached." << std::endl;
        std::cout << "[*] System D3D9 exports forwarded to System32." << std::endl;

        LogToFile("[*] Console allocated successfully.");
    }
    else {
        // error >:)
        char buf[128];
        sprintf_s(buf, "[!] AllocConsole failed, GetLastError=%lu", GetLastError());
        LogToFile(buf);
    }

    // logs to t5sp_mod_log.txt
    LogToFile("[*] Calling InstallPatches()...");
    InstallPatches();
    LogToFile("[*] InstallPatches() returned (hook install did not crash).");

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        LogToFile("[*] DllMain: DLL_PROCESS_ATTACH.");
        CreateThread(NULL, 0, MainThread, NULL, 0, NULL);
        break;
        // calls the shit (Overlay doesnt work needs fixed)
    case DLL_PROCESS_DETACH:
        LogToFile("[*] DllMain: DLL_PROCESS_DETACH.");
        //LogToFile("[*] Calling InitOverlay()...");
        //InitOverlay();
        //LogToFile("[*] InitOverlay() returned.");
        FreeConsole();
        break;
    }
    return TRUE;
}
