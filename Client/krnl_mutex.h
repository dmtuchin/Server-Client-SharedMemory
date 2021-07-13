#pragma once
#ifdef _DEBUG
#include <crtdbg.h>
#define ASSERT(expression) _ASSERTE(expression)
#define VERIFY(expression) ASSERT(expression)
#define VERIFY_(expected, expression) ASSERT(expected == expression)
#else
#define ASSERT(expression) ((void)0)
#define VERIFY(expression) (expression)
#define VERIFY_(expected, expression) (expression)
#endif

#include <string>
#include <synchapi.h>
#include <handleapi.h>
class krnl_mutex
{
	HANDLE h{};
	krnl_mutex(krnl_mutex const&);
	krnl_mutex const& operator = (krnl_mutex const& obj) { h = obj.h; return *this; };
public:
	constexpr krnl_mutex() = default;
	explicit krnl_mutex(std::string krnl_mutex_name) :
		h(CreateMutex(nullptr, false, krnl_mutex_name.c_str()))
	{
		;
	}
	~krnl_mutex()
	{
		if(h)
			CloseHandle(h);
	}
	bool attach(const char* krnl_mutex_name)
	{
		if (!h)
			h = OpenMutex(MUTEX_ALL_ACCESS, false, krnl_mutex_name);

		return h;
	}
	void detach()
	{
		VERIFY(CloseHandle(h));
		h = 0x00;
	}
	void lock()
	{
		VERIFY_(WAIT_OBJECT_0, WaitForSingleObject(h, INFINITE));
	}
	bool try_lock()
	{
		return WAIT_OBJECT_0 == WaitForSingleObject(h, 0);
	}
	void unlock()
	{
		VERIFY(ReleaseMutex(h));
	}
	HANDLE handle()
	{
		return h;
	}
};