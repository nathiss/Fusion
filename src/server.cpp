/**
 * @file server.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the implementation of the Server class.
 *
 * Copyright 2019 Kamil Rusin
 */

#include <string_view>
#include <tuple>
#include <utility>

#include <fusion_server/json.hpp>
#include <fusion_server/server.hpp>
#include <fusion_server/websocket_session.hpp>

namespace fusion_server {

std::unique_ptr<Server> Server::instance_ = nullptr;  // NOLINT(fuchsia-statically-constructed-objects)

Server& Server::GetInstance() noexcept {
  if (instance_ == nullptr) {
    instance_ = std::unique_ptr<Server>{new Server};
    instance_->GetLogger()->info("Creating a new server instance.");
  }
  return *instance_;
}

bool Server::Configure(json::JSON config) noexcept {
  config_ = std::move(config);

  if (config_.contains("logger")) {
    if (!config_["logger"].is_object()) {
      logger_->critical("[Config] Field \"logger\" is not an object.");
      return false;
    }
    if (!logger_manager_.Configure(config_["logger"])) {
       logger_->error("[Config::Logger] Config was ill-formed.");
    } else {
      logger_ = logger_manager_.CreateLogger<true>("server");
    }
  }

  if (!config_.contains("listener")) {
    logger_->critical("[Config::Listener] Configuration for the Listener in mandatory.");
    return false;
  } else {
    if (!config_["listener"].is_object()) {
      logger_->critical("[Config] Field \"listener\" is not an object.");
      return false;
    }
    listener_ = std::make_shared<Listener>(ioc_);
    listener_->SetLogger(logger_manager_.CreateLogger<false>("listener"));
    if (!listener_->Configure(config_["listener"])) return false;
  }

  // TODO(nathiss): complete configuration.

  auto ws = logger_manager_.CreateLogger<true>("websocket", LoggerManager::Level::none,
    true);
  auto game = logger_manager_.CreateLogger<true>("game", LoggerManager::Level::none,
    true);

  // FIXME(nathiss): *HACK* remove this ugly hack
  spdlog::register_logger(ws);
  spdlog::register_logger(game);
  return true;
}

void Server::SetLogger(LoggerManager::Logger logger) noexcept {
  logger_ = std::move(logger);
}

LoggerManager::Logger Server::GetLogger() const noexcept {
  return logger_;
}

boost::asio::io_context& Server::GetIOContext() noexcept {
  return ioc_;
}

system_abstractions::IncommingPackageDelegate&
Server::Register(WebSocketSession* session) noexcept {
  if (std::lock_guard l{sessions_correlation_mtx_}; sessions_correlation_.count(session) != 0) {
    logger_->warn("Second registration of a session {}.", session->GetRemoteEndpoint());
    return unjoined_delegate_;
  }

  sessions_correlation_[session] = {};

  unidentified_sessions_mtx_.lock();
  unidentified_sessions_.insert(session);
  unidentified_sessions_mtx_.unlock();

  logger_->debug("New WebSocket session registered {}.", session->GetRemoteEndpoint());

  return unjoined_delegate_;
}

void Server::Unregister(WebSocketSession* session) noexcept {
  if (has_stopped_) {
    // The server has stopped. We don't allow session to unregister in order to
    // prevent double free.
    return;
  }

  // TODO(nathiss): change lock scope of this mutex.
  std::lock_guard l{sessions_correlation_mtx_};
  if (auto it = sessions_correlation_.find(session); it != sessions_correlation_.end()) {
    auto game_name = it->second;
    sessions_correlation_.erase(it);

    // TODO(nathiss): refactor this.
    if (!game_name) {
      logger_->debug("Unregistering session {}.", session->GetRemoteEndpoint());
      std::lock_guard l{unidentified_sessions_mtx_};
      unidentified_sessions_.erase(session);
    } else {
      logger_->debug("Removing session {} from game {}.", session->GetRemoteEndpoint(), game_name.value());
      std::lock_guard l{games_mtx_};
      auto& game = games_[game_name.value()];
      game->Leave(session);
      if (game->GetPlayersCount() == 0) {
        logger_->debug("Game {} has no players. Removing.", game_name.value());
          games_.erase(game_name.value());
      }
    }
  } else {
    logger_->warn("Trying to unregister session which is not registered. [{}]",
      session->GetRemoteEndpoint());
  }
}

bool Server::StartAccepting() noexcept {
  logger_->info("Creating a Listener object.");
  listener_->Bind();
  return listener_->Run();
}

void Server::Shutdown() noexcept {
  has_stopped_ = true;
}

Server::Server() noexcept {
  logger_ = LoggerManager::Get();
  has_stopped_ = false;

  unjoined_delegate_ = [this](const json::JSON& package, WebSocketSession* src) {
    logger_->debug("Received a new package from {}.", src->GetRemoteEndpoint());
    auto response = MakeResponse(src, package);
    src->Write(std::make_shared<Package>(response.dump()));
  };
}

json::JSON Server::MakeResponse(WebSocketSession* src, const json::JSON& request) noexcept {
  const auto make_unidentified = [] {
    return json::JSON({
      {"type", {"warning"}},
      {"message", {"Received an unidentified package."}},
      {"closed", {false}},
    }, false, json::JSON::value_t::object);
  };

  const auto make_game_full = [](std::size_t id){
    return json::JSON({
      {"id", {id}},
      {"result", {"full"}},
    }, false, json::JSON::value_t::object);
  };

  if (request["type"] == "join") {
    std::string game_name = request["game"];
    games_mtx_.lock();
    auto it = games_.find(game_name);
    if (it == games_.end()) {
      it = games_.emplace(game_name, std::make_shared<Game>()).first;
      it->second->SetLogger(LoggerManager::Get("game"));
    }
    auto join_result = it->second->Join(src);
    games_mtx_.unlock();
    if (!join_result) {  // The game is full.
      return std::make_pair(false, make_game_full(request["id"]));
    }
    // If we're here it means the join was successful.
    src->delegate_ = std::get<0>(join_result.value());

    {
      std::lock_guard l{unidentified_sessions_mtx_};
      unidentified_sessions_.erase(src);
    }
    {
      std::lock_guard l{sessions_correlation_mtx_};
      sessions_correlation_[src] = std::make_optional<std::string>(std::move(game_name));
    }

    auto response = [&request, &join_result] {
      return json::JSON({
        {"id", {request["id"]}},
        {"result", {"joined"}},
        {"my_id", {std::get<2>(join_result.value())}},
        {"players", {std::get<1>(join_result.value())["players"]}},
        {"rays", {std::get<1>(join_result.value())["rays"]}},
      }, false, json::JSON::value_t::object);
    }();

    return std::make_pair(false, std::move(response));
  }  // "join"

  // If we're here it means we've received an unidentified package.
  logger_->warn("Received an unidentified package from {}. [type={}]",
    src->GetRemoteEndpoint(), request["type"].dump());
  return std::make_pair(false, make_unidentified());
}

}  // namespace fusion_server
