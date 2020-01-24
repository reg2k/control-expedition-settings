#include "ExpeditionSettings.h"

#include "Logger.h"
#include "GameTypes.h"
#include "Utils.h"
#include "rva/RVA.h"
#include "minhook/include/MinHook.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <cinttypes>

#define INI_LOCATION "./plugins/ExpeditionSettings.ini"

using namespace GameTypes;

//===============
// Globals
//===============
Logger g_logger;

//===============
// Typedefs
//===============
using _ExpeditionManager_Init = void(*)(ExpeditionManager* expeditionManager, void* a2, bool a3);

//===================
// Addresses [1]
//===================
RVA<uintptr_t> ExpeditionManager_Init_Mid("49 8B 5E 10 8B F8 48 8B D3 48 8B CE E8 ? ? ? ? 48 83 C3 50");
_ExpeditionManager_Init ExpeditionManager_Init_Original = nullptr;

//===============================
// Utilities
//===============================

HMODULE GetRMDModule(const char* modName) {
    char szModuleName[MAX_PATH] = "";
    snprintf(szModuleName, sizeof(szModuleName), "%s_rmdwin7_f.dll", modName);
    HMODULE hMod = GetModuleHandleA(szModuleName);

    if (!hMod) {
        snprintf(szModuleName, sizeof(szModuleName), "%s_rmdwin10_f.dll", modName);
        hMod = GetModuleHandleA(szModuleName);
    }

    if (!hMod) {
        _LOG("WARNING: Could not get module: %s.", modName);
    }
    return hMod;
}

//=============
// Main
//=============

namespace ExpeditionSettings {

    static const char* ALL_TIERS_SECTION = "ExpeditionSettings";

    void SetExpeditionTimeLimit(ExpeditionTierDescription* tier, const char* sectionName) {
        int value = GetPrivateProfileIntA(sectionName, "iExpeditionTimeLimit", 0, INI_LOCATION);
        if (!value) {
            value = GetPrivateProfileIntA(ALL_TIERS_SECTION, "iExpeditionTimeLimit", 0, INI_LOCATION);
        }
        if (value)
            tier->expeditionTimeLimit = value;
    }

    void SetStartingPlayerProgressionLevel(ExpeditionTierDescription* tier, const char* sectionName) {
        int value = GetPrivateProfileIntA(sectionName, "iStartingPlayerProgressionLevel", 0, INI_LOCATION);
        if (!value)
            value = GetPrivateProfileIntA(ALL_TIERS_SECTION, "iStartingPlayerProgressionLevel", 0, INI_LOCATION);
        if (value)
            tier->startingPlayerProgressionLevel = value;
    }

    void SetNumTicketsToStart(ExpeditionTierDescription* tier, const char* sectionName) {
        int value = GetPrivateProfileIntA(sectionName, "iNumTickets", 0, INI_LOCATION);
        if (!value)
            value = GetPrivateProfileIntA(ALL_TIERS_SECTION, "iNumTickets", 0, INI_LOCATION);
        if (value)
            tier->numTicketsToStart = value;
    }

    void SetExpeditionTierParameters(ExpeditionTierDescription* tier, const char* sectionName) {
        SetExpeditionTimeLimit(tier, sectionName);
        SetStartingPlayerProgressionLevel(tier, sectionName);
        SetNumTicketsToStart(tier, sectionName);
    }

    void ExpeditionManager_Init_Hook(ExpeditionManager* expeditionManager, void* a2, bool a3) {
        _LOG("ExpeditionManager_Init_Hook");
        ExpeditionManager_Init_Original(expeditionManager, a2, a3);

        ExpeditionTierDescription* tier1 = &expeditionManager->expeditionTierDescriptionArr[0];
        ExpeditionTierDescription* tier2 = &expeditionManager->expeditionTierDescriptionArr[1];
        ExpeditionTierDescription* tier3 = &expeditionManager->expeditionTierDescriptionArr[2];

        // Individual Tiers
        SetExpeditionTierParameters(tier1, "Tier 1");
        SetExpeditionTierParameters(tier2, "Tier 2");
        SetExpeditionTierParameters(tier3, "Tier 3");

        _LOG("ExpeditionManager_Init_Hook END");
    }

    bool InitAddresses() {
        _LOG("Sigscan start");
        RVAUtils::Timer tmr; tmr.start();
        RVAManager::UpdateAddresses(0);

        _LOG("Sigscan elapsed: %llu ms.", tmr.stop());

        // Check if all addresses were resolved
        for (auto rvaData : RVAManager::GetAllRVAs()) {
            if (!rvaData->effectiveAddress) {
                _LOG("Not resolved: %s", rvaData->sig);
            }
        }
        if (!RVAManager::IsAllResolved()) return false;

        return true;
    }

    bool Hook() {
        // Get function address
        _LOG("ExpeditionManager_Init_Mid at %p", ExpeditionManager_Init_Mid.GetUIntPtr());
        uintptr_t moduleBase = (uintptr_t)GetModuleHandle(NULL);
        RUNTIME_FUNCTION* funcEntry = RtlLookupFunctionEntry(ExpeditionManager_Init_Mid.GetUIntPtr(), &moduleBase, NULL);
        if (!funcEntry) {
            _LOG("FATAL: Failed to resolve function!");
            return false;
        }

        LPVOID funcStart = reinterpret_cast<void*>(moduleBase + funcEntry->BeginAddress);
        _LOG("ExpeditionManager_Init at %p", funcStart);

        // Apply hooks
        MH_Initialize();
        MH_CreateHook(funcStart, ExpeditionManager_Init_Hook, reinterpret_cast<LPVOID*>(&ExpeditionManager_Init_Original));
        if (MH_EnableHook(funcStart) != MH_OK) {
            _LOG("FATAL: Failed to install hook.");
            return false;
        }
        return true;
    }

    void Init() {
        char logPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, NULL, logPath))) {
            strcat_s(logPath, "\\Remedy\\Control\\ExpeditionSettings.log");
            g_logger.Open(logPath);
        }
        _LOG("ExpeditionSettings v1.0 by reg2k");
        _LOG("Game version: %" PRIX64, Utils::GetGameVersion());
        _LOG("Module base: %p", GetModuleHandle(NULL));

        // Sigscan
        if (!InitAddresses()) {
            MessageBoxA(NULL, "ExpeditionSettings is not compatible with this version of Control.\nPlease visit the mod page for updates.", "ExpeditionSettings", MB_OK | MB_ICONEXCLAMATION);
            _LOG("FATAL: Incompatible version");
            return;
        }
        _LOG("Addresses set");

        // Apply hooks
        if (!Hook()) {
            _LOG("FATAL: Hook failed.");
            return;
        }
        _LOG("Hook done");
    }
}