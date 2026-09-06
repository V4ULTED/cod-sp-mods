#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <deque>
#include <cstring>

// ===== miniz must be included like this =====
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_ARCHIVE_WRITING_APIS
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#define MINIZ_IMPLEMENTATION
#include "Project-utils/utils/utils/miniz.h"
#include "Project-utils/utils/utils/miniz.c"
// ===========================================

#include "Project-utils/utils/utils/hooks.h"
#include "Addresses/addresses.hpp"


// Engine structures
struct RawFile {
    const char* name;
    int         len;
    const char* buffer;
};

enum XAssetType {
    ASSET_TYPE_RAWFILE = 0x24
};

union XAssetHeader {
    RawFile* rawFile;
    void*    data;
};

struct XAsset {
    XAssetType   type;
    XAssetHeader header;
};

struct XAssetEntry {
    XAsset asset;
};

// Function types
using Scr_LoadScript_t        = int(__cdecl*)(int instance, const char* name);
using Scr_GetFunctionHandle_t = int(__cdecl*)(int instance, const char* script, const char* func);
using Scr_ExecThread_t        = uint16_t(__cdecl*)(int instance, int handle, int paramCount);
using Scr_FreeThread_t        = void(__cdecl*)(uint16_t handle, int instance);
using DB_LinkXAssetEntry_t    = XAssetEntry*(__cdecl*)(XAssetEntry* entry, int allowOverride);


// Config

static const char* SCRIPT_ROOT = "T5SP-MOD\\Scripts";
static bool        LOAD_ZM     = true;
static bool        LOAD_SP     = true;


// Globals
struct LoadedScript {
    std::string name;
    std::string path;
    int         mainHandle = 0;
};

static std::vector<LoadedScript> g_LoadedScripts;


struct LinkedAsset {
    std::string       name;
    std::vector<char> compressed;
    RawFile           rf{};
};
static std::deque<LinkedAsset> g_LinkedAssets;

utils::hook::detour LoadGameTypeDetour;
utils::hook::detour LinkAssetDetour;


// Helpers
bool ReadFileBinary(const std::string& path, std::vector<char>& out)
{
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    auto size = f.tellg();
    f.seekg(0);
    out.resize(static_cast<size_t>(size));
    return static_cast<bool>(f.read(out.data(), size));
}

bool CompressGsc(const std::vector<char>& source, std::vector<char>& out)
{
    if (source.empty()) return false;

    std::vector<char> src = source;
    src.push_back('\0');

    mz_ulong bound = mz_compressBound(static_cast<mz_ulong>(src.size()));
    std::vector<unsigned char> compressed(bound);

    mz_ulong compressedSize = bound;
    if (mz_compress2(compressed.data(), &compressedSize,
                     reinterpret_cast<const unsigned char*>(src.data()),
                     static_cast<mz_ulong>(src.size()),
                     MZ_DEFAULT_COMPRESSION) != MZ_OK)
    {
        return false;
    }

    out.resize(8 + compressedSize);
    uint32_t* header = reinterpret_cast<uint32_t*>(out.data());
    header[0] = static_cast<uint32_t>(compressedSize);
    header[1] = static_cast<uint32_t>(src.size());
    memcpy(out.data() + 8, compressed.data(), compressedSize);

    return true;
}

bool CreateAndLinkRawFile(const std::string& assetName, const std::vector<char>& source)
{
    // Each call gets its own slot in g_LinkedAssets so earlier assets'
    // name/buffer pointers are never clobbered by a later call.
    g_LinkedAssets.emplace_back();
    LinkedAsset& asset = g_LinkedAssets.back();

    if (!CompressGsc(source, asset.compressed)) {
        std::cerr << "[GSC] Compression failed for " << assetName << "\n";
        g_LinkedAssets.pop_back();
        return false;
    }

    asset.name = assetName;
    asset.rf.name   = asset.name.c_str();
    asset.rf.len    = static_cast<int>(asset.compressed.size());
    asset.rf.buffer = asset.compressed.data();

    XAssetEntry entry{};
    entry.asset.type           = ASSET_TYPE_RAWFILE;
    entry.asset.header.rawFile = &asset.rf;

    auto DB_LinkXAssetEntry = reinterpret_cast<DB_LinkXAssetEntry_t>(addresses::DB_LinkXAssetEntry_g());
    if (!DB_LinkXAssetEntry) {
        g_LinkedAssets.pop_back();
        return false;
    }

    DB_LinkXAssetEntry(&entry, 0);
    return true;
}


// Load every .gsc in a folder
void LoadScriptsFromFolder(const std::string& folder, const std::string& prefix)
{
    std::string search = folder + "\\*.gsc";
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search.c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) {
        std::cerr << "[GSC] Could not find folder: " << folder
                  << " (GetLastError=" << GetLastError() << ")\n";
        return;
    }

    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        std::string filename = fd.cFileName;
        std::string fullPath = folder + "\\" + filename;

        std::string rawAssetName = prefix + "/" + filename;
        std::string scriptName   = prefix + "/" + filename.substr(0, filename.size() - 4);

        std::cout << "[GSC] Loading " << fullPath << " as " << rawAssetName << "\n";

        std::vector<char> source;
        if (!ReadFileBinary(fullPath, source)) {
            std::cerr << "[GSC] Failed to read " << fullPath << "\n";
            continue;
        }

        if (!CreateAndLinkRawFile(rawAssetName, source)) {
            std::cerr << "[GSC] Failed to link " << rawAssetName << "\n";
            continue;
        }

        auto Scr_LoadScript = reinterpret_cast<Scr_LoadScript_t>(addresses::Scr_LoadScript_g());
        int result = Scr_LoadScript(0, scriptName.c_str());
        if (!result) {
            std::cerr << "[GSC] Scr_LoadScript failed for " << scriptName << "\n";
            continue;
        }

        auto Scr_GetFunctionHandle = reinterpret_cast<Scr_GetFunctionHandle_t>(addresses::Scr_GetFunctionHandle_g());
        int handle = Scr_GetFunctionHandle(0, scriptName.c_str(), "main");

        LoadedScript ls;
        ls.name       = scriptName;
        ls.path       = fullPath;
        ls.mainHandle = handle;
        g_LoadedScripts.push_back(ls);

        std::cout << "[GSC] Successfully loaded " << scriptName
                  << " (main handle = " << handle << ")\n";

    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);
}

// Main entry points
void LoadAllCustomGsc()
{
    g_LoadedScripts.clear();
    g_LinkedAssets.clear();

    if (LOAD_ZM)
        LoadScriptsFromFolder(std::string(SCRIPT_ROOT) + "\\ZM", "scripts/zm"); // Zombie scripts 

    if (LOAD_SP)
        LoadScriptsFromFolder(std::string(SCRIPT_ROOT) + "\\SP", "scripts/sp"); // Sp scripts
}

void ExecuteAllMains()
{
    auto Scr_ExecThread = reinterpret_cast<Scr_ExecThread_t>(addresses::Scr_ExecThread_g());
    auto Scr_FreeThread = reinterpret_cast<Scr_FreeThread_t>(addresses::Scr_FreeThread_g());

    if (!Scr_ExecThread) return;

    for (auto& script : g_LoadedScripts)
    {
        if (!script.mainHandle) continue;

        std::cout << "[GSC] Executing main() of " << script.name << "\n";
        uint16_t thread = Scr_ExecThread(0, script.mainHandle, 0);
        if (Scr_FreeThread)
            Scr_FreeThread(thread, 0);
    }
}

// dump stuff will change later
XAssetEntry* __cdecl Hooked_DB_LinkXAssetEntry(XAssetEntry* entry, int allowOverride)
{
    if (entry && entry->asset.type == ASSET_TYPE_RAWFILE && entry->asset.header.rawFile)
    {
        RawFile* rf = entry->asset.header.rawFile;
        std::string name = rf->name ? rf->name : "unknown";

        if (name.find(".gsc") != std::string::npos)
        {
            std::string safeName = name;
            for (auto& c : safeName) {
                if (c == '/' || c == '\\') c = '_';
            }

            std::ofstream f("dump_" + safeName + ".bin", std::ios::binary);
            if (f && rf->buffer && rf->len > 0) {
                f.write(rf->buffer, rf->len);
            }

            std::cout << "[DUMP] " << name << " len=" << rf->len << "\n";
        }
    }

    auto original = *LinkAssetDetour.get<DB_LinkXAssetEntry_t>();
    return original(entry, allowOverride);
}


// Hook
void __cdecl Hooked_Scr_LoadGameType()
{
    std::cout << "[GSC] Hooked_Scr_LoadGameType fired\n"; // this works but we need to find a away to hook scr_loadscript without crashing

    LoadGameTypeDetour.invoke<void>();

    LoadAllCustomGsc();
    ExecuteAllMains();
}


// Folder setup
void EnsureScriptFolders()
{
    CreateDirectoryA("T5SP-MOD", nullptr);
    CreateDirectoryA("T5SP-MOD\\Scripts", nullptr);
    CreateDirectoryA("T5SP-MOD\\Scripts\\ZM", nullptr);
    CreateDirectoryA("T5SP-MOD\\Scripts\\SP", nullptr);
    // creates folders
}


// Init
void InstallPatches()
{
    EnsureScriptFolders();
    addresses::Init();

    std::cout << "[*] Module base = 0x" << std::hex << addresses::base << std::dec << "\n";
    std::cout << "[*] Installing T5 GSC loader...\n";

    // Anti-debug
    utils::hook::set<std::uint8_t>(addresses::T5_Thread_Timer_g(), 0xC3);

    // Console unlocks
    utils::hook::set<std::uint8_t>(addresses::monkeyToy_g, 0x00); // monkeytoy for future use doesnt work
    utils::hook::set<std::uint8_t>(addresses::con_matchPrefixOnly_g(), 0x00); // con_matchPrefixOnly For future use doesnt work

    // Hook injection point
    LoadGameTypeDetour.create(addresses::Scr_LoadGameType_g(), Hooked_Scr_LoadGameType);
    LoadGameTypeDetour.enable();

    // TEMP DIAGNOSTIC hook — dumps real engine rawfile assets to disk
    LinkAssetDetour.create(addresses::DB_LinkXAssetEntry_g(), Hooked_DB_LinkXAssetEntry);
    LinkAssetDetour.enable();

    std::cout << "[*] GSC loader ready\n";
}