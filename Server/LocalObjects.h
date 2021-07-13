
#pragma once
#include <vector>
#include <mutex>

//local multithread object
template <typename T>
class ThreadSafeObject : public std::mutex
{
public:
	explicit constexpr ThreadSafeObject(T o) : anObj(o) {}
	inline void Set(T o) { anObj = o; }
	inline T Get() const { return anObj; }
private:
	constexpr ThreadSafeObject() = default;
	T anObj{};
};

class FileReceiver;
bool DispatchAppLayerMsg();
ServiceHeader::Command DispatchClientLayerMsg(FileReceiver* fr, unsigned int cid);
BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType);

ThreadSafeObject<bool> bAppExit(false);
ThreadSafeObject<int>  iThreadsCounter(0); // C - style is InterlockedIncrement