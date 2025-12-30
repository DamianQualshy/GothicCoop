#include <string>

#include <spdlog/spdlog.h>

namespace GOTHIC_ENGINE {
    void InitializeLogging(const std::string& logFilePath,
                           const std::string& logLevel,
                           bool enableConsole);

    void LogMessage(spdlog::level::level_enum level, const std::string& message);
    void LogRateLimited(spdlog::level::level_enum level,
                        const std::string& key,
                        const std::string& message,
                        long long intervalMs);

    spdlog::level::level_enum ParseLogLevel(const std::string& level);
}