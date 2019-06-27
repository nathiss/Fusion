#include <fusion_server/game.hpp>
#include <fusion_server/websocket_session.hpp>

using fusion_server::system_abstractions::Package;

namespace fusion_server {

Game::Game() noexcept {
  delegete_ = [this](Package package, WebSocketSession* session){
    // TODO: if it's a leave package, then change reqiester the session in the server
    this->BroadcastPackage(package);
  };
}

bool Game::Join(WebSocketSession *session, Team team) noexcept {
#ifdef DEBUG
  std::cout << "[Game: " << this << "] Joining: " << session << std::endl;
#endif
  if (IsInGame(session)) {
    return true; // This should never happen.
  }

  switch (team) {
    case Team::kFirst: {
      std::lock_guard l{first_team_mtx_};
      if (first_team_.size() >= kMaxPlayersPerTeam)
        return false;
      first_team_.insert({session, std::make_unique<Role>()});
      return true;
    }

    case Team::kSecond: {
      std::lock_guard l{second_team_mtx_};
      if (second_team_.size() >= kMaxPlayersPerTeam)
        return false;
      // TODO: change this to a sane one
      second_team_.insert({session, std::make_unique<Role>()});
      return true;
    }

    case Team::kRandom: {
      std::lock_guard l1{first_team_mtx_};
      std::lock_guard l2{second_team_mtx_};
      if (first_team_.size() > second_team_.size()) {
        if (second_team_.size() >= kMaxPlayersPerTeam)
          return false;
        second_team_.insert({session, std::make_unique<Role>()});
        return true;
      } else { // Either the second is bigger or they have the same size.
        if (first_team_.size() >= kMaxPlayersPerTeam)
          return false;
        first_team_.insert({session, std::make_unique<Role>()});
        return true;
      }
    }
  }  // switch
}

bool Game::Leave(WebSocketSession* session) noexcept {
#ifdef DEBUG
  std::cout << "[Game: " << this << "] Leaving: " << session << std::endl;
#endif
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
#ifdef DEBUG
  std::cout << "[Game: " << this << "] Broadcasting: " << *package << std::endl;
#endif
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

system_abstractions::IncommingPackageDelegate& Game::GetDelegate() noexcept {
  return delegete_;
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