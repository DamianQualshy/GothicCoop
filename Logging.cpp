#include <algorithm>
#include <chrono>
#include <cctype>
#include <map>
#include <mutex>
#include <vector>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "Logging.h"

namespace GOTHIC_ENGINE {
    namespace {
        struct RateLimitState {
            std::chrono::steady_clock::time_point lastLog{};
            std::size_t suppressed = 0;
        };

        std::mutex g_logMutex;
        std::map<std::string, RateLimitState> g_rateLimits;

        std::string ToLower(std::string value) {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return value;
        }
    }

    spdlog::level::level_enum ParseLogLevel(const std::string& level) {
        const auto normalized = ToLower(level);
        if (normalized == "trace") {
            return spdlog::level::trace;
        }
        if (normalized == "debug") {
            return spdlog::level::debug;
        }
        if (normalized == "warn" || normalized == "warning") {
            return spdlog::level::warn;
        }
        if (normalized == "error") {
            return spdlog::level::err;
        }
        if (normalized == "critical") {
            return spdlog::level::critical;
        }
        if (normalized == "off") {
            return spdlog::level::off;
        }
        return spdlog::level::info;
    }

    void InitializeLogging(const std::string& logFilePath,
                           const std::string& logLevel,
                           bool enableConsole) {
        std::lock_guard<std::mutex> lock(g_logMutex);

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, true));
        if (enableConsole) {
            sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        }

        auto logger = std::make_shared<spdlog::logger>("gothic_coop", sinks.begin(), sinks.end());
        auto level = ParseLogLevel(logLevel);
        logger->set_level(level);
        logger->flush_on(spdlog::level::info);
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

        spdlog::set_default_logger(logger);
    }

    void LogMessage(spdlog::level::level_enum level, const std::string& message) {
        auto logger = spdlog::default_logger();
        if (!logger) {
            return;
        }
        logger->log(level, "{}", message);
    }

    void LogRateLimited(spdlog::level::level_enum level,
                        const std::string& key,
                        const std::string& message,
                        long long intervalMs) {
        auto logger = spdlog::default_logger();
        if (!logger) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        std::size_t suppressed = 0;
        {
            std::lock_guard<std::mutex> lock(g_logMutex);
            auto& state = g_rateLimits[key];
            const auto elapsed = now - state.lastLog;
            if (state.lastLog.time_since_epoch().count() != 0
                && elapsed < std::chrono::milliseconds(intervalMs)) {
                state.suppressed++;
                return;
            }
            suppressed = state.suppressed;
            state.suppressed = 0;
            state.lastLog = now;
        }

        if (suppressed > 0) {
            logger->log(level, "{} (suppressed {} similar messages)", message, suppressed);
        }
        else {
            logger->log(level, "{}", message);
        }
    }
}