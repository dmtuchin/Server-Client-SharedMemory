// Server side: Data receiver (file)
#pragma once
#include "protocol.h"
#include "FileReceiver.hpp"

#include <mutex>
#include <iostream>
#include <filesystem>

void FileReceiver:: DispatchBuffer(unsigned int iReceivedBytes) 
{
    const ServiceHeader* hdr = (ServiceHeader*)pBuffer;
    switch (hdr->GetCommand())
    {
    case ServiceHeader::Command::name:
        DispatchFileName(pBuffer);
        break;
    case ServiceHeader::Command::size:
        DispatchFileSize(pBuffer);
        break;
    case ServiceHeader::Command::data:
        DispatchData(pBuffer);
        break;
    case ServiceHeader::Command::complete:
        DispatchComplete();
        break;
    default:
        break;
    }
};
void FileReceiver::DispatchFileName(const char* pData)  
{
    std::string path("");
    const ServiceHeader* hdr = (ServiceHeader*)pData;

    // at this situation, real file name starts at +sizeof(hdr) bytes after begin of the receiver's buffer
    std::string fn(&pData[sizeof(ServiceHeader)]);

    // C - style 
    errno_t err = fopen_s(&uf, std::string(path + fn).c_str(), "wb");
    if(err == 0)
    {
        iBytesReceived += hdr->GetDataSize();
        std::cout << "Receive file into: " << path << "/" << fn << std::endl;
    }
}

// Prevent Disk overflow
void FileReceiver::DispatchFileSize(const char* pData)
{
    const ServiceHeader* hdr = (ServiceHeader*)pData;
    if (sizeof(long) == hdr->GetDataSize())
    {// type size is correct
        long lFileSz = *(long*)pData;

        // C++17
        try {
            using namespace std::filesystem;
            std::error_code ec;
            path aPath = current_path();
            const space_info si = space(aPath, ec);
            if (si.available < lFileSz)
            {// Disk full or received file is too large
                std::cout << "Error: Disk full or receiving file is too large. Skipping" << std::endl;
                DispatchComplete();
            }
        }
        catch (std::filesystem::filesystem_error const& ex) {
            std::cout 
                << "what():  " << ex.what() << '\n'
                << "path1(): " << ex.path1() << '\n'
                << "path2(): " << ex.path2() << '\n'
                << "code().value():    " << ex.code().value() << '\n'
                << "code().message():  " << ex.code().message() << '\n'
                << "code().category(): " << ex.code().category().name() << '\n';
        }
    }
}
void FileReceiver::DispatchData(const char* pData)
{
    const ServiceHeader* hdr = (ServiceHeader*)pData;
    if (uf != nullptr)
    {
        fwrite(&pData[sizeof(ServiceHeader)], 1, hdr->GetDataSize(), uf);
        if (ferror(uf))
        {
            std::cout << "Write error. Close session." << std::endl;
            DispatchComplete();
        } else 
            iBytesReceived += hdr->GetDataSize();
    }
}
void FileReceiver::DispatchComplete()
{
    if (uf != nullptr)
    {
        fclose(uf);
        std::cout << "Session size: " << iBytesReceived << " Bytes. " << std::endl << "Done." << std::endl;
        iBytesReceived = 0;
        uf = nullptr;
    }
}