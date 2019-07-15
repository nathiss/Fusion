/**
 * @file game.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the implementation of the Game class.
 *
 * Copyright 2019 Kamil Rusin
 */

#include <fusion_server/game.hpp>
#include <fusion_server/json.hpp>
#include <fusion_server/server.hpp>
#include <fusion_server/system_abstractions.hpp>
#include <fusion_server/websocket_session.hpp>

using fusion_server::system_abstractions::Package;

namespace fusion_server {

Game::Game() noexcept
    : next_player_id_{0}, logger_{LoggerManager::Get()} {
  delegate_ = [this](const json::JSON& package, WebSocketSession* src){
    DoResponse(src, package);
  };
}

void Game::SetLogger(LoggerManager::Logger logger) noexcept {
  logger_ = std::move(logger);
}

LoggerManager::Logger Game::GetLogger() const noexcept {
  return logger_;
}

Game::join_result_t Game::Join(WebSocketSession *session, Team team) noexcept {
  if (IsInGame(session)) {
    logger_->warn("Trying to join already joined session. ({})",
      session->GetRemoteEndpoint());
    return {};
  }

  std::size_t player_id;
  switch (team) {
    case Team::kFirst: {
      {
        std::lock_guard l{first_team_mtx_};
        if (first_team_.size() >= kMaxPlayersPerTeam) {
          return {};
        }
        player_id = next_player_id_++;
        first_team_.insert({session, std::make_unique<Player>(player_id)});
      }
      break;
    }

    case Team::kSecond: {
      {
        std::lock_guard l{second_team_mtx_};
        if (second_team_.size() >= kMaxPlayersPerTeam) {
          return {};
        }
        player_id = next_player_id_++;
        second_team_.insert({session, std::make_unique<Player>(player_id)});
      }
      break;
    }

    case Team::kRandom:
    default: {
      std::lock_guard lf{first_team_mtx_};
      if (std::lock_guard ls{second_team_mtx_}; first_team_.size() >= second_team_.size()) {
        if (second_team_.size() >= kMaxPlayersPerTeam) {
          return {};
        }
        player_id = next_player_id_++;
        team = Team::kSecond;
        second_team_.insert({session, std::make_unique<Player>(player_id)});
        break;
      }
      // The second team is bigger than the first.
      if (first_team_.size() >= kMaxPlayersPerTeam) {
        return {};
      }
      player_id = next_player_id_++;
      team = Team::kFirst;
      first_team_.insert({session, std::make_unique<Player>(player_id)});
    }
  }  // switch

  logger_->debug("{} joined the game.", session->GetRemoteEndpoint());

  {
    std::lock_guard l{players_cache_mtx_};
    players_cache_[session] = team;
  }
  return std::make_optional<join_result_t::value_type>(delegate_, GetCurrentState(), player_id);
}

bool Game::Leave(WebSocketSession* session) noexcept {
  Team team_id{Team::kRandom};

  if (std::lock_guard l{players_cache_mtx_}; players_cache_.count(session) != 0) {
    team_id = players_cache_[session];
    players_cache_.erase(session);
  } else {
    logger_->warn("The given session doesn't exist in the cache. Searching in both teams.");
  }

  if (team_id == Team::kFirst || team_id == Team::kRandom) {
    std::lock_guard l{first_team_mtx_};
    for (const auto& pair : first_team_) {
      if (session == pair.first) {
        first_team_.erase(pair);
        return true;
      }
    }
  }

  if (team_id == Team::kSecond || team_id == Team::kRandom) {
    std::lock_guard l{second_team_mtx_};
    for (const auto& pair : second_team_) {
      if (session == pair.first) {
        second_team_.erase(pair);
        return true;
      }
    }
  }

  return false;
}

void Game::BroadcastPackage(const std::shared_ptr<Package>& package) noexcept {
  {
    std::lock_guard l{first_team_mtx_};
    for (auto& pair : first_team_) {
      pair.first->Write(package);
    }
  }

  {
    std::lock_guard l{second_team_mtx_};
    for (const auto& pair : second_team_) {
      pair.first->Write(package);
    }
  }
}

std::size_t Game::GetPlayersCount() const noexcept {
  std::size_t ret{0};
  {
    std::lock_guard l{first_team_mtx_};
    ret += first_team_.size();
  }
    {
    std::lock_guard l{second_team_mtx_};
    ret += second_team_.size();
  }
  return ret;
}

bool Game::IsInGame(WebSocketSession *session) const noexcept {
  std::lock_guard l{players_cache_mtx_};

  return players_cache_.count(session) != 0;
}

json::JSON Game::GetCurrentState() const noexcept {
  auto state = [] {
    return json::JSON({
      {"players", {json::JSON::array()}},
      {"rays", {json::JSON::array()}},
    }, false, json::JSON::value_t::object);
  }();

  {
    std::lock_guard l{rays_mtx_};
    for (auto& [_, ray] : rays_) {
      state["rays"].push_back(ray.ToJson());
    }
  }

  {
    std::lock_guard l{first_team_mtx_};
    for (auto& [_, player_ptr] : first_team_) {
      state["players"].push_back(player_ptr->ToJson());
    }
  }

  {
    std::lock_guard l{second_team_mtx_};
    for (auto& [_, player_ptr] : second_team_) {
      state["players"].push_back(player_ptr->ToJson());
    }
  }

  return state;
}

void
Game::DoResponse(WebSocketSession* session, const json::JSON& request) noexcept {
  const auto make_unidentified = [] {
    return json::JSON({
      {"type", {"warning"}},
      {"message", {"Received an unidentified package."}},
      {"closed", {false}},
    }, false, json::JSON::value_t::object);
  };

  // analysing
  if (request["type"] == "update") {
    if (request["team_id"] == 0) {
      std::lock_guard l{first_team_mtx_};
      for (const auto& [ws, player_ptr] : first_team_) {
        if (ws == session) {
          player_ptr->angle_ = request["angle"];
          player_ptr->position_.x = request["position"];
        }
      }
    } else {
      std::lock_guard l{second_team_mtx_};
      for (const auto& [ws, player_ptr] : second_team_) {
        if (ws == session) {
          player_ptr->angle_ = request["angle"];
          player_ptr->position_.x = request["position"];
        }
      }
    }

    auto response = [this] {
      json::JSON ret = json::JSON::object();
      auto state = GetCurrentState();
      return json::JSON({
        {"type", {"update"}},
        {"players", {state["players"]}},
        {"rays", {state["rays"]}},
      }, false, json::JSON::value_t::object);
    }();
    BroadcastPackage(std::make_shared<Package>(std::move(response)));
  }  // "update"

  if (request["type"] == "leave") {
    if (!Leave(session)) {
      logger_->warn("Trying to remove an unjoined session ({}).",
        session->GetRemoteEndpoint());
      session->Close();
      return;
    }

    logger_->debug("Session {} left the game.", session->GetRemoteEndpoint());

    // TODO(nathiss): broadcast the leaving
    session->delegate_ = Server::GetInstance().Register(session);
  }

  logger_->warn("Received an unidentified package from {}. [type={}]",
    session->GetRemoteEndpoint(), request["type"].dump());
  session->Write(std::make_shared<Package>(make_unidentified()));
}

}  // namespace fusion_server
