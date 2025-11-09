#include "UnionAfx.h"
#include <crtversion.h>

extern "C"

void EnableDraggableWindow() {
    *(int*)0x008D4234 = 1;
}

int __stdcall DllMain(HPLUGIN hModule, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        Union.DefineCRTVersion(_VC_CRT_MAJOR_VERSION, _VC_CRT_MINOR_VERSION, _VC_CRT_BUILD_VERSION, _VC_CRT_RBUILD_VERSION);
#ifdef _DEBUG
        EnableDraggableWindow();
#endif
    }
    if (fdwReason == DLL_PROCESS_DETACH)
    {

    }
    return True;
}