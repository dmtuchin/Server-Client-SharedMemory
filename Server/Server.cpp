// Server.cpp
//

#include <iostream>
#include <stdlib.h>
#include <map>
#include <mutex>
#include <thread>

#include "krnl_mutex.h"
#include "Protocol.h"
#include "MultiProcessMemory.h"
#include "LocalObjects.h"
#include "FileReceiver.hpp"

std::unique_ptr<krnl_mutex> pKernelMutex;
MultiProcessMemory aSharedMemory;

// server thread
void client_connected_thread(unsigned int ucid/*client id*/)
{
    int readed = 0;

    iThreadsCounter.lock();
    iThreadsCounter.Set(iThreadsCounter.Get() + 1);
    iThreadsCounter.unlock();

    pKernelMutex->lock();
    std::unique_ptr<FileReceiver> fr = std::make_unique<FileReceiver>((char*)(aSharedMemory.GetPointer()), SHARED_MEM_SIZE);
    pKernelMutex->unlock();

    bool bShutdown = false;
    ServiceHeader::Command aCmd = ServiceHeader::Command::none;
    do
    {
        
        if (pKernelMutex->handle())
        {
            aCmd = DispatchClientLayerMsg(fr.get(), ucid);

			if (aCmd == ServiceHeader::Command::none)
				SleepEx(10, TRUE); // have no messages, waiting
			else if (aCmd == ServiceHeader::Command::disconnect)
				break; // exit thread if client disconnected
		}
        // exit application sign
        bAppExit.lock();
        bShutdown = bAppExit.Get();
        bAppExit.unlock();
        
    } while (!bShutdown);

    fflush(stdout);

    iThreadsCounter.lock();
    iThreadsCounter.Set(iThreadsCounter.Get() - 1);
    iThreadsCounter.unlock();
}
int main()
{
    pKernelMutex = std::make_unique<krnl_mutex>(GUID_KERNEL_MUTEX);
    // register handler WM_CLOSE for console application
    SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);

    aSharedMemory.CreateSharedMemory();
    if (aSharedMemory.GetPointer() && pKernelMutex->handle())
    {
        std::cout << "Server created. Waiting for a connection..." << std::endl;
        bool bShutdown = false;
        do {
            pKernelMutex->lock();
            if (!DispatchAppLayerMsg())
                SleepEx(10, TRUE); // have no messages, waiting

            pKernelMutex->unlock();

            // exit application sign
            bAppExit.lock();
            bShutdown = bAppExit.Get();
            bAppExit.unlock();
        } while (!bShutdown);
    }
    else std::cout << "Error: Could not create the Server.";

    //Destroy Server
    // wait for exit all threads! and after it delete global mutex
    int iWaitingTreadsExit = 0;
    do {
        iThreadsCounter.lock();
        iWaitingTreadsExit =  iThreadsCounter.Get();
        iThreadsCounter.unlock();
    } while (iWaitingTreadsExit);

    return 0;    
}

// return not zero if have some message
bool DispatchAppLayerMsg()
{
    bool bHaveMessage = false;
    ServiceHeader::Command aCmd = ServiceHeader::Command::none;
    pKernelMutex->lock();
    ServiceHeader* pSrvHdr = (ServiceHeader*)(aSharedMemory.GetPointer());
    if (pSrvHdr->GetIdentity() != NULL)
    {
        switch (aCmd = pSrvHdr->GetCommand())
        {
        case ServiceHeader::Command::connect:
            std::cout << "Connection accepted: id# " << pSrvHdr->GetIdentity() << std::endl;
            std::thread t(client_connected_thread, pSrvHdr->GetIdentity());
            t.detach();
            memset(pSrvHdr, 0, sizeof(ServiceHeader));
            break;
        }
    }
    pKernelMutex->unlock();
    return bool(pSrvHdr->GetCommand()); // 
}

// return not zero if have some message
ServiceHeader::Command DispatchClientLayerMsg(FileReceiver* fr, unsigned int ucid/*client id*/)
{
    bool bHaveMessage = false;
    ServiceHeader::Command aCmd = ServiceHeader::Command::none;
    auto resethdr = [=](ServiceHeader* pSrvHdr) {memset(pSrvHdr, 0, sizeof(ServiceHeader)); };

    pKernelMutex->lock();
    ServiceHeader* pSrvHdr = (ServiceHeader*)(aSharedMemory.GetPointer());
    if (pSrvHdr->GetIdentity() != NULL && pSrvHdr->GetIdentity() == ucid)
    {
        switch (aCmd = pSrvHdr->GetCommand())
        {
        case ServiceHeader::Command::disconnect:
            std::cout << "id# " << pSrvHdr->GetIdentity() << " Disconnected" << std::endl;
            break;
        case ServiceHeader::Command::name:
            std::cout << "Filename accepted: id# " << pSrvHdr->GetIdentity() << std::endl;
        case ServiceHeader::Command::size:
        case ServiceHeader::Command::data:
        case ServiceHeader::Command::complete:
            fr->DispatchBuffer(SHARED_MEM_SIZE);
            break;
        default:
            std::cout << "id# " << pSrvHdr->GetIdentity() << " Error: Unexpected message" << std::endl;
        }
        resethdr(pSrvHdr);
    }
    pKernelMutex->unlock();
    return aCmd; // 
}

BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_CLOSE_EVENT)
    {
        // here wait & exit main process thread
         // exit application sign
        bAppExit.lock();
        bAppExit.Set(true);
        bAppExit.unlock();

        // wait for exit all threads! and after it delete global mutex
        int iWaitingTreadsExit = 0;
        do {
            iThreadsCounter.lock();
            iWaitingTreadsExit = iThreadsCounter.Get();
            iThreadsCounter.unlock();
        } while (iWaitingTreadsExit);

        // Next Sleep waits for release all resources in the main() thread
        SleepEx(500, TRUE);
        
        // exit application
        return TRUE;
    }

    return FALSE;
}