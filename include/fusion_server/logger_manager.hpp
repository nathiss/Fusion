/**
 * @file logger_manager.hpp
 *
 * This module is a part of Fusion Server project.
 * It contains the declaration of the LoggerManager class and functions used to
 * print "user types" by [{fmt}](https://github.com/fmtlib/fmt) library.
 *
 * Copyright 2019 Kamil Rusin
 */

#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/logic/tribool.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <fusion_server/json.hpp>

namespace fusion_server {

/**
 * This class represents the logger manager. It provides methods for creation
 * new loggers and for setting the default parameters for new loggers.
 */
class LoggerManager {
 public:
  /**
   * This is the type of loggers used in this program.
   */
  using Logger = std::shared_ptr<spdlog::logger>;

  /**
   * This enum contains all possible logging levels.
   */
  enum class Level {
    /**
     * @brief The trace logging level.
     */
    trace,

    /**
     * @brief The debug logging level.
     */
    debug,

    /**
     * @brief The info logging level.
     */
    info,

    /**
     * @brief The warning logging level.
     */
    warn,

    /**
     * @brief The error logging level.
     */
    error,

    /**
     * @brief The critical logging level.
     */
    critical,

    /**
     * This value indicates that the logging level of a new logger should be
     * the default one.
     */
    none,
  };

  /**
   * This constructor initialises the options to the default values.
   */
  LoggerManager() noexcept;

  /**
   * @brief Configures the object.
   * This method configures this object using the given config JSON object.
   * If the configuration was unsuccessful or it's not complete the method
   * returns false.
   *
   * @param[in] config
   *   A JSON object used to configure this instance.
   *
   * @return true
   *   The object has been successfully configured.
   *
   * @return false
   *   Configuration failed. The config object was not complete or corrupted.
   *
   * @note
   *   If the method fails, it is guaranteed that default config will be used
   *   for creating a new logger.
   */
  bool Configure(const json::JSON& config) noexcept;

  /**
   * @brief Creates a new logger.
   * This method creates a new logger. Each new logger has two sinks: a console
   * and a file.
   *
   * @tparam MakeThreadSafe
   *   This indicates whether or not a new logger should be thread-safe. If the
   *   value is unspecified, the default is used.
   *
   * @param[in] name
   *   The name of a new logger.
   *
   * @param[in] level
   *   The output level of a new logger.
   *
   * @param[in] register_as_global
   *   This indicates whether or not a new logger should be registered in the
   *   global registry. If the value is unspecified, the default is used.
   *
   * @param[in] pattern
   *   This is a output pattern for a new logger.
   *   @see [spdlog wiki: Custom formatting](https://github.com/gabime/spdlog/wiki/3.-Custom-formatting)
   *
   * @return
   *   A new logger is returned.
   */
  template <bool MakeThreadSafe = false>
  std::shared_ptr<spdlog::logger> CreateLogger(
    const std::string& name,
    Level level = Level::none,
    boost::logic::tribool register_as_global = boost::logic::tribool::indeterminate_value,
    const std::string& pattern = {}
    ) noexcept;

  /**
   * @brief Returns requested logger from the global registry.
   * This method returns the requested logger from the global registry. If no
   * name is specified, it returns the default logger. If a logger cannot be
   * found it returns nullptr.
   *
   * @param name
   *   A name of the requested logger.
   *
   * @return
   *   A requested logger.
   */
  static std::shared_ptr<spdlog::logger> Get(const std::string& name = {}) noexcept;

 private:
  /**
   * This method sets all underlying fields to the default values.
   */
  void SetDefault() noexcept;

  /**
   * This method returns a concatenated log root directory, requested logger
   * name and the extension.
   *
   * @param[in] file_name
   *   The filename of a new logger.
   *
   * @return
   *    A concatenated log root directory, requested logger name and the
   *    extension is returned.
   */
  [[nodiscard]] std::string AssembleFileName(std::string file_name) const noexcept;

  /**
   * The path to the directory where logs will be stored.
   */
  std::string root_;

  /**
 * @brief Logging pattern.
 * This is the default pattern for log lines, both in files and consoles.
 */
  std::string logger_pattern_;

  /**
   * This is the default extension for all log files.
   */
  std::string extension_;

  /**
   * This is the default level of logging for all new loggers.
   */
  Level level_;

  /**
   * This flag indicates whether or not new logger should be registered in the
   * global registry by default.
   */
  bool register_by_default_ ;
};

namespace {

/**
 * This free function converts the given log level to a level of the used
 * logging library.
 *
 * @param[in] level
 *   A logging level.
 *
 * @return
 *   A logging level of the native logging library.
 */
spdlog::level::level_enum LogLevelToNative(LoggerManager::Level level) noexcept {
  switch (level) {
    case decltype(level)::trace:
      return spdlog::level::trace;
    case decltype(level)::debug:
      return spdlog::level::debug;
    case decltype(level)::info:
      return spdlog::level::info;
    case decltype(level)::warn:
      return spdlog::level::warn;
    case decltype(level)::error:
      return spdlog::level::err;
    case decltype(level)::critical:
      return spdlog::level::critical;
    default:
      return spdlog::level::off;
  }
}

}

template <bool MakeThreadSafe>
std::shared_ptr<spdlog::logger> LoggerManager::CreateLogger(
  const std::string& name, LoggerManager::Level level,
  boost::logic::tribool register_as_global , const std::string& pattern) noexcept {
  auto filename = LoggerManager::AssembleFileName(name);

  if constexpr (MakeThreadSafe) {
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    file_sink->set_level(
      level == decltype(level)::none ? LogLevelToNative(level_) :
    LogLevelToNative(level));
    file_sink->set_pattern(pattern.empty() ? logger_pattern_ : pattern);

    console_sink->set_level(
      level == decltype(level)::none ? LogLevelToNative(level_) :
    LogLevelToNative(level));
    console_sink->set_pattern(pattern.empty() ? logger_pattern_ : pattern);

    spdlog::sinks_init_list sinks_list{file_sink, console_sink};

    auto logger = std::make_shared<spdlog::logger>(name, sinks_list);
    logger->set_level(
      level == decltype(level)::none ? LogLevelToNative(level_) :
    LogLevelToNative(level));

    if (register_as_global == decltype(register_as_global)::indeterminate_value ?
    register_by_default_ : register_as_global) {
      spdlog::register_logger(logger);
  }

  return logger;
  } else {
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_st>(filename, true);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();

    file_sink->set_level(
      level == decltype(level)::none ? LogLevelToNative(level_) :
      LogLevelToNative(level));
    file_sink->set_pattern(pattern.empty() ? logger_pattern_ : pattern);

    console_sink->set_level(
      level == decltype(level)::none ? LogLevelToNative(level_) :
      LogLevelToNative(level));
    console_sink->set_pattern(pattern.empty() ? logger_pattern_ : pattern);

    spdlog::sinks_init_list sinks_list{file_sink, console_sink};

    auto logger = std::make_shared<spdlog::logger>(name, sinks_list);
    logger->set_level(
      level == decltype(level)::none ? LogLevelToNative(level_) :
      LogLevelToNative(level));

    if (register_as_global == decltype(register_as_global)::indeterminate_value ?
        register_by_default_ : register_as_global) {
      spdlog::register_logger(logger);
    }

    return logger;
  }
}

/**
 * @brief Prints endpoints.
 * This function is used to print endpoints into logger messages. It returns a
 * reference to @p os argument.
 *
 * @tparam OStream
 *   A type which meets output stream concepts.
 *
 * @param os
 *   This is output stream used to print an endpoint.
 *
 * @param[in] endpoint
 *   This is an endpoint to be printed.
 *
 * @return
 *   A reference to @p os.
 */
template <typename OStream>
OStream& operator<<(OStream& os, const boost::asio::ip::tcp::socket::endpoint_type& endpoint) noexcept {
  return os << endpoint;
}

}  // namespace fusion_server
