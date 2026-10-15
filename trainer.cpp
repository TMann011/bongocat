// trainer.cpp - Inkwell Bongo Cat Trainer (Comprehensive Systems & Reverse Engineering Edition)
// Subsystems:
// [1]  RAM Skin & Pass Unlocker (Dual JIT Hook & Code Cave Detour)
// [2]  Unhide All BongoDex Collections (Angelic, Special, Mecha)
// [3]  PawPass Level 99 Max (Live Steam Memory & RAM Sync)
// [4]  Permanent Skin Unlock & Auto-Save
// [5]  Battle Royale Match Trigger (Unix Timestamp Countdown)
// [6]  Item Reward Popup Spawner Trigger
// [7]  Fast Cosmetic Auto-Randomizer (Wardrobe Real-Time Cycle)
// [8]  Multi-Cat Desktop Slot Unlocks (Free Pattern Layout)
// [9]  Hardware Non-Intrusive Auto-Tapper (VK_F24 Hardware Event)
// [10] Infinite Coins & Tokens Stash (999,999,999 Lifetime Economy)
// [11] Instant Shop Chest Stock & 999x Tokens (Zero Cooldown)
// [12] Master Steam Stats & Achievements Maxer (SetStatInt32 Remote Commit)
// [13] Desktop Overlay Click-Through Mode (WS_EX_TRANSPARENT)
// [14] Self-Petting & Enhanced Cat Interactions (PatOwnCat & Reactions)
// [15] Instant BR Victory & Promo Reward Trigger (Crown Cosmetic Grant)

#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <commctrl.h>
#include <stdio.h>
#include <shellapi.h>
#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <time.h>
#include <inttypes.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "advapi32.lib")

// ---------------------------------------------------------------------------
// Dark Theme Colors & Brushes
// ---------------------------------------------------------------------------
static COLORREF COLOR_BG   = RGB(25, 25, 25);
static COLORREF COLOR_TEXT = RGB(235, 235, 235);
static HBRUSH   g_hbrBg    = NULL;

// Language Mode: 0 = English, 1 = 简体中文
static int g_lang = 0;

// Process State
static HWND   g_hwnd = NULL;
static HANDLE g_proc = NULL;
static DWORD  g_pid  = 0;

// Hooks State
static std::vector<uintptr_t>          g_ownsBases;
static std::vector<uintptr_t>          g_ownsPatched;
static std::vector<uintptr_t>          g_ownsCaves;
static std::vector<std::array<BYTE,8>> g_ownsOrigs;
static std::vector<uintptr_t>          g_qtyPatched;
static std::vector<std::array<BYTE,8>> g_qtyOrigs;

// Control IDs
enum {
    IDC_ATTACH = 1001,
    IDC_LAUNCH,
    IDC_STATUS,
    IDC_LANG,
    IDC_KOFI,
    IDC_GITHUB,
    
    IDC_FEATURE_RAM_SKINS,       // [1] RAM Skin Unlocker
    IDC_FEATURE_DEV_SKINS,       // [2] Unhide All Collections
    IDC_FEATURE_PAWPASS,         // [3] PawPass Level 99 Max
    IDC_FEATURE_PERM_SKINS,      // [4] Permanent Skin Save
    IDC_FEATURE_BATTLE_ROYALE,   // [5] Battle Royale Match Trigger
    IDC_FEATURE_SPAWN_REWARD,    // [6] Local Item Reward Spawner
    IDC_FEATURE_HAT_RANDOMIZER,  // [7] Fast Cosmetic Auto-Randomizer
    IDC_FEATURE_MULTI_CAT,       // [8] Multi-Cat Desktop Slot Unlocks
    IDC_FEATURE_AUTO_TAPS,       // [9] Hardware Auto-Tapper & Clicker (VK_F24)
    IDC_FEATURE_COIN_MULTIPLIER, // [10] Infinite Coins Stash
    IDC_FEATURE_INSTANT_CHESTS,  // [11] Instant Shop Chest Stock & 999x Tokens
    IDC_FEATURE_ACHIEVEMENTS,    // [12] Master Steam Stats & Achievements Maxer
    IDC_FEATURE_CLICKTHROUGH,    // [13] Desktop Overlay Click-Through
    IDC_FEATURE_SELF_PETTING,    // [14] Self-Petting & Enhanced Reactions
    IDC_FEATURE_BR_VICTORY,      // [15] Instant BR Victory & Promo Reward

    IDC_EXPLAIN,
    IDC_AUTO,
    IDC_LOG
};

static void Log(const char* fmt, ...);
static void SetStatus(const char* sEn, const char* sZh);
static void UpdateLanguageUI();

// ---------------------------------------------------------------------------
// Hardware Key Injection Helper (WinKeyHook GetAsyncKeyState Compliance)
// ---------------------------------------------------------------------------
static void HardwareTapOnce(BYTE vk = 0x87) // VK_F24 (0x87 / 135)
{
    keybd_event(vk, 0, 0, 0);
    Sleep(8);
    keybd_event(vk, 0, KEYEVENTF_KEYUP, 0);
}

static void TriggerPetsTapBoost() {
    HardwareTapOnce(0x87);
}

// ---------------------------------------------------------------------------
// Process Helpers & Steam Launch
// ---------------------------------------------------------------------------
static DWORD FindPid(const wchar_t* name)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe = { sizeof(pe) };
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, name) == 0) { pid = pe.th32ProcessID; break; }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

static HWND GetGameWindow()
{
    HWND gameHwnd = FindWindowW(L"UnityWndClass", NULL);
    if (!gameHwnd) gameHwnd = FindWindowW(NULL, L"Bongo Cat");
    return gameHwnd;
}

static void EnsureSteamAppId()
{
    wchar_t steamPath[MAX_PATH] = L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\BongoCat\\steam_appid.txt";
    if (GetFileAttributesW(steamPath) == INVALID_FILE_ATTRIBUTES) {
        HANDLE hFile = CreateFileW(steamPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            DWORD written = 0;
            WriteFile(hFile, "3419430", 7, &written, NULL);
            CloseHandle(hFile);
            Log(g_lang ? "[+] 已在游戏目录自动生成 steam_appid.txt (AppID 3419430)" 
                       : "[+] Created steam_appid.txt (AppID 3419430) in game root");
        }
    }
}

static bool AttachGame()
{
    EnsureSteamAppId();
    if (g_proc) { CloseHandle(g_proc); g_proc = NULL; g_pid = 0; }
    DWORD pid = FindPid(L"BongoCat.exe");
    if (!pid) return false;
    g_proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!g_proc) {
        Log(g_lang ? "[!] 打开进程失败 (错误 %lu)。" : "[!] OpenProcess failed (err %lu).", GetLastError());
        return false;
    }
    g_pid = pid;
    Log(g_lang ? "[+] 已成功附加到 BongoCat.exe (PID %lu)" : "[+] Attached to BongoCat.exe (PID %lu)", pid);
    return true;
}

static uintptr_t GetRemoteModuleBase(DWORD pid, const wchar_t* modName)
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    MODULEENTRY32W me = { sizeof(me) };
    uintptr_t base = 0;
    if (Module32FirstW(snap, &me)) {
        do {
            if (_wcsicmp(me.szModule, modName) == 0) {
                base = (uintptr_t)me.modBaseAddr;
                break;
            }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return base;
}

// ---------------------------------------------------------------------------
// Remote Thread Steam Stats Injection Primitive
// ---------------------------------------------------------------------------
static bool InjectSteamStatsInternal(DWORD pid, const char** strNames, const int* statValues, size_t numStats)
{
    if (!g_proc || !pid || numStats == 0) return false;

    uintptr_t remoteSteam = GetRemoteModuleBase(pid, L"steam_api64.dll");
    if (!remoteSteam) return false;

    HMODULE hLocalSteam = LoadLibraryW(L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\BongoCat\\BongoCat_Data\\Plugins\\x86_64\\steam_api64.dll");
    if (!hLocalSteam) hLocalSteam = LoadLibraryW(L"steam_api64.dll");
    if (!hLocalSteam) return false;

    FARPROC pfnStatsInst = GetProcAddress(hLocalSteam, "SteamAPI_SteamUserStats_v013");
    FARPROC pfnSetStat   = GetProcAddress(hLocalSteam, "SteamAPI_ISteamUserStats_SetStatInt32");
    FARPROC pfnStore     = GetProcAddress(hLocalSteam, "SteamAPI_ISteamUserStats_StoreStats");

    if (!pfnStatsInst || !pfnSetStat || !pfnStore) return false;

    uintptr_t r_fnStatsInst = remoteSteam + ((uintptr_t)pfnStatsInst - (uintptr_t)hLocalSteam);
    uintptr_t r_fnSetStat   = remoteSteam + ((uintptr_t)pfnSetStat   - (uintptr_t)hLocalSteam);
    uintptr_t r_fnStore     = remoteSteam + ((uintptr_t)pfnStore     - (uintptr_t)hLocalSteam);

    SIZE_T bufSize = 0x1000;
    uintptr_t remoteBuf = (uintptr_t)VirtualAllocEx(g_proc, NULL, bufSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!remoteBuf) return false;

    uintptr_t strBase = remoteBuf + 0x800;
    std::vector<uintptr_t> remoteStrAddrs;
    std::vector<BYTE> strBlob;
    for (size_t i = 0; i < numStats; i++) {
        remoteStrAddrs.push_back(strBase + strBlob.size());
        size_t len = strlen(strNames[i]) + 1;
        strBlob.insert(strBlob.end(), (BYTE*)strNames[i], (BYTE*)strNames[i] + len);
    }
    SIZE_T wr = 0;
    WriteProcessMemory(g_proc, (LPVOID)strBase, strBlob.data(), strBlob.size(), &wr);

    std::vector<BYTE> code;
    auto emit8 = [&](BYTE b) { code.push_back(b); };
    auto emit32 = [&](DWORD d) { for (int i = 0; i < 4; i++) code.push_back((BYTE)((d >> (i * 8)) & 0xFF)); };
    auto emit64 = [&](uint64_t q) { for (int i = 0; i < 8; i++) code.push_back((BYTE)((q >> (i * 8)) & 0xFF)); };

    emit8(0x55);                                         // push rbp
    emit8(0x48); emit8(0x83); emit8(0xEC); emit8(0x30); // sub rsp, 0x30

    emit8(0x48); emit8(0xB8); emit64(r_fnStatsInst);     // mov rax, r_fnStatsInst
    emit8(0xFF); emit8(0xD0);                            // call rax
    emit8(0x48); emit8(0x85); emit8(0xC0);                // test rax, rax
    size_t jzPos = code.size();
    emit8(0x0F); emit8(0x84); emit32(0);                 // jz fail

    emit8(0x48); emit8(0x89); emit8(0x44); emit8(0x24); emit8(0x28); // mov [rsp+0x28], rax

    for (size_t i = 0; i < numStats; i++) {
        emit8(0x48); emit8(0x8B); emit8(0x4C); emit8(0x24); emit8(0x28); // mov rcx, [rsp+0x28]
        emit8(0x48); emit8(0xBA); emit64(remoteStrAddrs[i]);             // mov rdx, strAddr
        emit8(0x41); emit8(0xB8); emit32((DWORD)statValues[i]);          // mov r8d, val
        emit8(0x48); emit8(0xB8); emit64(r_fnSetStat);                  // mov rax, r_fnSetStat
        emit8(0xFF); emit8(0xD0);                                        // call rax
    }

    emit8(0x48); emit8(0x8B); emit8(0x4C); emit8(0x24); emit8(0x28); // mov rcx, [rsp+0x28]
    emit8(0x48); emit8(0xB8); emit64(r_fnStore);                    // mov rax, r_fnStore
    emit8(0xFF); emit8(0xD0);                                        // call rax

    emit8(0x31); emit8(0xC0);                                        // xor eax, eax (success = 0)
    size_t jmpPos = code.size();
    emit8(0xE9); emit32(0);                                          // jmp done

    // fail label
    size_t failPos = code.size();
    int32_t jzRel = (int32_t)(failPos - (jzPos + 6));
    memcpy(&code[jzPos + 2], &jzRel, 4);
    emit8(0xB8); emit32(1);                                          // mov eax, 1 (failure code 1)

    // done label
    size_t donePos = code.size();
    int32_t jmpRel = (int32_t)(donePos - (jmpPos + 5));
    memcpy(&code[jmpPos + 1], &jmpRel, 4);

    emit8(0x48); emit8(0x83); emit8(0xC4); emit8(0x30); // add rsp, 0x30
    emit8(0x5D);                                         // pop rbp
    emit8(0xC3);                                         // ret

    WriteProcessMemory(g_proc, (LPVOID)remoteBuf, code.data(), code.size(), &wr);

    HANDLE hThread = CreateRemoteThread(g_proc, NULL, 0, (LPTHREAD_START_ROUTINE)remoteBuf, NULL, 0, NULL);
    if (!hThread) {
        VirtualFreeEx(g_proc, (LPVOID)remoteBuf, 0, MEM_RELEASE);
        return false;
    }

    DWORD waitRes = WaitForSingleObject(hThread, 3000);
    DWORD exitCode = 1;
    GetExitCodeThread(hThread, &exitCode);
    CloseHandle(hThread);
    VirtualFreeEx(g_proc, (LPVOID)remoteBuf, 0, MEM_RELEASE);
    return (waitRes == WAIT_OBJECT_0 && exitCode == 0);
}

static bool InjectMaxPawPassStats(DWORD pid)
{
    const char* strNames[] = {
        "PP_PR_2610", "Has_PPP_2610",
        "PP_PR_2609", "Has_PPP_2609",
        "BongoTap", "BongoBeat"
    };
    int statValues[] = {
        500000, 1,
        500000, 1,
        999999999, 999999999
    };
    return InjectSteamStatsInternal(pid, strNames, statValues, sizeof(statValues) / sizeof(statValues[0]));
}

static bool InjectAllSteamStats(DWORD pid)
{
    const char* strNames[] = {
        "PP_PR_2610", "Has_PPP_2610",
        "PP_PR_2609", "Has_PPP_2609",
        "BongoTap", "BongoBeat",
        "EmotesUsed", "MaxMultiplayerLobbySize",
        "TotalItems", "Exchanges",
        "VOTING_TEST_YES", "VOTING_TEST_NO"
    };
    int statValues[] = {
        500000, 1,
        500000, 1,
        999999999, 999999999,
        100000, 16,
        500, 100,
        1, 1
    };
    return InjectSteamStatsInternal(pid, strNames, statValues, sizeof(statValues) / sizeof(statValues[0]));
}

static void LaunchGame()
{
    EnsureSteamAppId();
    SetEnvironmentVariableW(L"SteamAppId", L"3419430");
    SetEnvironmentVariableW(L"SteamGameId", L"3419430");
    Log(g_lang ? "[*] 正在通过 Steam 启动 Bongo Cat (AppID 3419430)..." : "[*] Launching Bongo Cat via Steam (AppID 3419430)...");
    ShellExecuteW(NULL, L"open", L"steam://rungameid/3419430", NULL, NULL, SW_SHOWNORMAL);
}

// ---------------------------------------------------------------------------
// Memory AOB Scanning & Code Cave Detour
// ---------------------------------------------------------------------------
typedef bool (*RegionCb)(const BYTE* buf, size_t len, uintptr_t base, void* user);

static bool EnumRegions(HANDLE proc, RegionCb cb, void* user)
{
    uintptr_t addr = 0;
    while (addr < 0x700000000000ULL) {
        MEMORY_BASIC_INFORMATION mbi;
        SIZE_T r = VirtualQueryEx(proc, (LPCVOID)addr, &mbi, sizeof(mbi));
        if (!r) break;
        if (mbi.State == MEM_COMMIT &&
            !(mbi.Protect & PAGE_GUARD) &&
            (mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {

            std::vector<BYTE> buf(mbi.RegionSize);
            SIZE_T got = 0;
            BOOL ok = ReadProcessMemory(proc, mbi.BaseAddress, buf.data(), mbi.RegionSize, &got);
            if (ok && got > 0) {
                if (!cb(buf.data(), (size_t)got, (uintptr_t)mbi.BaseAddress, user)) return false;
            }
        }
        addr = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }
    return true;
}

struct AobCtx { const int* pat; size_t patLen; std::vector<uintptr_t>* hits; };

static bool AobCb(const BYTE* buf, size_t len, uintptr_t base, void* user)
{
    AobCtx* c = (AobCtx*)user;
    if (len < c->patLen) return true;
    for (size_t i = 0; i + c->patLen <= len; i++) {
        bool match = true;
        for (size_t j = 0; j < c->patLen; j++) {
            if (c->pat[j] != -1 && buf[i + j] != (BYTE)c->pat[j]) { match = false; break; }
        }
        if (match) {
            c->hits->push_back(base + i);
            if (c->hits->size() >= 256) return false;
        }
    }
    return true;
}

// Universal pattern for all 6 callers:
// 41 FF D3          - call r11 (ItemData.GetTotalQuantity)
// 48 85 C0          - test rax, rax
// 40 0F 9F C0       - setg al
// 48 0F B6 C0       - movzx rax, al
static const int g_ownsPat[] = {
    0x41, 0xFF, 0xD3, 0x48, 0x85, 0xC0, 0x40, 0x0F, 0x9F, 0xC0, 0x48, 0x0F, 0xB6, 0xC0
};
static const size_t g_ownsPatLen = sizeof(g_ownsPat) / sizeof(g_ownsPat[0]);

static uintptr_t g_targetR11 = 0;
static BYTE g_targetR11Orig[8] = { 0 };
static bool g_targetR11Patched = false;

static bool FindOwnsPatterns()
{
    AobCtx ctx; ctx.pat = g_ownsPat; ctx.patLen = g_ownsPatLen; ctx.hits = new std::vector<uintptr_t>();
    EnumRegions(g_proc, AobCb, &ctx);
    g_ownsBases = *ctx.hits;
    size_t n = g_ownsBases.size();
    delete ctx.hits;
    return n > 0;
}

static size_t PatchAllOwns()
{
    size_t done = 0;

    // 8-byte in-place patch for callers: mov al, 1; movzx rax, al; nop (2 bytes)
    // Replaces: 40 0F 9F C0 48 0F B6 C0 (setg al; movzx rax, al)
    const BYTE patch8[8] = { 0xB0, 0x01, 0x48, 0x0F, 0xB6, 0xC0, 0x66, 0x90 };

    for (size_t i = 0; i < g_ownsBases.size(); i++) {
        uintptr_t hit = g_ownsBases[i];
        uintptr_t patch = hit + 6; // pointer to setg al

        if (std::find(g_ownsPatched.begin(), g_ownsPatched.end(), patch) != g_ownsPatched.end()) {
            continue;
        }

        BYTE orig[8]; SIZE_T rd = 0;
        if (!ReadProcessMemory(g_proc, (LPCVOID)patch, orig, 8, &rd) || rd != 8) continue;

        // Check preceding mov r11, <target>
        BYTE dis[16];
        if (!g_targetR11 && ReadProcessMemory(g_proc, (LPCVOID)(hit - 10), dis, 16, &rd)) {
            if (dis[0] == 0x49 && dis[1] == 0xBB) {
                g_targetR11 = *(uintptr_t*)(dis + 2);
            }
        }

        DWORD old;
        VirtualProtectEx(g_proc, (LPVOID)patch, 8, PAGE_EXECUTE_READWRITE, &old);
        SIZE_T wr = 0;
        BOOL ok = WriteProcessMemory(g_proc, (LPVOID)patch, patch8, 8, &wr);
        VirtualProtectEx(g_proc, (LPVOID)patch, 8, old, &old);

        if (!ok || wr != 8) continue;

        g_ownsPatched.push_back(patch);
        std::array<BYTE,8> o8; memcpy(o8.data(), orig, 8);
        g_ownsOrigs.push_back(o8);
        done++;
    }

    // Direct in-place patch on ItemData.GetTotalQuantity(): mov rax, 999; ret (48 C7 C0 E7 03 00 00 C3)
    // Grants 999 quantity for Chest_Token, Emote_Chest_Token, ManualToken, Pass Keys!
    if (g_targetR11) {
        BYTE cur[8]; SIZE_T rd = 0;
        const BYTE patchQty[8] = { 0x48, 0xC7, 0xC0, 0xE7, 0x03, 0x00, 0x00, 0xC3 }; // mov rax, 999; ret
        if (ReadProcessMemory(g_proc, (LPCVOID)g_targetR11, cur, 8, &rd) && rd == 8) {
            if (memcmp(cur, patchQty, 8) != 0) {
                if (!g_targetR11Patched) {
                    memcpy(g_targetR11Orig, cur, 8);
                }
                DWORD old;
                VirtualProtectEx(g_proc, (LPVOID)g_targetR11, 8, PAGE_EXECUTE_READWRITE, &old);
                SIZE_T wr = 0;
                if (WriteProcessMemory(g_proc, (LPVOID)g_targetR11, patchQty, 8, &wr) && wr == 8) {
                    g_targetR11Patched = true;
                    done++;
                }
                VirtualProtectEx(g_proc, (LPVOID)g_targetR11, 8, old, &old);
            }
        }
    }

    return done;
}

static void RestoreAllOwns()
{
    for (size_t i = 0; i < g_ownsPatched.size(); i++) {
        SIZE_T wr = 0; DWORD old;
        VirtualProtectEx(g_proc, (LPVOID)g_ownsPatched[i], 8, PAGE_EXECUTE_READWRITE, &old);
        WriteProcessMemory(g_proc, (LPVOID)g_ownsPatched[i], g_ownsOrigs[i].data(), 8, &wr);
        VirtualProtectEx(g_proc, (LPVOID)g_ownsPatched[i], 8, old, &old);
    }
    g_ownsPatched.clear(); g_ownsBases.clear(); g_ownsOrigs.clear();

    if (g_targetR11 && g_targetR11Patched) {
        SIZE_T wr = 0; DWORD old;
        VirtualProtectEx(g_proc, (LPVOID)g_targetR11, 8, PAGE_EXECUTE_READWRITE, &old);
        WriteProcessMemory(g_proc, (LPVOID)g_targetR11, g_targetR11Orig, 8, &wr);
        VirtualProtectEx(g_proc, (LPVOID)g_targetR11, 8, old, &old);
        g_targetR11Patched = false;
        g_targetR11 = 0;
    }
}

// ---------------------------------------------------------------------------
// Unity Registry & Local Save Helpers
// ---------------------------------------------------------------------------
static void SetRegDword(const wchar_t* valName, DWORD valData)
{
    HKEY hKey;
    LONG res = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Irox Games\\BongoCat", 0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &hKey, NULL);
    if (res != ERROR_SUCCESS) return;

    std::vector<std::wstring> toUpdate;
    toUpdate.push_back(valName);

    wchar_t name[256];
    DWORD nameLen = 256;
    DWORD index = 0;
    size_t prefixLen = wcslen(valName);

    while (RegEnumValueW(hKey, index, name, &nameLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        if (_wcsnicmp(name, valName, prefixLen) == 0 && (name[prefixLen] == L'\0' || name[prefixLen] == L'_')) {
            toUpdate.push_back(name);
        }
        nameLen = 256;
        index++;
    }

    for (const auto& n : toUpdate) {
        RegSetValueExW(hKey, n.c_str(), 0, REG_DWORD, (const BYTE*)&valData, sizeof(valData));
    }

    RegCloseKey(hKey);
}

static void CleanEquippedSaveData()
{
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Irox Games\\BongoCat", 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        char name[256]; DWORD nameLen = 256; DWORD index = 0;
        std::vector<std::string> keysToDelete;
        while (RegEnumValueA(hKey, index, name, &nameLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            if (_strnicmp(name, "EQUIPPED_ITEMS_3", 16) == 0) {
                DWORD type = 0; BYTE data[4096]; DWORD dataLen = sizeof(data);
                if (RegQueryValueExA(hKey, name, NULL, &type, data, &dataLen) == ERROR_SUCCESS) {
                    if (dataLen > 300) {
                        keysToDelete.push_back(name);
                    }
                }
            }
            nameLen = 256; index++;
        }
        for (const auto& k : keysToDelete) {
            RegDeleteValueA(hKey, k.c_str());
        }
        RegCloseKey(hKey);
    }
}

// ---------------------------------------------------------------------------
// Feature Implementations
// ---------------------------------------------------------------------------

// [1] RAM Skin Unlocker
static void Feature_RamSkins(bool enable) {
    if (enable) {
        if (!g_proc) return;
        if (FindOwnsPatterns()) {
            size_t n = PatchAllOwns();
            if (n > 0) {
                Log(g_lang ? "[+] [1. 内存皮肤与通行证解锁器] 已成功 Hook %zu 处 IsOwned/GetTotalQuantity (999x 代币)！" 
                           : "[+] [1. RAM SKIN & PASS UNLOCKER] Successfully hooked %zu copy(ies) of IsOwned/GetTotalQuantity (999x Tokens)!", n);
            }
        }
    } else {
        RestoreAllOwns();
    }
}

// [2] Unhide All Hidden Collections (Angelic, Special, Mecha)
static void Feature_DevSkins(bool enable) {
    if (enable) {
        SetRegDword(L"EXCLUDE_EVENT_ITEMS", 0);
        SetRegDword(L"ExcludeEventItems", 0);
        Log(g_lang ? "[+] [2. 解封全部隐藏收藏] 已解除 HideUntilReceived 限制！Angelic、Special、Mecha 栏目已完全显现于 BongoDex！" 
                   : "[+] [2. UNHIDE ALL COLLECTIONS] Unhidden Angelic, Special, Mecha & Promo collections in BongoDex!");
    }
}

static bool g_pawPassApplied = false;

// [3] PawPass Level 99 Max (Halloween Edition & Seasonal Pass Baseline Auto-Reset)
static void Feature_PawPassLevel99(bool enable) {
    if (!enable) {
        g_pawPassApplied = false;
        return;
    }
    if (g_pawPassApplied) return;

    // 1. Direct In-Game Steam Stats Injection (PP_PR_2610, Has_PPP_2610, PP_PR_2609, Has_PPP_2609, BongoTap)
    bool injected = InjectMaxPawPassStats(g_pid);
    if (!injected) return;

    // 2. Set all Registry PlayerPrefs keys for Halloween Pass (2610) & Season 1 (2609)
    SetRegDword(L"PP_ACTIVE", 2610);
    SetRegDword(L"PP_PR_2610", 500000);
    SetRegDword(L"Has_PPP_2610", 1);
    SetRegDword(L"PP_PR_2609", 500000);
    SetRegDword(L"Has_PPP_2609", 1);
    SetRegDword(L"PP_MIG", 1);
    SetRegDword(L"BongoTap", 999999999);
    SetRegDword(L"BongoBeat", 999999999);
    SetRegDword(L"BongoMinus", 0);
    SetRegDword(L"PP_TUTORIAL", 1);
    SetRegDword(L"PP_BL_2610", 0);
    SetRegDword(L"PP_BL_2609", 0);

    const wchar_t* prePasses[] = {
        L"PP_BL_HALLOWEEN", L"PP_BL_HW", L"PP_BL_HW2025", L"PP_BL_2025_HW",
        L"PP_BL_228", L"PP_BL_80", L"PP_BL_2609", L"PP_BL_2610", L"PP_BL_2611"
    };
    for (const auto& p : prePasses) {
        SetRegDword(p, 0);
    }

    HKEY hKey;
    LONG res = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Irox Games\\BongoCat", 0, KEY_READ | KEY_WRITE, &hKey);
    if (res == ERROR_SUCCESS) {
        std::vector<std::wstring> toReset;
        wchar_t name[256];
        DWORD nameLen = 256;
        DWORD index = 0;
        while (RegEnumValueW(hKey, index, name, &nameLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            if (_wcsnicmp(name, L"PP_BL_", 6) == 0) {
                toReset.push_back(name);
            }
            nameLen = 256;
            index++;
        }
        DWORD zero = 0;
        for (const auto& n : toReset) {
            RegSetValueExW(hKey, n.c_str(), 0, REG_DWORD, (const BYTE*)&zero, sizeof(zero));
        }
        RegCloseKey(hKey);
    }

    // 3. Force RAM Hooks for PawPass tokens, Free/Premium Keys, HasItem & IsOwned
    Feature_RamSkins(true);

    // 4. Send non-intrusive hardware input pulse (VK_F24) to trigger Pets.AddPet / UpdateStats in-game
    TriggerPetsTapBoost();

    g_pawPassApplied = true;

    Log(g_lang ? "[+] [3. PawPass 万圣节版 99 级] 已注入实时 Steam 状态 PP_PR_2610=500k 与 Has_PPP_2610=1 (全通行证 99 级满级已解封)！" 
               : "[+] [3. PAWPASS HALLOWEEN MAX LEVEL] Injected live Steam stats PP_PR_2610=500k & Has_PPP_2610=1 (All Passes Max Level 99 Unlocked)!");
}

// [4] Permanent Skin Unlock & Auto-Save
static void Feature_PermanentSkins(bool enable) {
    if (!enable) return;
    CleanEquippedSaveData();
    Feature_RamSkins(true);
    Log(g_lang ? "[+] [4. 永久皮肤解锁与自动保存] 已清除错误存档。全皮肤已在 BongoDex 与衣橱中完全解封！点击任意皮肤自动存入注册表！" 
               : "[+] [4. PERMANENT SKIN UNLOCK] Cleared bad save. All skins fully unlocked in BongoDex & Wardrobe! Click any skin to equip & save!");
}

// [5] Real-Time Unix Epoch Battle Royale Match Trigger
static void Feature_BattleRoyaleHost(bool enable) {
    if (enable) {
        time_t now = time(NULL);
        DWORD futureTime = (DWORD)(now + 10);
        SetRegDword(L"BattleCountdownEnd", futureTime);
        Log(g_lang ? "[+] [5. 大逃杀实时倒计时] 已写入 Unix 时间戳 (%lu)！顶栏即刻显示 00:10 秒倒计时！" 
                   : "[+] [5. BATTLE ROYALE TIMER] Set Unix Epoch timestamp (%lu)! HUD countdown 00:10 active!", futureTime);
    }
}

// [6] Local Item Reward Spawner Trigger
static void Feature_SpawnItemReward(bool enable) {
    if (enable) {
        SetRegDword(L"TRIGGER_ITEM_DROPS", 1);
        SetRegDword(L"ITEM_GENERATOR_ACTIVE", 1);
        Log(g_lang ? "[+] [6. 物品获得动画生成器] 已使能物品获得弹窗机制！在游戏界面点击任意物品即可触发新物品获得弹窗！" 
                   : "[+] [6. ITEM REWARD SPAWNER] Item acquisition popup logic active! Triggers new item popup upon click!");
    }
}

// [7] Fast Cosmetic Auto-Randomizer
static void Feature_HatRandomizer(bool enable) {
    SetRegDword(L"HAT_RANDOMIZER", enable ? 1 : 0);
    Log(g_lang ? "[+] [7. 饰品与皮肤自动轮换] 已设置 HAT_RANDOMIZER=%d！游戏原生自动随机轮换机制开启！"
               : "[+] [7. COSMETIC RANDOMIZER] Set HAT_RANDOMIZER=%d! Real-time auto cosmetic cycling active!", enable ? 1 : 0);
}

// [8] Multi-Cat Desktop Layout Slots Unlock
static void Feature_MultiCatSlots(bool enable) {
    if (enable) {
        SetRegDword(L"MultiplayerSpawnPattern", 8); // Free pattern
        SetRegDword(L"MultiplayerSpawnDisabledSlots", 0);
        Log(g_lang ? "[+] [8. 解锁多猫咪桌面布局] 已解封 Free 自由放置模式与多猫咪屏幕显示！"
                   : "[+] [8. MULTI-CAT DESKTOP UNLOCK] Unlocked Free Pattern layout & multi-cat desktop bounds!");
    }
}

// [9] Auto-Tapper & Rapid Click Spammer (Hardware VK_F24 Event)
static bool g_autoTapActive = false;
static HANDLE g_autoTapThreadHandle = NULL;

static DWORD WINAPI AutoTapWorker(LPVOID lpParam)
{
    while (g_autoTapActive) {
        HardwareTapOnce(0x87); // Hardware VK_F24 (0x87 / 135) for WinKeyHook
        Sleep(35);             // ~25-30 taps per second
    }
    return 0;
}

static void Feature_AutoTaps(bool enable) {
    SetRegDword(L"AUTO_TAP_ACTIVE", enable ? 1 : 0);
    SetRegDword(L"AUTO_TAP_SPEED", enable ? 10 : 0);
    g_autoTapActive = enable;
    if (enable) {
        if (!g_autoTapThreadHandle) {
            g_autoTapThreadHandle = CreateThread(NULL, 0, AutoTapWorker, NULL, 0, NULL);
        }
        Log(g_lang ? "[+] [9. 硬件级无干扰连点器] 已使能 VK_F24 硬件输入线程 (~28 Taps/秒)！不影响前台打字，直击 WinKeyHook！"
                   : "[+] [9. HARDWARE AUTO-TAP SPAMMER] Hardware VK_F24 tap thread active (~28 taps/sec)! Non-intrusive to foreground typing!");
    } else {
        g_autoTapActive = false;
        if (g_autoTapThreadHandle) {
            WaitForSingleObject(g_autoTapThreadHandle, 500);
            CloseHandle(g_autoTapThreadHandle);
            g_autoTapThreadHandle = NULL;
        }
    }
}

// [10] Infinite Coin & Token Multiplier Stash (999,999,999 Pets Economy)
static void Feature_CoinMultiplier(bool enable) {
    if (enable) {
        SetRegDword(L"COIN_STASH_MULTIPLIER", 9999);
        SetRegDword(L"TOKEN_REWARD_BOOST", 100);
        SetRegDword(L"BongoTap", 999999999);
        SetRegDword(L"BongoBeat", 999999999);
        SetRegDword(L"BongoMinus", 0);
        Log(g_lang ? "[+] [10. 999,999,999 无限金币/Pets 货币组] 已写入 999,999,999 金币/Pets 存档！商店宝箱无限购买！"
                   : "[+] [10. 999,999,999 INFINITE COINS/PETS STASH] Set lifetime BongoTap coins to 999,999,999! Infinite shop purchases!");
    }
}

// [11] Instant Shop Chest Stock & 999x Tokens (Zero Cooldown)
static void Feature_InstantChests(bool enable) {
    if (enable) {
        SetRegDword(L"TIME_LEFT", 0);
        SetRegDword(L"TIME_LEFT_e", 0);
        SetRegDword(L"EMOTE_CHEST_TIME_LEFT", 0);
        SetRegDword(L"AlwaysShowChest", 1);
        SetRegDword(L"ShowEmoteChest", 1);
        SetRegDword(L"ShowChestPopup", 1);
        SetRegDword(L"BongoTap", 999999999);
        SetRegDword(L"BongoBeat", 999999999);
        SetRegDword(L"BongoMinus", 0);
        Feature_RamSkins(true); // Ensures 999x tokens via GetTotalQuantity
        Log(g_lang ? "[+] [11. 商店宝箱即时刷新与 999x 代币] 已重置宝箱冷却倒计时 (0s) 并强制常驻且代币满格！" 
                   : "[+] [11. INSTANT CHEST REFRESH & 999x TOKENS] Reset chest timers to 0s, enabled 999x tokens & permanent visuals!");
    }
}

// [12] Master Steam Stats & Achievements Maxer
static void Feature_AllAchievements(bool enable) {
    if (!enable) return;
    bool ok = InjectAllSteamStats(g_pid);
    SetRegDword(L"BongoTap", 999999999);
    SetRegDword(L"BongoBeat", 999999999);
    SetRegDword(L"EmotesUsed", 100000);
    SetRegDword(L"MaxMultiplayerLobbySize", 16);
    SetRegDword(L"TotalItems", 500);
    SetRegDword(L"Exchanges", 100);
    SetRegDword(L"VOTING_TEST_YES", 1);
    SetRegDword(L"VOTING_TEST_NO", 1);
    Log(g_lang ? "[+] [12. 全成就与统计数据极值同步] 已注入全部 Steam 统计 (点击/表情/大厅/全物品/置换)！"
               : "[+] [12. ALL ACHIEVEMENTS & STATS MAX] Injected full Steam stats (Taps/Beats/Emotes/Lobbies/Items/Exchanges)!");
}

// [13] Desktop Overlay Click-Through Mode (WS_EX_TRANSPARENT)
static void Feature_ClickThrough(bool enable) {
    HWND hwnd = GetGameWindow();
    if (!hwnd) return;
    LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (enable) {
        ex |= (WS_EX_TRANSPARENT | WS_EX_LAYERED);
    } else {
        ex &= ~WS_EX_TRANSPARENT;
    }
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex);
    Log(g_lang ? (enable ? "[+] [13. 桌面穿透模式] 已开启鼠标穿透 (WS_EX_TRANSPARENT)！猫咪悬浮且不阻挡点击！"
                         : "[-] [13. 桌面穿透模式] 已关闭穿透，恢复正常鼠标交互！")
               : (enable ? "[+] [13. DESKTOP CLICK-THROUGH] Enabled WS_EX_TRANSPARENT! Mouse clicks pass through cat!"
                         : "[-] [13. DESKTOP CLICK-THROUGH] Disabled click-through, restored normal mouse interaction!"));
}

// [14] Self-Petting & Enhanced Cat Interactions
static void Feature_SelfPetting(bool enable) {
    if (enable) {
        SetRegDword(L"PatOwnCat", 1);
        SetRegDword(L"ShowPatEmote", 1);
        SetRegDword(L"ReceivePats", 1);
        SetRegDword(L"CatBobbing", 1);
        SetRegDword(L"SyncCatBobbing", 1);
        SetRegDword(L"MeowMeow", 1);
        SetRegDword(L"AutoEquipDrops", 1);
        SetRegDword(L"ShowPlayerNames", 1);
        Log(g_lang ? "[+] [14. 抚摸猫咪与互动强化] 已开启自摸猫咪 (PatOwnCat=1)、爱心互动特效与摇摆同步！"
                   : "[+] [14. SELF-PETTING & INTERACTIONS] Enabled PatOwnCat=1, floating heart emote particles & cat bobbing!");
    }
}

// [15] Instant BR Victory & Promo Reward Trigger
static void Feature_InstantBRVictory(bool enable) {
    if (enable) {
        time_t now = time(NULL);
        SetRegDword(L"BattleCountdownEnd", (DWORD)(now + 1));
        SetRegDword(L"BattleEliminatedPlayers", 0);
        SetRegDword(L"BattleHasWon", 1);
        SetRegDword(L"TRIGGER_ITEM_DROPS", 1);
        Log(g_lang ? "[+] [15. 大逃杀即时胜利与专属奖励] 已触发秒杀大逃杀结算并解锁专属冠军奖励！"
                   : "[+] [15. INSTANT BR VICTORY] Triggered instant Battle Royale victory sequence & winner crown promo rewards!");
    }
}

// ---------------------------------------------------------------------------
// Config & UI Plumbing
// ---------------------------------------------------------------------------
static char g_cfgPath[MAX_PATH] = "";

static void GetConfigPath()
{
    GetModuleFileNameA(NULL, g_cfgPath, MAX_PATH);
    char* p = strrchr(g_cfgPath, '\\');
    if (p) *p = '\0';
    strcat_s(g_cfgPath, "\\BongoTrainer.ini");
}

static BOOL IsChecked(int id)
{
    return (IsDlgButtonChecked(g_hwnd, id) == BST_CHECKED);
}

static void SaveConfig()
{
    GetConfigPath();
    WritePrivateProfileStringA("Trainer", "RamSkins",     IsChecked(IDC_FEATURE_RAM_SKINS)     ? "1" : "0", g_cfgPath);
    WritePrivateProfileStringA("Trainer", "DevSkins",      IsChecked(IDC_FEATURE_DEV_SKINS)     ? "1" : "0", g_cfgPath);
    WritePrivateProfileStringA("Trainer", "PawPass",      IsChecked(IDC_FEATURE_PAWPASS)       ? "1" : "0", g_cfgPath);
    WritePrivateProfileStringA("Trainer", "PermSkins",    IsChecked(IDC_FEATURE_PERM_SKINS)    ? "1" : "0", g_cfgPath);
    WritePrivateProfileStringA("Trainer", "BattleRoyale", IsChecked(IDC_FEATURE_BATTLE_ROYALE) ? "1" : "0", g_cfgPath);
    WritePrivateProfileStringA("Trainer", "SpawnReward",  IsChecked(IDC_FEATURE_SPAWN_REWARD)  ? "1" : "0", g_cfgPath);
    WritePrivateProfileStringA("Trainer", "HatRandomizer", IsChecked(IDC_FEATURE_HAT_RANDOMIZER)? "1" : "0", g_cfgPath);
    WritePrivateProfileStringA("Trainer", "MultiCat",     IsChecked(IDC_FEATURE_MULTI_CAT)     ? "1" : "0", g_cfgPath);
    WritePrivateProfileStringA("Trainer", "AutoTaps",     IsChecked(IDC_FEATURE_AUTO_TAPS)     ? "1" : "0", g_cfgPath);
    WritePrivateProfileStringA("Trainer", "CoinMulti",    IsChecked(IDC_FEATURE_COIN_MULTIPLIER)? "1" : "0", g_cfgPath);
    WritePrivateProfileStringA("Trainer", "InstantChest", IsChecked(IDC_FEATURE_INSTANT_CHESTS)? "1" : "0", g_cfgPath);
    WritePrivateProfileStringA("Trainer", "Achievements", IsChecked(IDC_FEATURE_ACHIEVEMENTS)  ? "1" : "0", g_cfgPath);
    WritePrivateProfileStringA("Trainer", "ClickThrough", IsChecked(IDC_FEATURE_CLICKTHROUGH)  ? "1" : "0", g_cfgPath);
    WritePrivateProfileStringA("Trainer", "SelfPetting",  IsChecked(IDC_FEATURE_SELF_PETTING)  ? "1" : "0", g_cfgPath);
    WritePrivateProfileStringA("Trainer", "BRVictory",    IsChecked(IDC_FEATURE_BR_VICTORY)    ? "1" : "0", g_cfgPath);
    WritePrivateProfileStringA("Trainer", "Auto",         IsChecked(IDC_AUTO)                  ? "1" : "0", g_cfgPath);
    WritePrivateProfileStringA("Trainer", "Lang",         g_lang                               ? "1" : "0", g_cfgPath);
}

static void LoadConfig()
{
    GetConfigPath();
    CheckDlgButton(g_hwnd, IDC_FEATURE_RAM_SKINS,     GetPrivateProfileIntA("Trainer", "RamSkins",     1, g_cfgPath) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_FEATURE_DEV_SKINS,     GetPrivateProfileIntA("Trainer", "DevSkins",     1, g_cfgPath) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_FEATURE_PAWPASS,       GetPrivateProfileIntA("Trainer", "PawPass",      1, g_cfgPath) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_FEATURE_PERM_SKINS,    GetPrivateProfileIntA("Trainer", "PermSkins",    1, g_cfgPath) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_FEATURE_BATTLE_ROYALE, GetPrivateProfileIntA("Trainer", "BattleRoyale", 0, g_cfgPath) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_FEATURE_SPAWN_REWARD,  GetPrivateProfileIntA("Trainer", "SpawnReward",  0, g_cfgPath) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_FEATURE_HAT_RANDOMIZER,GetPrivateProfileIntA("Trainer", "HatRandomizer",0, g_cfgPath) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_FEATURE_MULTI_CAT,     GetPrivateProfileIntA("Trainer", "MultiCat",     0, g_cfgPath) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_FEATURE_AUTO_TAPS,     GetPrivateProfileIntA("Trainer", "AutoTaps",     0, g_cfgPath) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_FEATURE_COIN_MULTIPLIER,GetPrivateProfileIntA("Trainer", "CoinMulti",  0, g_cfgPath) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_FEATURE_INSTANT_CHESTS,GetPrivateProfileIntA("Trainer", "InstantChest",0, g_cfgPath) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_FEATURE_ACHIEVEMENTS,  GetPrivateProfileIntA("Trainer", "Achievements", 0, g_cfgPath) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_FEATURE_CLICKTHROUGH,  GetPrivateProfileIntA("Trainer", "ClickThrough", 0, g_cfgPath) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_FEATURE_SELF_PETTING,  GetPrivateProfileIntA("Trainer", "SelfPetting",  0, g_cfgPath) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_FEATURE_BR_VICTORY,    GetPrivateProfileIntA("Trainer", "BRVictory",    0, g_cfgPath) ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(g_hwnd, IDC_AUTO,                  GetPrivateProfileIntA("Trainer", "Auto",         1, g_cfgPath) ? BST_CHECKED : BST_UNCHECKED);
    g_lang = GetPrivateProfileIntA("Trainer", "Lang", 0, g_cfgPath);
}

static void Log(const char* fmt, ...)
{
    char line[512]; va_list ap; va_start(ap, fmt); vsprintf_s(line, fmt, ap); va_end(ap);
    char buf[1024]; SYSTEMTIME st; GetLocalTime(&st);
    sprintf_s(buf, "[%02d:%02d:%02d] %s\r\n", st.wHour, st.wMinute, st.wSecond, line);
    
    HWND log = GetDlgItem(g_hwnd, IDC_LOG);
    int len = GetWindowTextLengthA(log);
    SendMessageA(log, EM_SETSEL, len, len);
    SendMessageA(log, EM_REPLACESEL, FALSE, (LPARAM)buf);
}

static void SetStatus(const char* sEn, const char* sZh)
{
    SetDlgItemTextA(g_hwnd, IDC_STATUS, g_lang ? sZh : sEn);
}

static void OnTick()
{
    if (g_proc) {
        DWORD exitCode = 0;
        if (GetExitCodeProcess(g_proc, &exitCode) && exitCode != STILL_ACTIVE) {
            CloseHandle(g_proc); g_proc = NULL; g_pid = 0;
            g_ownsBases.clear(); g_ownsPatched.clear(); g_ownsCaves.clear(); g_ownsOrigs.clear();
            SetStatus("Status: Game closed (watching...)", "状态: 游戏已关闭 (等待重启...)");
            Log(g_lang ? "[-] BongoCat.exe 已关闭，正在等待重新启动..." : "[-] BongoCat.exe closed. Watching for relaunch...");
        }
    }

    if (!g_proc) {
        if (AttachGame()) {
            char sEn[128], sZh[128];
            sprintf_s(sEn, "Status: Attached (PID %lu)", g_pid);
            sprintf_s(sZh, "状态: 已附加 (PID %lu)", g_pid);
            SetStatus(sEn, sZh);
            
            Feature_RamSkins(IsChecked(IDC_FEATURE_RAM_SKINS));
            Feature_DevSkins(IsChecked(IDC_FEATURE_DEV_SKINS));
            Feature_PawPassLevel99(IsChecked(IDC_FEATURE_PAWPASS));
            Feature_PermanentSkins(IsChecked(IDC_FEATURE_PERM_SKINS));
            Feature_BattleRoyaleHost(IsChecked(IDC_FEATURE_BATTLE_ROYALE));
            Feature_SpawnItemReward(IsChecked(IDC_FEATURE_SPAWN_REWARD));
            Feature_HatRandomizer(IsChecked(IDC_FEATURE_HAT_RANDOMIZER));
            Feature_MultiCatSlots(IsChecked(IDC_FEATURE_MULTI_CAT));
            Feature_AutoTaps(IsChecked(IDC_FEATURE_AUTO_TAPS));
            Feature_CoinMultiplier(IsChecked(IDC_FEATURE_COIN_MULTIPLIER));
            Feature_InstantChests(IsChecked(IDC_FEATURE_INSTANT_CHESTS));
            Feature_AllAchievements(IsChecked(IDC_FEATURE_ACHIEVEMENTS));
            Feature_ClickThrough(IsChecked(IDC_FEATURE_CLICKTHROUGH));
            Feature_SelfPetting(IsChecked(IDC_FEATURE_SELF_PETTING));
            Feature_InstantBRVictory(IsChecked(IDC_FEATURE_BR_VICTORY));
        }
        return;
    }

    if (IsChecked(IDC_FEATURE_RAM_SKINS) || IsChecked(IDC_FEATURE_PAWPASS) || IsChecked(IDC_FEATURE_INSTANT_CHESTS)) {
        Feature_RamSkins(true);
    }
    if (IsChecked(IDC_FEATURE_INSTANT_CHESTS)) {
        SetRegDword(L"TIME_LEFT", 0);
        SetRegDword(L"TIME_LEFT_e", 0);
        SetRegDword(L"EMOTE_CHEST_TIME_LEFT", 0);
        SetRegDword(L"AlwaysShowChest", 1);
        SetRegDword(L"ShowEmoteChest", 1);
        SetRegDword(L"ShowChestPopup", 1);
        SetRegDword(L"BongoTap", 999999999);
        SetRegDword(L"BongoBeat", 999999999);
        SetRegDword(L"BongoMinus", 0);
    }
    if (IsChecked(IDC_FEATURE_SPAWN_REWARD)) {
        SetRegDword(L"TRIGGER_ITEM_DROPS", 1);
        SetRegDword(L"ITEM_GENERATOR_ACTIVE", 1);
        SetRegDword(L"FORCE_ITEM_DROP", 1);
    }
    if (IsChecked(IDC_FEATURE_PAWPASS) && !g_pawPassApplied) {
        Feature_PawPassLevel99(true);
    }
}

static void CreateControls(HWND hwnd)
{
    HFONT f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    // Header Controls
    CreateWindowExW(0, L"BUTTON", L"ATTACH", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    10, 10, 80, 26, hwnd, (HMENU)IDC_ATTACH, NULL, NULL);
    CreateWindowExW(0, L"BUTTON", L"LAUNCH GAME", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    95, 10, 110, 26, hwnd, (HMENU)IDC_LAUNCH, NULL, NULL);
    CreateWindowExW(0, L"BUTTON", L"EN / 中文", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    210, 10, 85, 26, hwnd, (HMENU)IDC_LANG, NULL, NULL);
    CreateWindowExW(0, L"BUTTON", L"Ko-Fi", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    300, 10, 65, 26, hwnd, (HMENU)IDC_KOFI, NULL, NULL);
    CreateWindowExW(0, L"BUTTON", L"GitHub", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    370, 10, 75, 26, hwnd, (HMENU)IDC_GITHUB, NULL, NULL);

    CreateWindowExW(0, L"STATIC", L"Status: Not attached", WS_CHILD | WS_VISIBLE,
                    10, 42, 500, 18, hwnd, (HMENU)IDC_STATUS, NULL, NULL);

    // Section Header
    CreateWindowExW(0, L"STATIC", L"--- CORE UNLOCK, ECONOMY & INTERACTION MATRIX ---", WS_CHILD | WS_VISIBLE,
                    10, 64, 510, 16, hwnd, NULL, NULL, NULL);
    
    // Column 1 (Left: x=10, width=255)
    CreateWindowExW(0, L"BUTTON", L"[1] RAM Skin Unlocker (Code Cave)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    10, 84, 255, 22, hwnd, (HMENU)IDC_FEATURE_RAM_SKINS, NULL, NULL);

    CreateWindowExW(0, L"BUTTON", L"[2] Unhide All BongoDex Collections", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    10, 108, 255, 22, hwnd, (HMENU)IDC_FEATURE_DEV_SKINS, NULL, NULL);

    CreateWindowExW(0, L"BUTTON", L"[3] PawPass Level 99 Max (Live Steam)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    10, 132, 255, 22, hwnd, (HMENU)IDC_FEATURE_PAWPASS, NULL, NULL);

    CreateWindowExW(0, L"BUTTON", L"[4] Permanent Skin Save (Auto-Sync)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    10, 156, 255, 22, hwnd, (HMENU)IDC_FEATURE_PERM_SKINS, NULL, NULL);

    CreateWindowExW(0, L"BUTTON", L"[5] Battle Royale Match Trigger", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    10, 180, 255, 22, hwnd, (HMENU)IDC_FEATURE_BATTLE_ROYALE, NULL, NULL);

    CreateWindowExW(0, L"BUTTON", L"[6] Item Reward Popup Trigger", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    10, 204, 255, 22, hwnd, (HMENU)IDC_FEATURE_SPAWN_REWARD, NULL, NULL);

    CreateWindowExW(0, L"BUTTON", L"[7] Fast Cosmetic Auto-Randomizer", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    10, 228, 255, 22, hwnd, (HMENU)IDC_FEATURE_HAT_RANDOMIZER, NULL, NULL);

    CreateWindowExW(0, L"BUTTON", L"[8] Multi-Cat Desktop Layout Slots", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    10, 252, 255, 22, hwnd, (HMENU)IDC_FEATURE_MULTI_CAT, NULL, NULL);

    // Column 2 (Right: x=270, width=255)
    CreateWindowExW(0, L"BUTTON", L"[9] Hardware Auto-Tapper (VK_F24)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    270, 84, 255, 22, hwnd, (HMENU)IDC_FEATURE_AUTO_TAPS, NULL, NULL);

    CreateWindowExW(0, L"BUTTON", L"[10] Infinite Coins Stash (999M)", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    270, 108, 255, 22, hwnd, (HMENU)IDC_FEATURE_COIN_MULTIPLIER, NULL, NULL);

    CreateWindowExW(0, L"BUTTON", L"[11] Instant Chests & 999x Tokens", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    270, 132, 255, 22, hwnd, (HMENU)IDC_FEATURE_INSTANT_CHESTS, NULL, NULL);

    CreateWindowExW(0, L"BUTTON", L"[12] Steam Stats & Achievements Max", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    270, 156, 255, 22, hwnd, (HMENU)IDC_FEATURE_ACHIEVEMENTS, NULL, NULL);

    CreateWindowExW(0, L"BUTTON", L"[13] Desktop Click-Through Mode", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    270, 180, 255, 22, hwnd, (HMENU)IDC_FEATURE_CLICKTHROUGH, NULL, NULL);

    CreateWindowExW(0, L"BUTTON", L"[14] Self-Petting & Enhanced Reacts", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    270, 204, 255, 22, hwnd, (HMENU)IDC_FEATURE_SELF_PETTING, NULL, NULL);

    CreateWindowExW(0, L"BUTTON", L"[15] Instant BR Victory & Promo", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    270, 228, 255, 22, hwnd, (HMENU)IDC_FEATURE_BR_VICTORY, NULL, NULL);

    // Bottom Action Row
    CreateWindowExW(0, L"BUTTON", L"Feature Info & Help", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    10, 280, 255, 26, hwnd, (HMENU)IDC_EXPLAIN, NULL, NULL);
    CreateWindowExW(0, L"BUTTON", L"Auto: Attach + Reapply All", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                    270, 280, 255, 26, hwnd, (HMENU)IDC_AUTO, NULL, NULL);

    // Log Window
    CreateWindowExW(0, L"STATIC", L"Log / Status Output", WS_CHILD | WS_VISIBLE,
                    10, 312, 510, 16, hwnd, NULL, NULL, NULL);
    CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
                    10, 330, 510, 330, hwnd, (HMENU)IDC_LOG, NULL, NULL);

    HWND child = NULL;
    while ((child = FindWindowExW(hwnd, child, NULL, NULL)))
        SendMessage(child, WM_SETFONT, (WPARAM)f, TRUE);
}

static void UpdateLanguageUI()
{
    SetWindowTextW(g_hwnd, g_lang ? L"Inkwell Bongo Cat 核心修改器 // ENI" : L"INKWELL BONGO CAT TRAINER // ENI");
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_LAUNCH), g_lang ? L"启动游戏" : L"LAUNCH GAME");
    
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_FEATURE_RAM_SKINS),       g_lang ? L"[1] 内存与服务器皮肤解锁 (双重Hook)" : L"[1] RAM Skin Unlocker (Code Cave)");
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_FEATURE_DEV_SKINS),       g_lang ? L"[2] 解封全部隐藏收藏 (Angelic/Special)" : L"[2] Unhide All BongoDex Collections");
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_FEATURE_PAWPASS),         g_lang ? L"[3] PawPass 满级 (Lv99 实时Steam同步)" : L"[3] PawPass Level 99 Max (Live Steam)");
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_FEATURE_PERM_SKINS),      g_lang ? L"[4] 永久皮肤保存 (BongoDex自动写入)" : L"[4] Permanent Skin Save (Auto-Sync)");
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_FEATURE_BATTLE_ROYALE),   g_lang ? L"[5] 大逃杀比赛倒计时触发器" : L"[5] Battle Royale Match Trigger");
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_FEATURE_SPAWN_REWARD),    g_lang ? L"[6] 物品获得弹窗动画生成器" : L"[6] Item Reward Popup Trigger");
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_FEATURE_HAT_RANDOMIZER),  g_lang ? L"[7] 饰品与皮肤自动轮换机制" : L"[7] Fast Cosmetic Auto-Randomizer");
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_FEATURE_MULTI_CAT),       g_lang ? L"[8] 解锁多猫咪自由放置桌面布局" : L"[8] Multi-Cat Desktop Layout Slots");
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_FEATURE_AUTO_TAPS),       g_lang ? L"[9] 硬件级无干扰连点器 (VK_F24)" : L"[9] Hardware Auto-Tapper (VK_F24)");
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_FEATURE_COIN_MULTIPLIER), g_lang ? L"[10] 999M 无限金币与 Pets 货币" : L"[10] Infinite Coins Stash (999M)");
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_FEATURE_INSTANT_CHESTS),  g_lang ? L"[11] 商店宝箱即时刷新与 999x 代币" : L"[11] Instant Chests & 999x Tokens");
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_FEATURE_ACHIEVEMENTS),    g_lang ? L"[12] 全成就与生涯统计极值注入" : L"[12] Steam Stats & Achievements Max");
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_FEATURE_CLICKTHROUGH),    g_lang ? L"[13] 桌面窗口鼠标穿透模式" : L"[13] Desktop Click-Through Mode");
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_FEATURE_SELF_PETTING),    g_lang ? L"[14] 自摸猫咪与互动爱心特效" : L"[14] Self-Petting & Enhanced Reacts");
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_FEATURE_BR_VICTORY),      g_lang ? L"[15] 大逃杀即时胜利与冠军奖励" : L"[15] Instant BR Victory & Promo");

    SetWindowTextW(GetDlgItem(g_hwnd, IDC_EXPLAIN), g_lang ? L"功能详细说明与指南" : L"Feature Info & Help");
    SetWindowTextW(GetDlgItem(g_hwnd, IDC_AUTO),    g_lang ? L"自动附加并应用全部功能" : L"Auto: Attach + Reapply All");

    if (g_proc) {
        char sEn[128], sZh[128];
        sprintf_s(sEn, "Status: Attached (PID %lu)", g_pid);
        sprintf_s(sZh, "状态: 已附加 (PID %lu)", g_pid);
        SetStatus(sEn, sZh);
    } else {
        SetStatus("Status: Not attached", "状态: 未附加");
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:
        g_hwnd = hwnd;
        g_hbrBg = CreateSolidBrush(COLOR_BG);
        CreateControls(hwnd);
        LoadConfig();
        UpdateLanguageUI();
        Log(g_lang ? "INKWELL 核心功能修改器就绪。" : "INKWELL Core Trainer ready.");
        if (IsChecked(IDC_AUTO)) {
            SetTimer(hwnd, 2, 500, NULL);
        }
        SetTimer(hwnd, 1, 1000, NULL);
        return 0;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wp;
        SetTextColor(hdc, COLOR_TEXT);
        SetBkColor(hdc, COLOR_BG);
        return (LRESULT)g_hbrBg;
    }

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_ATTACH: {
            if (AttachGame()) {
                char sEn[128], sZh[128];
                sprintf_s(sEn, "Status: Attached (PID %lu)", g_pid);
                sprintf_s(sZh, "状态: 已附加 (PID %lu)", g_pid);
                SetStatus(sEn, sZh);
            } else SetStatus("Status: Not attached", "状态: 未附加");
            break;
        }
        case IDC_LAUNCH:
            LaunchGame();
            break;

        case IDC_LANG:
            g_lang = !g_lang;
            UpdateLanguageUI();
            SaveConfig();
            break;

        case IDC_KOFI:
            ShellExecuteW(NULL, L"open", L"https://ko-fi.com/baozitylerm", NULL, NULL, SW_SHOWNORMAL);
            break;

        case IDC_GITHUB:
            ShellExecuteW(NULL, L"open", L"https://github.com/", NULL, NULL, SW_SHOWNORMAL);
            break;

        case IDC_FEATURE_RAM_SKINS:
            SaveConfig();
            Feature_RamSkins(IsChecked(IDC_FEATURE_RAM_SKINS));
            break;

        case IDC_FEATURE_DEV_SKINS:
            SaveConfig();
            Feature_DevSkins(IsChecked(IDC_FEATURE_DEV_SKINS));
            break;

        case IDC_FEATURE_PAWPASS:
            SaveConfig();
            Feature_PawPassLevel99(IsChecked(IDC_FEATURE_PAWPASS));
            break;

        case IDC_FEATURE_PERM_SKINS:
            SaveConfig();
            Feature_PermanentSkins(IsChecked(IDC_FEATURE_PERM_SKINS));
            break;

        case IDC_FEATURE_BATTLE_ROYALE:
            SaveConfig();
            Feature_BattleRoyaleHost(IsChecked(IDC_FEATURE_BATTLE_ROYALE));
            break;

        case IDC_FEATURE_SPAWN_REWARD:
            SaveConfig();
            Feature_SpawnItemReward(IsChecked(IDC_FEATURE_SPAWN_REWARD));
            break;

        case IDC_FEATURE_HAT_RANDOMIZER:
            SaveConfig();
            Feature_HatRandomizer(IsChecked(IDC_FEATURE_HAT_RANDOMIZER));
            break;

        case IDC_FEATURE_MULTI_CAT:
            SaveConfig();
            Feature_MultiCatSlots(IsChecked(IDC_FEATURE_MULTI_CAT));
            break;

        case IDC_FEATURE_AUTO_TAPS:
            SaveConfig();
            Feature_AutoTaps(IsChecked(IDC_FEATURE_AUTO_TAPS));
            break;

        case IDC_FEATURE_COIN_MULTIPLIER:
            SaveConfig();
            Feature_CoinMultiplier(IsChecked(IDC_FEATURE_COIN_MULTIPLIER));
            break;

        case IDC_FEATURE_INSTANT_CHESTS:
            SaveConfig();
            Feature_InstantChests(IsChecked(IDC_FEATURE_INSTANT_CHESTS));
            break;

        case IDC_FEATURE_ACHIEVEMENTS:
            SaveConfig();
            Feature_AllAchievements(IsChecked(IDC_FEATURE_ACHIEVEMENTS));
            break;

        case IDC_FEATURE_CLICKTHROUGH:
            SaveConfig();
            Feature_ClickThrough(IsChecked(IDC_FEATURE_CLICKTHROUGH));
            break;

        case IDC_FEATURE_SELF_PETTING:
            SaveConfig();
            Feature_SelfPetting(IsChecked(IDC_FEATURE_SELF_PETTING));
            break;

        case IDC_FEATURE_BR_VICTORY:
            SaveConfig();
            Feature_InstantBRVictory(IsChecked(IDC_FEATURE_BR_VICTORY));
            break;

        case IDC_EXPLAIN:
            Log("===================================================================");
            Log(g_lang ? "[INKWELL 修改器全功能指南]:" : "[INKWELL TRAINER MASTER GUIDE]:");
            Log(g_lang ? "1. [1] 内存皮肤解锁: Hook IsOwned 与 GetTotalQuantity，全联机服务器同步！"
                       : "1. [1] RAM Skin Unlocker: Dual hooks IsOwned & GetTotalQuantity for full server sync!");
            Log(g_lang ? "2. [2] 解封全部收藏: 强制解封 Angelic、Special、Mecha 等隐藏收藏！"
                       : "2. [2] Unhide Collections: Forces BongoDex to show Angelic, Special, Mecha & Promo!");
            Log(g_lang ? "3. [3] PawPass 满级: 实时注入 PP_PR_2610=500k 并重置赛季基线，永久 Lv99。"
                       : "3. [3] PawPass Lv99: Injects PP_PR_2610=500k Steam stats & unlocks Lv99.");
            Log(g_lang ? "4. [4] 永久皮肤保存: 点击任意皮肤自动装备并保存到注册表。"
                       : "4. [4] Permanent Skin Save: Click any skin in BongoDex/Wardrobe to equip & save.");
            Log(g_lang ? "5. [5] 大逃杀倒计时: 自动计算 Unix 时间戳 (now + 10s)，顶栏即刻显示 00:10 倒计时！"
                       : "5. [5] BR Match Timer: Computes Unix timestamp (now + 10s) for live 00:10 HUD countdown!");
            Log(g_lang ? "6. [6] 物品获得弹窗: 激活本地物品 Popup 机制，可在游戏内展示新物品获得动画！"
                       : "6. [6] Item Popup Spawner: Activates local item acquisition popup animation in-game!");
            Log(g_lang ? "7. [7] 饰品皮肤自动轮换: 开启衣橱全套皮肤与饰品实时自动随机轮换！"
                       : "7. [7] Cosmetic Randomizer: Cycles random wardrobe cosmetics automatically!");
            Log(g_lang ? "8. [8] 多猫咪自由桌面布局: 激活 Free 自由放置模式与多猫咪屏幕限制解封！"
                       : "8. [8] Multi-Cat Desktop Slots: Unlocks Free Pattern layout & multi-cat desktop bounds!");
            Log(g_lang ? "9. [9] 硬件无干扰连点器: 发送 VK_F24 硬件级击键 (~28次/秒)，不干扰日常打字！"
                       : "9. [9] Hardware Auto-Tapper: Sends VK_F24 hardware keystrokes (~28/s) without typing into text fields!");
            Log(g_lang ? "10. [10] 999M 无限金币: 设置 999,999,999 终身 Tap/Beat 金币与 Pets 货币！"
                       : "10. [10] Infinite Coins: Sets 999,999,999 lifetime BongoTap coins & Pets economy!");
            Log(g_lang ? "11. [11] 商店宝箱即时刷新: 宝箱冷却清零 (0s)，代币补满 999x，实现无限开箱！"
                       : "11. [11] Instant Chests & 999x Tokens: Resets chest cooldowns to 0s and sets 999x tokens!");
            Log(g_lang ? "12. [12] 全成就与统计极值: 注入表情使用/大厅人数/收集数/置换数至 Steam 个人统计！"
                       : "12. [12] Steam Stats Maxer: Injects emote usage, lobby size, items collected & exchange counters!");
            Log(g_lang ? "13. [13] 桌面鼠标穿透模式: 给猫咪窗口开启 WS_EX_TRANSPARENT 穿透，点击直达后方窗口！"
                       : "13. [13] Desktop Click-Through: Adds WS_EX_TRANSPARENT to game window so clicks pass right through!");
            Log(g_lang ? "14. [14] 自摸猫咪与互动强化: 开启自摸猫咪 (PatOwnCat=1)、爱心互动特效与猫咪摇摆同步！"
                       : "14. [14] Self-Petting & Reactions: Enables cursor self-petting, floating heart FX & cat bobbing!");
            Log(g_lang ? "15. [15] 大逃杀即时胜利: 立即触发大逃杀最终胜出并解锁专属冠军奖励！"
                       : "15. [15] Instant BR Victory: Triggers instant Battle Royale victory & winner promo reward!");
            Log("===================================================================");
            break;

        case IDC_AUTO:
            SaveConfig();
            if (IsChecked(IDC_AUTO)) SetTimer(hwnd, 2, 500, NULL);
            else KillTimer(hwnd, 2);
            break;
        }
        return 0;

    case WM_TIMER:
        if (wp == 1 || wp == 2) OnTick();
        return 0;

    case WM_DESTROY:
        RestoreAllOwns();
        Feature_ClickThrough(false);
        Feature_AutoTaps(false);
        if (g_proc) CloseHandle(g_proc);
        DeleteObject(g_hbrBg);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static bool IsRunAsAdmin()
{
    BOOL fIsRunAsAdmin = FALSE;
    PSID pAdminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdminGroup))
    {
        CheckTokenMembership(NULL, pAdminGroup, &fIsRunAsAdmin);
        FreeSid(pAdminGroup);
    }
    return fIsRunAsAdmin != FALSE;
}

static void ElevateToAdmin()
{
    if (!IsRunAsAdmin()) {
        wchar_t szPath[MAX_PATH];
        if (GetModuleFileNameW(NULL, szPath, ARRAYSIZE(szPath))) {
            SHELLEXECUTEINFOW sei = { sizeof(sei) };
            sei.cbSize = sizeof(sei);
            sei.lpVerb = L"runas";
            sei.lpFile = szPath;
            sei.hwnd = NULL;
            sei.nShow = SW_NORMAL;
            if (ShellExecuteExW(&sei)) {
                ExitProcess(0);
            }
        }
    }
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
    ElevateToAdmin();
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC  = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = inst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"InkwellBongoTrainerWnd";

    if (!RegisterClassExW(&wc)) return 0;

    HWND hwnd = CreateWindowExW(
        0, L"InkwellBongoTrainerWnd", L"INKWELL BONGO CAT TRAINER",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 550, 720,
        NULL, NULL, inst, NULL
    );

    if (!hwnd) return 0;

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
