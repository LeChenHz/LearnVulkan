#include <windows.h>
#include "Present_hook.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
std::shared_ptr<spdlog::logger> myLogger;

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        //auto logPath = getLogPath();
        //myLogger = spdlog::basic_logger_mt("file_logger", logPath, true);
        //myLogger->set_level(spdlog::level::info);
        //myLogger->flush_on(spdlog::level::info);
        HookPresent();
    }
    break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        UnHook();
        break;
    }
    return TRUE;
}

