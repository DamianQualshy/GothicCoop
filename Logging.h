#include <string>

#ifdef next_state
#pragma push_macro("next_state")
#undef next_state
#define GOTHICCOOP_RESTORE_NEXT_STATE
#endif

#ifndef FMT_UNICODE
#define FMT_UNICODE 0
#endif

#include <spdlog/spdlog.h>

#ifdef GOTHICCOOP_RESTORE_NEXT_STATE
#pragma pop_macro("next_state")
#undef GOTHICCOOP_RESTORE_NEXT_STATE
#endif

namespace Common {
    class CStringA;
}

namespace GOTHIC_ENGINE {
    void InitializeLogging(const std::string& logFilePath,
                           const std::string& logLevel,
                           bool enableConsole);

    void LogMessage(spdlog::level::level_enum level, const std::string& message);
    void LogMessage(spdlog::level::level_enum level, const Common::CStringA& message);
    void LogMessage(spdlog::level::level_enum level, const char* message);
    void LogRateLimited(spdlog::level::level_enum level,
                        const std::string& key,
                        const std::string& message,
                        long long intervalMs);
    void LogRateLimited(spdlog::level::level_enum level,
                        const std::string& key,
                        const Common::CStringA& message,
                        long long intervalMs);
    void LogRateLimited(spdlog::level::level_enum level,
                        const std::string& key,
                        const char* message,
                        long long intervalMs);

    spdlog::level::level_enum ParseLogLevel(const std::string& level);
}