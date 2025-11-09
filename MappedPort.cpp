#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "mappedport.h"

namespace GOTHIC_ENGINE {
    MappedPort::MappedPort() {
        Initialize();
    }

    MappedPort::MappedPort(long lPort, wchar_t* protocol, wchar_t* description) {
        if (Initialize()) mapped_ = Map(lPort, protocol, description);
    }

    bool MappedPort::Initialize() {
        lport_ = 0;
        mapped_ = false;
        initialized_ = false;
        hr = 0;
        uPnP = NULL;
        spmOut = NULL;
        spmCol = NULL;
        CoInitialize(NULL);

        hr = CoCreateInstance(__uuidof(UPnPNAT), NULL, CLSCTX_ALL, __uuidof(IUPnPNAT), (void**)&uPnP);
        if (hr == S_OK && uPnP != NULL) {
            hr = uPnP->get_StaticPortMappingCollection(&spmCol);

            if (hr == S_OK && spmCol != NULL) {
                initialized_ = true;
            }
        }

        return initialized_;
    }

    MappedPort::~MappedPort() {
        if (mapped_) this->Unmap();
        if (uPnP) uPnP->Release();
        if (spmCol) spmCol->Release();
        if (spmOut) spmOut->Release();
        if (initialized_) CoUninitialize();
    }

    bool MappedPort::Map(long lPort, wchar_t* protocol, wchar_t* description) {
        WSADATA wsaData;
        char hostname[256];
        PHOSTENT hostinfo;
        wchar_t ip[16];
        lport_ = lPort;
        wcscpy_s(protocol_, 4, protocol);
        if ((initialized_ == true) && (mapped_ == false)) {
            spmCol->Remove(lport_, protocol_);
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0) {
                if (gethostname(hostname, sizeof(hostname)) == 0) {
                    hostinfo = gethostbyname(hostname);
                    if (hostinfo != NULL) {
                        if (InetNtopW(AF_INET, (struct in_addr*)*hostinfo->h_addr_list, ip, sizeof(ip)) != NULL) {
                            if (spmCol->Add(lPort, protocol, lPort, ip, TRUE, description, &spmOut) == S_OK) {
                                mapped_ = true;
                                ChatLog("(Server) UPnP port was added successfully.");
                            }
                        }
                    }
                }
                WSACleanup();
            }
        }
        return mapped_;
    }

    bool MappedPort::Unmap() {
        return (spmCol->Remove(lport_, protocol_) == S_OK);
    }

    bool MappedPort::IsMapped() {
        return mapped_;
    }
}