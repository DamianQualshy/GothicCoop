namespace GOTHIC_ENGINE {
    std::vector<std::string> lastProcessedPackages;

    const int LastExecutedFunctionAddressesMaxLimit = 5000;
    PBYTE LastExecutedFunctionAddresses[LastExecutedFunctionAddressesMaxLimit];
    int LastExecutedFunctionAddressesIndex = -1;
    bool TrackLastExecutedFunctions = true;

    extern "C" void __declspec(naked) __cdecl _penter(void) noexcept {
        __asm {
            push ebp;
            mov ebp, esp;
            sub esp, __LOCAL_SIZE;
            pushad;
        }

        if (TrackLastExecutedFunctions && MainThreadId == GetCurrentThreadId()) {
            PBYTE addr;
            addr = (PBYTE)_ReturnAddress();

            LastExecutedFunctionAddressesIndex += 1;
            LastExecutedFunctionAddresses[LastExecutedFunctionAddressesIndex] = addr;

            if (LastExecutedFunctionAddressesIndex == LastExecutedFunctionAddressesMaxLimit - 1) {
                LastExecutedFunctionAddressesIndex = 0;
            }
        }

        _asm {
            popad;
            mov esp, ebp;
            pop ebp;
            ret;
        }
    }

    void SaveErrorDetails() {
        TrackLastExecutedFunctions = false;

        CoopLog("State:\r");
        CoopLog(PluginState);
        CoopLog("\r");
        CoopLog("Last packages:");
        for (auto data : lastProcessedPackages) {
            CoopLog(data);
            CoopLog("\r");
        }
        CoopLog("\r");

        HANDLE process;
        process = GetCurrentProcess();
        DWORD64 dllBase = (DWORD64)GetModuleHandleA("GothicCoop.dll");

        std::vector<std::string> lastMethodCalls;
        std::vector<std::string> lastCoreMethodCalls;

        for (int i = 1; i <= LastExecutedFunctionAddressesMaxLimit; i++) {
            int currentFuncIndex = LastExecutedFunctionAddressesIndex + i;
            if (currentFuncIndex > LastExecutedFunctionAddressesMaxLimit - 1) {
                currentFuncIndex = LastExecutedFunctionAddressesMaxLimit - (i + LastExecutedFunctionAddressesIndex);
                if (currentFuncIndex < 0) {
                    currentFuncIndex = -currentFuncIndex;
                }
            }

            DWORD64 dwDisplacement = 0;
            DWORD64 dwAddress = (DWORD64)(LastExecutedFunctionAddresses[currentFuncIndex]);

            char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
            PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
            pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
            pSymbol->MaxNameLen = MAX_SYM_NAME;

            if (SymFromAddr(process, dwAddress, &dwDisplacement, pSymbol))
            {
                CoopLog(pSymbol->Name);
                CoopLog("\r");
                if (i >= LastExecutedFunctionAddressesMaxLimit - 11 && i != LastExecutedFunctionAddressesMaxLimit) {
                    lastMethodCalls.push_back(pSymbol->Name);
                }
                
                auto symbolName = std::string(pSymbol->Name);
                if (symbolName.rfind("Gothic_", 0) == 0 && i != LastExecutedFunctionAddressesMaxLimit) {
                    lastCoreMethodCalls.push_back(symbolName);
                }
            }
            else
            {
                auto log = string::Combine("GothicCoop.dll+%i\n", dwAddress - dllBase).ToChar();
                CoopLog(log);
                if (i >= LastExecutedFunctionAddressesMaxLimit - 11 && i != LastExecutedFunctionAddressesMaxLimit) {
                    lastMethodCalls.push_back(log);
                }
            }
        }

        std::vector<std::string> last10CoreMethodCalls(lastCoreMethodCalls.end() - 10, lastCoreMethodCalls.end());

        if (GameChat) {
            if (!GameChat->IsShowing()) {
                GameChat->ToggleShowing();
            }

            GameChat->Clear();
            ChatLog(string::Combine("[GothicCoop] Error (v. %i):", COOP_VERSION), zCOLOR(230, 0, 0, 255));

            if (PluginState.compare("") != 0) {
                ChatLog("State:");
                ChatLog(PluginState.c_str());
            }

            ChatLog("Packages:");
            for (auto data : lastProcessedPackages) {
                ChatLog(data.c_str());
            }

            ChatLog("Calls:");
            for (const auto& piece : lastMethodCalls) {
                ChatLog(piece.c_str());
            }

            ChatLog("Core calls:");
            for (const auto& piece : last10CoreMethodCalls) {
                ChatLog(piece.c_str());
            }

            ChatLog("Please save and rehost the game. You can also try to continue playing the game if it is stable.");
            ChatLog("You can hide the error by pressing P (by default).");
        }

        TrackLastExecutedFunctions = true;
    }

    void SaveNetworkPacket(const char* data) {
        if (lastProcessedPackages.size() < 10) {
            lastProcessedPackages.push_back(std::string(data));
        }
        else
        {
            lastProcessedPackages.erase(lastProcessedPackages.begin());
            lastProcessedPackages.push_back(std::string(data));
        }
    }
}