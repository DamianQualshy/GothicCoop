#ifndef INC_MAPPEDPORT_H
#define INC_MAPPEDPORT_H
#pragma comment (lib, "ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Natupnp.h>
#include <Windows.h>

namespace GOTHIC_ENGINE {
    class MappedPort {
    private:
        HRESULT hr;
        wchar_t protocol_[4];
        long lport_;
        bool initialized_;
        bool mapped_;
        IUPnPNAT* uPnP;
        IStaticPortMapping* spmOut;
        IStaticPortMappingCollection* spmCol;
        bool Initialize();

    public:
        MappedPort();
        MappedPort(long lPort, wchar_t* protocol, wchar_t* description);	~MappedPort();
        bool Map(long lPort, wchar_t* protocol, wchar_t* description);
        bool Unmap();
        bool IsMapped();
    };
}
#endif