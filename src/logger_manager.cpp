/**
 * @file logger_manager.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the definition of the LoggerManager class.
 *
 * (c) 2019 by Kamil Rusin
 */

#include <utility>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <fusion_server/logger_manager.hpp>

namespace fusion_server {

LoggerManager::LoggerManager() noexcept {
  SetDefault();
}

bool LoggerManager::Configure(const json::JSON& config) noexcept {
  if (config.contains("root")) {
    if (!config["root"].is_string()) {
      return false;
    }
    root_ = config["root"];
  }

  if (config.contains("extension")) {
    if (!config["extension"].is_string()) {
      SetDefault();
      return false;
    }
    extension_ = config["extension"];
  }

  if (config.contains("level")) {
    if (!config["level"].is_string()) {
      SetDefault();
      return false;
    }
    if (config["level"] == "trace") level_ = Level::trace;
    else if (config["level"] == "debug") level_ = Level::debug;
    else if (config["level"] == "info") level_ = Level::info;
    else if (config["level"] == "warn") level_ = Level::warn;
    else if (config["level"] == "error") level_ = Level::error;
    else if (config["level"] == "critical") level_ = Level::critical;
    else level_ = Level::none;
  }

  if (config.contains("pattern")) {
    if (!config["pattern"].is_string()) {
      SetDefault();
      return false;
    }
    logger_pattern_ = config["pattern"];
  }

  if (config.contains("register_by_default")) {
    if (!config["register_by_default"].is_boolean()) {
      SetDefault();
      return false;
    }
    register_by_default_ = config["register_by_default"];
  }

  return true;
}

std::shared_ptr<spdlog::logger> LoggerManager::Get(const std::string& name) noexcept {
  if (name.empty()) {
    return spdlog::default_logger();
  }
  return spdlog::get(name);
}

void LoggerManager::SetDefault() noexcept {
  root_ = "/";
  logger_pattern_ = "[%H:%M:%S:%e] [thread %t] [%^%l@%n%$] %v";
  extension_ = ".log";
  level_ = Level::warn;
  register_by_default_ = false;
}

std::string LoggerManager::AssembleFileName(std::string file_name) const noexcept {
  return root_ + std::move(file_name) + extension_;
}

}  // namespace fusion_server
