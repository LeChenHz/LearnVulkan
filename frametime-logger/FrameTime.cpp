#include <iostream>
#include <windows.h>
#include <string>
#include <regex>

#pragma warning(push)
#pragma warning(disable:4081 4101 4267)
#include "cmdline.h"
#pragma warning(pop)

enum class OpCode
{
    None,
    Begin,
    End,
    Pause,
    Resume,
    Other,
};

void GetOpAndWriteSize(std::string str, std::string name, int* size, int* op)
{
    if (!strcmp(str.c_str(), "begin"))
    {
        *size = 2 * sizeof(int);
        *op = 1;
        return;
    }
    else if (!strcmp(str.c_str(), "end"))
    {
        *size = (int)(2 * sizeof(int) + name.length());
        *op = 2;
        return;
    }
    else if (!strcmp(str.c_str(), "pause"))
    {
        *size = (int)(2 * sizeof(int) + name.length());
        *op = 3;
        return;
    }
    else if (!strcmp(str.c_str(), "resume"))
    {
        *size = (int)(2 * sizeof(int) + name.length());
        *op = 4;
        return;
    }
}

void LevelUp()
{
    HANDLE hToken;
    LUID luid;
    TOKEN_PRIVILEGES tp;
    BOOL res = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
    if (!res) {
        printf("OpenProcessToken failed!\n");
    }
    res = LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);
    if (!res) {
        printf("LookupPrivilegeValue failed!\n");
    }
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    tp.Privileges[0].Luid = luid;
    res = AdjustTokenPrivileges(hToken, 0, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    if (!res) {
        printf("AdjustTokenPrivileges failed!\n");
    }
}

bool InjectDll(const char* dllPath, HANDLE hProcess)
{
    size_t BufSize = strlen(dllPath) + 1;
    LPVOID AllocAddr = VirtualAllocEx(hProcess, NULL, BufSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (AllocAddr == 0)
    {
        printf("VirtualAllocEx failed!\n");
        return false;
    }

    BOOL res = WriteProcessMemory(hProcess, AllocAddr, dllPath, BufSize, NULL);
    if (!res)
    {
        printf("WriteProcessMemory failed!\n");
        return false;
    }
    PTHREAD_START_ROUTINE pfnStartAddr = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "LoadLibraryA");

    HANDLE hRemoteThread;
    hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, pfnStartAddr, AllocAddr, 0, NULL);
    WaitForSingleObject(hRemoteThread, INFINITE);
    if (!hRemoteThread)
    {
        printf("Inject Dll failed");
        return false;
    }
    
    printf("Inject Dll Success");

    return true;
}


int main(int argc, char* argv[])
{
    //MessageBoxA(NULL, "text", "test", NULL);
    cmdline::parser params;
    params.add<int>("pid", 'p', "Process ID", false, 0);
    params.add<std::string>("control", 'c', "Control Command", false, "Idle");
    params.add<std::string>("name", 'n', "File Name", false, "");
    params.parse_check(argc, argv);

    int pid = params.get<int>("pid");
    std::string control = params.get<std::string>("control");
    std::string fileName = params.get<std::string>("name");

    LevelUp();
    if (pid != 0 && !strcmp(control.c_str(), "Idle"))
    {
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS | PROCESS_CREATE_THREAD, 0, pid);

        char temp[MAX_PATH];
        DWORD dwRet = GetModuleFileNameA(NULL, temp, MAX_PATH);
        if (dwRet == 0)
        {
            printf("Load Path Failed\n");
        }
        std::string dllPath(temp);
        dllPath = std::regex_replace(dllPath, std::regex("FrameTime.exe"), "FrameTimeHook.dll");

        bool res = InjectDll(dllPath.c_str(), hProcess);
        if (!res)
        {
            printf("Inject Dll Failed\n");
            return 1;
        }

        CloseHandle(hProcess);
    }

    if (strcmp(control.c_str(), "Idle"))
    {
        HANDLE pipe;
        std::string pipeName = "\\\\.\\pipe\\FrameTime" + std::to_string(pid);
        while (1)
        {
            pipe = CreateFileA(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

            if (pipe != INVALID_HANDLE_VALUE)
            {
                break;
            }

            if (GetLastError() != ERROR_PIPE_BUSY)
            {
                printf("CreateFileA failed. err=%d\n", GetLastError());
                return 1;
            }
            if (!WaitNamedPipeA(pipeName.c_str(), 60000))
            {
                printf("Could not open pipe\n");
                return 1;
            }
        }
        DWORD dwMode = PIPE_READMODE_MESSAGE;
        BOOL success = SetNamedPipeHandleState(pipe, &dwMode, NULL, NULL);
        if (!success)
        {
            printf("SetNamedPipeHandleState failed");
            return 1;
        }

        int writeSize = 0, op = 0;
        GetOpAndWriteSize(control, fileName, &writeSize, &op);
        DWORD bytesSize;
        BYTE* bytesWritten = new BYTE[writeSize];
        if (op == 2)
        {
            memcpy(bytesWritten, &writeSize, 4);
            memcpy(bytesWritten + 4, &op, 4);
            memcpy(bytesWritten, &writeSize, 4);
            memcpy(bytesWritten + sizeof(int), &op, 4);
            memcpy(bytesWritten + 2 * sizeof(int), fileName.c_str(), fileName.length());
        }
        else
        {
            memcpy(bytesWritten, &writeSize, 4);
            memcpy(bytesWritten + 4, &op, 4);
        }

        success = WriteFile(pipe, bytesWritten, writeSize, &bytesSize, NULL);
        if (!success || bytesSize != writeSize)
        {
            printf("WriteFile failed\n");
            return 1;
        }

        char reply[256];
        DWORD bytesRead;
        do
        {
            success = ReadFile(pipe, reply, sizeof(reply), &bytesRead, NULL);
            if (!success && GetLastError() != ERROR_MORE_DATA)
                break;
            if (bytesRead == 1)
            {
                int res = int(reply[0]);
                switch (res)
                {
                case 1:
                    printf("Warning: You shoule use resume command but not begin to reactive recording!\n");
                    break;
                case 2 :
                    printf("Error: You can not use end without begin!\n");
                    break;
                case 3:
                    printf("Error: You can not use pause without begin!\n");
                    break;
                case 4:
                    printf("Error: Not pausing state now, resume failed!\n");
                    break;
                default:
                    break;
                }
            }
            else
            {
                int res = int(reply[0]);
                printf("Result: %s\n", reply);
            }
        } while (!success);

        if (!success)
        {
            printf("ReadFile from pipe failed. err=%d\n", GetLastError());
            return 1;
        }

        CloseHandle(pipe);
    }

    return 1;
}

