#include <fusion_server/game.hpp>
#include <fusion_server/websocket_session.hpp>

using fusion_server::system_abstractions::Package;

namespace fusion_server {

Game::Game() noexcept {
  delegete_ = [this](Package package, WebSocketSession*){
    // TODO: if it's a leave package, then change reqiester the session in the server
    this->BroadcastPackage(package);
  };
}

auto Game::Join(WebSocketSession *session, Team team) noexcept
    -> join_result_t {
#ifdef DEBUG
  std::cout << "[Game: " << this << "] Joining: " << session << std::endl;
#endif
  if (IsInGame(session)) {
    return {}; // This should never happen.
  }

  switch (team) {
    case Team::kFirst: {
      std::lock_guard l{first_team_mtx_};
      if (first_team_.size() >= kMaxPlayersPerTeam)
        return {};
      auto player_id = next_player_id_++;
      first_team_.insert({session, std::make_unique<Player>(player_id)});
      return std::make_optional<join_result_t::value_type>(delegete_, GetCurrentState(), player_id);
    }

    case Team::kSecond: {
      std::lock_guard l{second_team_mtx_};
      if (second_team_.size() >= kMaxPlayersPerTeam)
        return {};
      auto player_id = next_player_id_++;
      second_team_.insert({session, std::make_unique<Player>(player_id)});
      return std::make_optional<join_result_t::value_type>(delegete_, GetCurrentState(), player_id);
    }

    case Team::kRandom:
    default: {
      std::lock_guard l1{first_team_mtx_};
      std::lock_guard l2{second_team_mtx_};
      if (first_team_.size() > second_team_.size()) {
        if (second_team_.size() >= kMaxPlayersPerTeam)
          return {};
        auto player_id = next_player_id_++;
        second_team_.insert({session, std::make_unique<Player>(player_id)});
        return std::make_optional<join_result_t::value_type>(delegete_, GetCurrentState(), player_id);
      } else { // Either the second is bigger or they have the same size.
        if (first_team_.size() >= kMaxPlayersPerTeam)
          return {};
        auto player_id = next_player_id_++;
        first_team_.insert({session, std::make_unique<Player>(player_id)});
        return std::make_optional<join_result_t::value_type>(delegete_, GetCurrentState(), player_id);
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

PackageParser::JSON Game::GetCurrentState() const noexcept {
  PackageParser::JSON state{
    {"players", decltype(state)::array()},
    {"rays", decltype(state)::array()},
  };

  {
    std::lock_guard l{rays_mtx_};
    for (auto& [_, ray] : rays_)
      state["rays"].push_back(ray.ToJson());
  }

  {
    // We don't need to lock the mutex, because it's already locked by the
    // callee.
    for (auto& [_, player_ptr] : first_team_)
      state["players"].push_back(player_ptr->ToJson());
  }

  {
    // We don't need to lock the mutex, because it's already locked by the
    // callee.
    for (auto& [_, player_ptr] : second_team_)
      state["players"].push_back(player_ptr->ToJson());
  }

  return state;
}

}  // namespace fusion_server