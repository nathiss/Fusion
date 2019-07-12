/**
 * @file system_abstractions.hpp
 *
 * This module is a part of Fusion Server project.
 * It declares the common abstractions of the server.
 *
 * (c) 2019 by Kamil Rusin
 */


#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <fusion_server/package_parser.hpp>

namespace fusion_server {

/**
 * This is the forward declaration of the WebSocketSession class.
 */
class WebSocketSession;

namespace system_abstractions {

/**
 * This is the package type used in both WebSocket and HTTP sessions.
 */
using Package = std::shared_ptr<const std::string>;

/**
 * This function template is a wrapper for creating Package objects.
 *
 * @param[in] args
 *   Arguments passed to the used factory.
 *
 * @return
 *   The created Package object.
 */
template <typename... Args>
[[ nodiscard ]] Package make_Package(Args... args) noexcept {
  return std::make_shared<const std::string>(std::forward<Args>(args)...);
}

/**
 * This type is used to create delegates that will be called by WebSocket
 * sessions each time, when a new package arrives.
 *
 * @param[in] package
 *   The package received from the client.
 *
 * @param[in] session
 *   The session connected to the client.
 */
using IncommingPackageDelegate = std::function< void(const PackageParser::JSON&  package, WebSocketSession* session) >;

/**
 * @brief Logging pattern.
 * This constant is a default pattern for log lines, both in files and console.
 */
static constexpr const char* kLoggerPattern = "[%H:%M:%S:%e] [thread %t] [%^%l@%n%$] %v";

/**
 * @brief Creates a logger.
 * This function creates a logger and associates it with the given name.
 * If @p registrate is set to true, the new logger will be registered in global
 * registry.
 *
 * @tparam Sinks
 *   A pack of types that meet spdlog::sink_ptr concepts.
 *
 * @param logger_name
 *   The name of a new logger.
 *
 * @param registrate
 *   If true the logger will be registered in the global registry.
 *
 * @param sinks
 *   A pack of sinks that will be forwarded to the new logger object.
 *
 * @return
 *   A shared pointer to the new logger.
 *
 * @see [spdlog Sinks](https://github.com/gabime/spdlog/wiki/4.-Sinks)
 * @see [spdlog Logger registry](https://github.com/gabime/spdlog/wiki/5.-Logger-registry)
 */
template <typename ...Sinks>
auto CreateLogger(std::string logger_name, bool registrate, Sinks... sinks) noexcept {

#ifdef DEBUG
  auto console_log_level = spdlog::level::debug;
#else
  auto console_log_level = spdlog::level::warn;
#endif

  // TODO: read the path from config file
  std::string file_name{"logs/"};
  file_name += logger_name + ".log";

  auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(file_name, true);
  file_sink->set_level(console_log_level);
  file_sink->set_pattern(kLoggerPattern);

  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_level(console_log_level);
  console_sink->set_pattern(kLoggerPattern);

  spdlog::sinks_init_list sinks_list{file_sink, console_sink, std::forward<Sinks>(sinks)...};

  auto logger = std::make_shared<spdlog::logger>(logger_name, std::move(sinks_list));
  logger->set_level(spdlog::level::debug);
  logger->flush_on(console_log_level);

  if (registrate)
    spdlog::register_logger(logger);

  return logger;
}

}  // namespace system_abstractions

}  // namespace fusion_server