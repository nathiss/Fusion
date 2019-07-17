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
    configuration_.root_ = config["root"];
  }

  if (config.contains("extension")) {
    if (!config["extension"].is_string()) {
      SetDefault();
      return false;
    }
    configuration_.extension_ = config["extension"];
  }

  if (config.contains("level")) {
    if (!config["level"].is_string()) {
      SetDefault();
      return false;
    }
    if (config["level"] == "trace") configuration_.level_ = Level::trace;
    else if (config["level"] == "debug") configuration_.level_ = Level::debug;
    else if (config["level"] == "info") configuration_.level_ = Level::info;
    else if (config["level"] == "warn") configuration_.level_ = Level::warn;
    else if (config["level"] == "error") configuration_.level_ = Level::error;
    else if (config["level"] == "critical") configuration_.level_ = Level::critical;
    else configuration_.level_ = Level::none;
  }

  if (config.contains("pattern")) {
    if (!config["pattern"].is_string()) {
      SetDefault();
      return false;
    }
    configuration_.logger_pattern_ = config["pattern"];
  }

  if (config.contains("register_by_default")) {
    if (!config["register_by_default"].is_boolean()) {
      SetDefault();
      return false;
    }
    configuration_.register_by_default_ = config["register_by_default"];
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
  configuration_.root_ = "/";
  configuration_.logger_pattern_ = "[%H:%M:%S:%e] [thread %t] [%^%l@%n%$] %v";
  configuration_.extension_ = ".log";
  configuration_.level_ = Level::warn;
  configuration_.register_by_default_ = false;
}

std::string LoggerManager::AssembleFileName(std::string file_name) const noexcept {
  return configuration_.root_ + std::move(file_name) + configuration_.extension_;
}

}  // namespace fusion_server
