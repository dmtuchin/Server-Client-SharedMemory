// Client listener server side
#pragma once
#include <fstream>
#include <memory.h>

class FileReceiver
{
public:
    explicit FileReceiver(char* pGlobalBuffer, const int iBufferSize) : pBuffer(pGlobalBuffer), iBuffSize(iBufferSize)
    {
        if (nullptr == pBuffer && iBufferSize > 0)
        {
            bLocalBuffer = true;
            pBuffer = new char[iBufferSize]();
            ClearBuffer();
        }
    };
    
    ~FileReceiver() {
        if (uf) fclose(uf);
        if(bLocalBuffer && pBuffer) delete pBuffer;
    };

    inline void ClearBuffer() {memset(pBuffer, 0, iBuffSize);}; 
    inline char* GetBufferPtr() const { return pBuffer;};
    void DispatchBuffer(unsigned int iReceivedBytes);

private:
    bool bLocalBuffer{};
    std::ifstream upload{}; // file, which will be upload from the current client
    FILE* uf{};// upload file
    long iBytesReceived{}; // counter bytes of the session
    char* pBuffer{};
    int  iBuffSize{};

    void DispatchFileName(const char* pszName);
    void DispatchFileSize(const char* pData);
    void DispatchData(const char* pData);
    void DispatchComplete();
};