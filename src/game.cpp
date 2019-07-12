/**
 * @file game.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the implementation of the Game class.
 *
 * (c) 2019 by Kamil Rusin
 */

#include <fusion_server/game.hpp>
#include <fusion_server/logger_types.hpp>
#include <fusion_server/package_parser.hpp>
#include <fusion_server/server.hpp>
#include <fusion_server/system_abstractions.hpp>
#include <fusion_server/websocket_session.hpp>

using fusion_server::system_abstractions::Package;

namespace fusion_server {

Game::Game(std::string game_name) noexcept {
  std::string logger_name{"game["};
  logger_name += std::move(game_name);
  logger_name += "]";
  logger_ = system_abstractions::CreateLogger(std::move(logger_name), false);
  delegete_ = [this](const PackageParser::JSON& package, WebSocketSession* src){
    DoResponse(src, package);
  };
}

auto Game::Join(WebSocketSession *session, Team team) noexcept
    -> join_result_t {
  if (IsInGame(session)) {
    logger_->warn("Tryig to join already joined session. ({})",
      session->GetRemoteEndpoint());
    return {};
  }

  std::size_t player_id;
  switch (team) {
    case Team::kFirst: {
      {
        std::lock_guard l{first_team_mtx_};
        if (first_team_.size() >= kMaxPlayersPerTeam)
          return {};
        player_id = next_player_id_++;
        first_team_.insert({session, std::make_unique<Player>(player_id)});
      }
      break;
    }

    case Team::kSecond: {
      {
        std::lock_guard l{second_team_mtx_};
        if (second_team_.size() >= kMaxPlayersPerTeam)
          return {};
        player_id = next_player_id_++;
        second_team_.insert({session, std::make_unique<Player>(player_id)});
      }
      break;
    }

    case Team::kRandom:
    default: {
      std::lock_guard lf{first_team_mtx_};
      if (std::lock_guard ls{second_team_mtx_}; first_team_.size() >= second_team_.size()) {
        if (second_team_.size() >= kMaxPlayersPerTeam)
          return {};
        player_id = next_player_id_++;
        team = Team::kSecond;
        second_team_.insert({session, std::make_unique<Player>(player_id)});
        break;
      }
      // The second team is bigger than the first.
      if (first_team_.size() >= kMaxPlayersPerTeam)
        return {};
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
  return std::make_optional<join_result_t::value_type>(delegete_, GetCurrentState(), player_id);
}

bool Game::Leave(WebSocketSession* session) noexcept {
  Team team_id{Team::kRandom};

  if (std::lock_guard l{players_cache_mtx_}; players_cache_.count(session) != 0) {
    team_id = players_cache_[session];
    players_cache_.erase(session);
  } else{
    logger_->warn("The given session doesn't exist in the cache. Searching in both teams.");
  }

  if (team_id == Team::kFirst || team_id == Team::kRandom) {
    std::lock_guard l{first_team_mtx_};
    for (const auto& pair : first_team_)
      if (session == pair.first) {
        first_team_.erase(pair);
        return true;
      }
  }

  if (team_id == Team::kSecond || team_id == Team::kRandom) {
    std::lock_guard l{second_team_mtx_};
    for (const auto& pair : second_team_)
      if (session == pair.first) {
        second_team_.erase(pair);
        return true;
      }
  }

  return false;
}

void Game::BroadcastPackage(Package package) noexcept {
  {
    std::lock_guard l{first_team_mtx_};
    for (auto& pair : first_team_)
      pair.first->Write(package);
  }

  {
    std::lock_guard l{second_team_mtx_};
    for (const auto& pair : second_team_)
      pair.first->Write(package);
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

PackageParser::JSON Game::GetCurrentState() const noexcept {
  auto state = []{
    PackageParser::JSON ret = PackageParser::JSON::object();
    ret["players"] = PackageParser::JSON::array();
    ret["rays"] = PackageParser::JSON::array();
    return ret;
  }();

  {
    std::lock_guard l{rays_mtx_};
    for (auto& [_, ray] : rays_)
      state["rays"].push_back(ray.ToJson());
  }

  {
    std::lock_guard l{first_team_mtx_};
    for (auto& [_, player_ptr] : first_team_)
      state["players"].push_back(player_ptr->ToJson());
  }

  {
    std::lock_guard l{second_team_mtx_};
    for (auto& [_, player_ptr] : second_team_)
      state["players"].push_back(player_ptr->ToJson());
  }

  return state;
}

void
Game::DoResponse(WebSocketSession* session, const PackageParser::JSON& request) noexcept {
  const auto make_unidentified = [] {
    PackageParser::JSON ret = PackageParser::JSON::object();
    ret["type"] = "warning";
    ret["message"] = "Received an unidentified package.";
    ret["closed"] = false;
    return ret;
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

    const auto response = [this]{
      PackageParser::JSON ret = PackageParser::JSON::object();
      ret["type"] = "update";
      auto state = GetCurrentState();
      ret["players"] = state["players"];
      ret["rays"] = state["rays"];

      return ret;
    }();
    BroadcastPackage(system_abstractions::make_Package(std::move(response)));
  }  // "update"

  if (request["type"] == "leave") {
    if(!Leave(session)) {
      logger_->warn("Trying to remove an unjoined session ({}).",
        session->GetRemoteEndpoint());
      session->Close();
      return;
    } else {
      logger_->debug("Session {} left the game.", session->GetRemoteEndpoint());
    }

    // TODO: broadcast the leaving
    session->delegate_ = Server::GetInstance().Register(session);
  }

  logger_->warn("Received an unidentified package from {}. [type={}]",
    session->GetRemoteEndpoint(), request["type"].dump());
  session->Write(system_abstractions::make_Package(make_unidentified()));
}

}  // namespace fusion_server
