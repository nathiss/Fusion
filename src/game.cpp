#include <fusion_server/game.hpp>
#include <fusion_server/websocket_session.hpp>

using fusion_server::system_abstractions::Package;

namespace fusion_server {

Game::Game() noexcept {
  delegete_ = [](Package package, WebSocketSession* session){

  };
}

std::pair<bool, system_abstractions::IncommingPackageDelegate&>
Game::Join(WebSocketSession *session, Team team) noexcept {
  if (IsInGame(session)) {
    return {true, delegete_}; // This should never happen.
  }

  switch (team) {
    case Team::kFirst: {
      std::lock_guard l{first_team_mtx_};
      if (first_team_.size() >= kMaxPlayersPerTeam)
        return {false, delegete_};
      first_team_.insert({session, std::make_unique<Role>()});
      return {true, delegete_};
    }

    case Team::kSecond: {
      std::lock_guard l{second_team_mtx_};
      if (second_team_.size() >= kMaxPlayersPerTeam)
        return {false, delegete_};
      // TODO: change this to a sane one
      second_team_.insert({session, std::make_unique<Role>()});
      return {true, delegete_};
    }

    case Team::kRandom: {
      std::lock_guard l1{first_team_mtx_};
      std::lock_guard l2{second_team_mtx_};
      if (first_team_.size() > second_team_.size()) {
        if (second_team_.size() >= kMaxPlayersPerTeam)
          return {false, delegete_};
        second_team_.insert({session, std::make_unique<Role>()});
        return {true, delegete_};
      } else { // Either the second is bigger or they have the same size.
        if (first_team_.size() >= kMaxPlayersPerTeam)
          return {false, delegete_};
        first_team_.insert({session, std::make_unique<Role>()});
        return {true, delegete_};
      }
    }
  }  // switch
}

bool Game::Leave(WebSocketSession* session) noexcept {
  {
    std::lock_guard l{first_team_mtx_};
    for (const auto& pair : first_team_)
      if (session == pair.first) {
        first_team_.erase(pair);
        return true;
      }
  }

  {
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

bool Game::IsInGame(WebSocketSession *session) const noexcept {
  {
    std::lock_guard l{first_team_mtx_};
    for (const auto& pair : first_team_)
      if (session == pair.first)
        return true;
  }

  {
    std::lock_guard l{second_team_mtx_};
    for (const auto& pair : second_team_)
      if (session == pair.first)
        return true;
  }

  return false;
}

}  // namespace fusion_server