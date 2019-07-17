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

std::unique_ptr<std::set<std::string>> LoggerManager::registered_names_ = nullptr;

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

  if (config.contains("flush_every")) {
    if (!config["flush_every"].is_number_float()) {
      SetDefault();
      return false;
    }
    configuration_.flush_every_default_ = std::chrono::seconds(
      config["flush_every"]);
    spdlog::flush_every(configuration_.flush_every_default_);
  }

  return true;
}

auto LoggerManager::Get(const std::string& name) noexcept -> LoggerManager::Logger {
  if (name.empty()) return spdlog::default_logger();
  if (registered_names_ == nullptr) return nullptr;

  if (auto it = registered_names_->find(name); it == registered_names_->end()) {
    return nullptr;
  }
  return spdlog::get(name);
}

auto LoggerManager::Register(Logger logger) noexcept -> std::pair<bool, Logger> {
  if (registered_names_ == nullptr) {
    registered_names_ = std::make_unique<std::set<std::string>>();
  }
  auto [it, took_place] = registered_names_->insert(logger->name());
  return std::make_pair(took_place, spdlog::get(*it));
}

void LoggerManager::SetDefault() noexcept {
  configuration_.root_ = "/";
  configuration_.logger_pattern_ = "[%H:%M:%S:%e] [thread %t] [%^%l@%n%$] %v";
  configuration_.extension_ = ".log";
  configuration_.level_ = Level::warn;
  configuration_.register_by_default_ = false;
  configuration_.flush_every_default_ = std::chrono::seconds(30);
}

std::string LoggerManager::AssembleFileName(std::string file_name) const noexcept {
  return configuration_.root_ + std::move(file_name) + configuration_.extension_;
}

}  // namespace fusion_server
