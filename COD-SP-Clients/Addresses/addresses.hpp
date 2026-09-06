#pragma once
#include <windows.h>
#include <cstdint>

namespace addresses
{
    inline std::uintptr_t base = 0;

    inline void Init()
    {
        base = reinterpret_cast<std::uintptr_t>(GetModuleHandleA(nullptr));
    }


    constexpr std::uintptr_t PREFERRED_BASE = 0x00400000;

    constexpr std::uintptr_t T5_Thread_Timer_off       = 0x004C06E0 - PREFERRED_BASE;
    constexpr std::uintptr_t Scr_LoadScript_off         = 0x00661AF0 - PREFERRED_BASE;
    constexpr std::uintptr_t Scr_GetFunctionHandle_off  = 0x004E3470 - PREFERRED_BASE;
    constexpr std::uintptr_t Scr_ExecThread_off         = 0x005598E0 - PREFERRED_BASE;
    constexpr std::uintptr_t Scr_FreeThread_off         = 0x005DE2C0 - PREFERRED_BASE;
    constexpr std::uintptr_t Scr_LoadGameType_off       = 0x004B7F80 - PREFERRED_BASE;
    constexpr std::uintptr_t DB_LinkXAssetEntry_off     = 0x007A2F10 - PREFERRED_BASE;
    constexpr std::uintptr_t Dvar_FindVar_off           = 0x0057FF80 - PREFERRED_BASE;

    constexpr std::uintptr_t monkeyToy_off              = 0x004A11C9 - PREFERRED_BASE;
    constexpr std::uintptr_t con_matchPrefixOnly_off     = 0x004A11DC - PREFERRED_BASE;
    

    inline std::uintptr_t T5_Thread_Timer_g()       { return base + T5_Thread_Timer_off; }
    inline std::uintptr_t Scr_LoadScript_g()        { return base + Scr_LoadScript_off; }
    inline std::uintptr_t Scr_GetFunctionHandle_g() { return base + Scr_GetFunctionHandle_off; }
    inline std::uintptr_t Scr_ExecThread_g()        { return base + Scr_ExecThread_off; }
    inline std::uintptr_t Scr_FreeThread_g()        { return base + Scr_FreeThread_off; }
    inline std::uintptr_t Scr_LoadGameType_g()      { return base + Scr_LoadGameType_off; }
    inline std::uintptr_t DB_LinkXAssetEntry_g()    { return base + DB_LinkXAssetEntry_off; }
    inline std::uintptr_t Dvar_FindVar_g()          { return base + Dvar_FindVar_off; }
    inline std::uintptr_t monkeyToy_g()              { return base + monkeyToy_off; }
    inline std::uintptr_t con_matchPrefixOnly_g()     { return base + con_matchPrefixOnly_off; }
}
