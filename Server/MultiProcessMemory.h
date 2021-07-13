
class MultiProcessMemory
{
public:
	constexpr MultiProcessMemory() = default;
	//server mode operation
	void CreateSharedMemory()
	{
		// create shared memory object
		LARGE_INTEGER size;
		size.QuadPart = SHARED_MEM_SIZE;

		hFileMapping = CreateFileMapping(INVALID_HANDLE_VALUE,
			nullptr, PAGE_READWRITE, size.HighPart, size.LowPart, GUID_SHARED_MEMORY);

		if (hFileMapping)
		{
			pShMem = MapViewOfFile(hFileMapping,
				FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
		}
	}
	// client mode operation
	void OpenSharedMemory()
	{
		// open shared memory object
		if (pShMem == nullptr)
		{
			hFileMapping = OpenFileMapping(
				FILE_MAP_READ | FILE_MAP_WRITE, FALSE, GUID_SHARED_MEMORY);

			if (hFileMapping)
			{
				pShMem = MapViewOfFile(hFileMapping,
					FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
			}
		}
	}
	// client side operation
	void CloseSharedMemory()
	{
		UnmapViewOfFile(pShMem); pShMem = nullptr;
		CloseHandle(hFileMapping); hFileMapping = nullptr;
	}
	inline LPVOID GetPointer() const
	{
		return pShMem;
	}
	~MultiProcessMemory()
	{
		UnmapViewOfFile(pShMem);
		CloseHandle(hFileMapping);
	}
private:
	// multiprocess sync objects
	LPVOID pShMem = nullptr;
	HANDLE hFileMapping = nullptr;
};