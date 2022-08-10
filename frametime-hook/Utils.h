#pragma once
#include <dxgi.h>
#include <dxgi1_2.h>
#include <stdio.h>
#include <string>
#include <regex>
#include <thread>
#include <mutex>
#include <chrono>
#include <fstream>

#define BUFSIZE 256
enum class RecordState
{
	Idle,
	Recording,
	Flushing,
	Pausing,
};

std::vector<long long>mFrameTimeArray;
RecordState mState = RecordState::Idle;
std::mutex mArrayMutex;

long long GetFrameTime();
int OnStateChanged(BYTE* buffer);
void DumpToFile(BYTE* buffer);
void AddFrameTime(long long time);

template <typename T>
void SafeRelease(T& resource)
{
	if (resource)
	{
		resource->Release();
		resource = nullptr;
	}
}

template <typename outType>
outType* CastComObject(IUnknown* object)
{
	outType* outObject = nullptr;
	HRESULT result = object->QueryInterface(__uuidof(outType), reinterpret_cast<void**>(&outObject));
	if (SUCCEEDED(result))
	{
		return outObject;
	}
	else
	{
		SafeRelease(outObject);
		return nullptr;
	}
}

_LARGE_INTEGER lastFrame{ 0 };
long long GetFrameTime()
{
	_LARGE_INTEGER nFreq;
	_LARGE_INTEGER currentFrame;

	auto now = std::chrono::high_resolution_clock::now();
	auto time_value = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

	QueryPerformanceFrequency(&nFreq);
	QueryPerformanceCounter(&currentFrame);
	if (lastFrame.QuadPart == 0)
	{
		lastFrame = currentFrame;
		return 0;
	}
	double deltaTime = (currentFrame.QuadPart - lastFrame.QuadPart) / (double)nFreq.QuadPart;
	lastFrame = currentFrame;
	//return 1000 * deltaTime;
	return time_value;

}

std::string getLogPath()
{
	HMODULE libRenderer = GetModuleHandle(L"libRenderer.dll");
	if (libRenderer == 0)
	{
		char tempPath[MAX_PATH];
		DWORD ret = GetTempPathA(MAX_PATH, tempPath);
		if (ret == 0)
		{
			printf("Get temp path failed");
		}
		std::string res(tempPath);
		return res + "frameLog.txt";
	}
	char path[MAX_PATH];
	GetModuleFileNameA(libRenderer, path, sizeof(path));
	std::string logPath(path);
	logPath = std::regex_replace(logPath, std::regex("libRenderer.dll"), "graphics_log\\frameLog.txt");
	return logPath;
}

enum class ErrorCode
{
	NONE,
	PauseToBegin,
	IdleToFlush,
	IdleToPause,
	PauseToRecord,
};
int OnStateChanged(BYTE* buffer)
{
	int opCode = *((int*)buffer + 1);
	switch (opCode)
	{
	case 1:
		if (mState == RecordState::Pausing)
		{
			mState = RecordState::Recording;
			return (int)ErrorCode::PauseToBegin;
		}
		mState = RecordState::Recording;
		break;
	case 2:
		if (mState == RecordState::Idle)
		{
			//error 2: Not recording state, flush fail
			return (int)ErrorCode::IdleToFlush;
		}
		mState = RecordState::Flushing;
		DumpToFile(buffer);
		mState = RecordState::Idle;
		break;
	case 3:
		if (mState == RecordState::Idle)
		{
			//error 3: Not recording state, pause fail
			return (int)ErrorCode::IdleToPause;
		}
		mState = RecordState::Pausing;
		break;
	case 4:
		if (mState != RecordState::Pausing)
		{
			//error 4: Not pausing state
			return (int)ErrorCode::PauseToRecord;
		}
		mState = RecordState::Recording;
		break;
	default:
		break;
	}
	return 0;
}

void AddFrameTime(long long time)
{
	std::lock_guard<std::mutex> _l(mArrayMutex);
	if (mState == RecordState::Recording)
	{
		mFrameTimeArray.push_back(time);
	}
}

void DumpToFile(BYTE* buffer)
{
	//MessageBoxA(NULL, "dump to file", "info", 0);
	std::lock_guard<std::mutex> _l(mArrayMutex);
	int totalSize = *(int*)buffer;
	int fileNameSize = totalSize - 2 * sizeof(int);
	static int i = 1;
	std::string logPath;
	HMODULE hookModule = GetModuleHandle(L"FrameTimeHook.dll");
	char path[MAX_PATH];
	GetModuleFileNameA(hookModule, path, sizeof(path));
	if (fileNameSize == 0)
	{
		logPath = std::string(path);
		logPath = std::regex_replace(logPath, std::regex("FrameTimeHook.dll"), "frameTime_") + std::to_string(i) + ".txt";
	}
	else
	{
		char* fileName = new char[fileNameSize + 1];
		memcpy(fileName, ((char*)buffer + 2 * sizeof(int)), fileNameSize);
		fileName[fileNameSize] = '\0';
		logPath = std::string(path);
		logPath = std::regex_replace(logPath, std::regex("FrameTimeHook.dll"), fileName) + ".txt";
		delete[]fileName;
	}
	std::fstream writeFile;
	writeFile.open(logPath, std::ios::out | std::ios::trunc);
	for (auto time : mFrameTimeArray)
	{
		writeFile << time << std::endl;
	}
	writeFile.close();
	mFrameTimeArray.clear();
	i++;
}

