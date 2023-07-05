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

        char buffer[MAX_PATH + 1];
        GetModuleFileNameA(nullptr, buffer, MAX_PATH);
        std::cout << "Module name: " << buffer << "\n";
        
        fs::path module_path(buffer);
        if (module_path.filename() != "imascgstage.exe")
            return -1;

        init_hook();

    }
    else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        remove_hook();
        FreeConsole();

    }

    return TRUE;
}