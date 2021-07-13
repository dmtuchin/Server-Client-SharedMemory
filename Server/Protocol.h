#include <Windows.h>

#define SHARED_MEM_SIZE 256 * 1024
#define GUID_SHARED_MEMORY TEXT("d6022f20-1283-4158-b972-d5f59e295532")
#define GUID_KERNEL_MUTEX TEXT("daa4f9a1-3a05-4018-a6e7-50a3a9efdcf8")

// HDR is the ServiceHeader object
/* -------------------- Between Process Shared Memory ------------------ */
/* HDR------------------------------------------------------------------ */
class ServiceHeader
{
public:
    enum class Command
    {
        none = 0,   //
        name = 1,   // Buffer have a file name
        size,       // Size of file
        data,       // Buffer have part of the file
        complete,   // End of file
        disconnect, // close connection
        connect     // 
    };
    explicit constexpr ServiceHeader(ServiceHeader::Command cmd, unsigned int len, unsigned int id) : aCmd(cmd), aLen(len), anId(id) {};
    inline Command GetCommand() const { return aCmd; }
    inline unsigned int GetDataSize() const { return aLen; }
    inline unsigned int GetIdentity() const { return anId; }

private:
    Command  aCmd{}; // for 32/64 bit CPUs type int faster than byte
    unsigned int  aLen{}; // for 32/64 bit CPUs type int faster than word/byte
    unsigned int  anId{}; // PID of client
};