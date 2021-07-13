#pragma once
#include <mutex>
#include <vector>
#include <string>

// maximum filepath length
#define DIRECTORY_NAME_SIZE 512

//local multithread object
template <typename T>
class ThreadSafeObject : public std::mutex
{
public:
	constexpr ThreadSafeObject() = default;
	explicit constexpr ThreadSafeObject(T o) : anObj(o) {}
	inline void Set(T o) { anObj = o; }
	inline T Get() const { return anObj; }
private:
	T anObj{};
};

struct aConfig
{
	typedef struct aTask
	{
		constexpr aTask() = default;
		aTask(const aTask& o) : iTime_milliSec(o.iTime_milliSec)
		{
			strcpy_s(szFilename, o.szFilename);
		};
		aTask(const aTask&& o) noexcept : iTime_milliSec(o.iTime_milliSec)
		{
			strcpy_s(szFilename, o.szFilename);
		};
		aTask& operator = (const aTask& o)
		{
			iTime_milliSec = o.iTime_milliSec;
			strcpy_s(szFilename, o.szFilename);
			return *this;
		};
		int iTime_milliSec{};
		char szFilename[DIRECTORY_NAME_SIZE]{};
	};
	std::vector<aTask> allTasks;
};


// client PID as an identifier
DWORD aPID = GetCurrentProcessId();

//config file
aConfig theConfig;

// exit application flag
ThreadSafeObject<bool> bAppExit(false);

void ReadConfig(const char* pszCfgFile);
bool SendFile(const std::string& file);
bool SendCommand(ServiceHeader::Command cmd, unsigned int len, const char* bytes);
BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType);