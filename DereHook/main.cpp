#include <iostream>
#include <string>
#include <filesystem>
#include <Windows.h>

#include "hook.h"

namespace fs = std::filesystem;


BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        AllocConsole();
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

        init_hook();

    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        remove_hook();
        FreeConsole();

    }

    return TRUE;
}