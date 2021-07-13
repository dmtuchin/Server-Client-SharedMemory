// Client.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <windows.h>
#include "krnl_mutex.h"
#include "Protocol.h"
#include "LocalObjects.h"
#include "ConfigMaster/ConfigMaster.h"
#include "MultiProcessMemory.h"

// multiprocess sync objects
MultiProcessMemory aSharedMemory;

int main()
{
    ReadConfig("config.txt");
    // register handler WM_CLOSE for console application
    SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);

    krnl_mutex aKernelMutex;
    bool bAppShutdown = false;
    bool bServerShutdown = false;

    bServerShutdown = SendCommand(ServiceHeader::Command::connect, 0, nullptr);
    std::cout << "My Id# " << aPID << std::endl;

    if (!bServerShutdown)
    {
        std::cout << "Connected to Server." << std::endl;
        do {
            aConfig::aTask aTask;
            if (theConfig.allTasks.size() > 0)
            {
                aTask = theConfig.allTasks.at(0);
                std::cout << "Sleep for " << aTask.iTime_milliSec << " ms." << std::endl;
                SleepEx(aTask.iTime_milliSec, TRUE);
                std::cout << "Transferring " << aTask.szFilename << std::endl;
            }
            // open kernel sync object
            aKernelMutex.attach(GUID_KERNEL_MUTEX);
            if (aKernelMutex.handle())
            {
                if (aTask.szFilename[0] != 0x00)
                {
                    bServerShutdown = SendFile(aTask.szFilename);
                    // removing block which was sent
                    theConfig.allTasks.erase(theConfig.allTasks.begin());
                } else // have not tasks
                    SleepEx(10, TRUE);
                
                aKernelMutex.detach();

                // exit application sign
                bAppExit.lock();
                bAppShutdown = bAppExit.Get();
                bAppExit.unlock();
            }
            else
                bServerShutdown = true;

        } while (!bAppShutdown && !bServerShutdown);
    }
    else {
        std::cout << "Could not connect to the Server." << std::endl;
    }
    std::cout << bAppShutdown ? "Bye." : "Server shutdown. Bye.";
    std::cout << std::endl;

    return 0;
}

bool SendCommand(ServiceHeader::Command cmd, unsigned int len, const char* bytes)
{
    bool bDetachKrnlMutex = false;
    bool bServerShutdown = false;
    ServiceHeader hdr(cmd, len, aPID);
    constexpr int data_size = SHARED_MEM_SIZE - sizeof(ServiceHeader);

    if (len <= data_size)
    {
        bool bSent = false;
        std::unique_ptr<krnl_mutex> pKernelMutex = std::make_unique<krnl_mutex>();
        try {
            do {
                // open kernel sync object
                pKernelMutex->attach(GUID_KERNEL_MUTEX);
                if (pKernelMutex->handle())
                {
                    bDetachKrnlMutex = true;
                    pKernelMutex->lock();
                    aSharedMemory.OpenSharedMemory();
                    if (aSharedMemory.IsOpened())
                    {
                        ServiceHeader* pSrvHdr = (ServiceHeader*)(aSharedMemory.GetPointer());
                        if (pSrvHdr->GetIdentity() == 0) // Server is ready for receive
                        {
                            std::memcpy((ServiceHeader*)(aSharedMemory.GetPointer()), &hdr, sizeof ServiceHeader);
                            if (bytes != nullptr && len)
                                std::memcpy((char*)((char*)(aSharedMemory.GetPointer()) + sizeof ServiceHeader), bytes, len);
                            bSent = true;
                        } // otherwise wait time for server ready for incomming messages
                        else { // waiting my queue or Server has been turned off
                            SleepEx(10, TRUE);
                        }
                        pKernelMutex->unlock();
                        pKernelMutex->detach();
                        bDetachKrnlMutex = false;
                        aSharedMemory.CloseSharedMemory();
                    }
                    else
                    {// shared memory destoyed by Server
                        bServerShutdown = true; break;
                    }
                }
                else
                {// kernel mutex destoyed by Server
                    bServerShutdown = true; break;
                }
            } while (!bSent && !bServerShutdown);
        }
        catch(...)
        {// Server has been turned off.
            bServerShutdown = true;
        }
        if(bDetachKrnlMutex)
            pKernelMutex->detach();

    }
    return bServerShutdown;
}

BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_CLOSE_EVENT)
    {
        // here wait & exit main process thread
        SendCommand(ServiceHeader::Command::disconnect, 0, nullptr);
        SleepEx(100, TRUE);

        // exit application sign
        bAppExit.lock();
        bAppExit.Set(true);
        bAppExit.unlock();

        SleepEx(100, TRUE);
        return TRUE;
    }

    return FALSE;
}

bool SendFile(const std::string& file)
{
    bool bServerShutdown = false;
    constexpr const int data_size = SHARED_MEM_SIZE - sizeof(ServiceHeader);
    std::unique_ptr<char[]> local_buf (new char[SHARED_MEM_SIZE]);
    
    // splitting to pure file name and its path
    std::string path = "", name = "";
    size_t iDelimiter = file.find_last_of('\\');
    if (iDelimiter == -1) iDelimiter = file.find_last_of('/');
    if (iDelimiter >= 0)
    {// complex path
        path = file.substr(0, iDelimiter + 1);
        name = file.substr(iDelimiter + 1);
    }
    else name = file;
    // send file name at first
    ServiceHeader hdr(ServiceHeader::Command::name, name.length(), aPID);

    FILE* f; // C - style reading, because we need transfer low lovel buffer part by part.
    // At this  moment, C - style is comfortable
    errno_t err = fopen_s(&f, std::string(path + name).c_str(), "rb");
    if (err == 0)
    {
        // send first packet with file name
        bServerShutdown = SendCommand(ServiceHeader::Command::name, name.length() + 1/*with zero symbol*/, name.c_str());
        //
        // send second packet with file size
        fseek(f, 0L, SEEK_END);
        long lFilesize = ftell(f); rewind(f);
        *(long*)local_buf.get() = lFilesize;
        bServerShutdown = SendCommand(ServiceHeader::Command::size, sizeof(lFilesize), local_buf.get());
        //
        int iReads = data_size;
        while (!bServerShutdown && iReads == data_size)
        {
            iReads = fread(local_buf.get(), 1, data_size, f);
            bServerShutdown = SendCommand(ServiceHeader::Command::data, iReads, local_buf.get());
            SleepEx(10, TRUE);
        }
        bServerShutdown = SendCommand(ServiceHeader::Command::complete, 0, nullptr);

        std::cout << "File has been transferred." << std::endl;

        fclose(f);
    }
    else std::cout << "Cant open the file." << std::endl;

    return bServerShutdown;
}

void ReadConfig(const char* pszFilename)
{
    // Reading config file
    aConfig::aTask oneTask;
    ConfigMaster cfg(pszFilename);
    ConfigMaster::ConfigItem anTimeItem(ConfigMaster::CONFIGTYPE::TheInteger, &oneTask.iTime_milliSec, "WAIT_TIME");
    cfg.AssociateItem(&anTimeItem);
    ConfigMaster::ConfigItem anFileItem(ConfigMaster::ConfigItem(ConfigMaster::CONFIGTYPE::TheString, oneTask.szFilename, "FILE"));
    cfg.AssociateItem(&anFileItem);
    
    while (cfg.ReadBlock())
    {
        theConfig.allTasks.push_back(oneTask);
    }
    if (theConfig.allTasks.size() == 0)
        std::cout << "config file is empty or not found" << std::endl;
}