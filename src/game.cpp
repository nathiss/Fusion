#include <fusion_server/game.hpp>
#include <fusion_server/server.hpp>
#include <fusion_server/websocket_session.hpp>

using fusion_server::system_abstractions::Package;

namespace fusion_server {

Game::Game() noexcept {
  delegete_ = [this](Package package, WebSocketSession* src){
    auto parsed = package_parser_.Parse(*package);
    if (!parsed) {
      PackageParser::JSON response = {
        {"error_message", "Package does not contain a valid request."},
        {"closed", true}
      };
      src->Close(system_abstractions::make_Package(response.dump()));
      return;
    }

    DoResponse(src, std::move(parsed.value()));
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

// TODO: This whole method below is an absurd. It requires a bit of refactorisation.
// Adding a middle-ware is the first step to make it sane (checking if a package
// is valid is a nightmare, indeed).
void
Game::DoResponse(WebSocketSession* session, const PackageParser::JSON& request) noexcept {
  const auto make_invalid = [](std::string_view type = {}){
    std::string message = "One of the client's packages was ill-formed.";
    if (type != std::string_view{})
      message += "[";
      message += type;
      message += "]";
    return PackageParser::JSON {
      {"type", "error"},
      {"message", std::move(message)},
      {"closed", true}};
  };

  const auto make_unidentified = [](
    [[ maybe_unused ]] std::string_view type = {}
  ) {
    return PackageParser::JSON {
      {"type", "warning"},
      {"messaege", "Received an unidentified package."},
      {"closed", false}
    };
  };

  // analysing
  if (!request.contains("type"))
    session->Close(system_abstractions::make_Package(make_invalid()));

  if (request["type"] == "update") {
    if (!(request.contains("position") && request.contains("angle") &&
        request.contains("team_id") && request.size() == 4)) {
      session->Close(system_abstractions::make_Package(make_invalid("update")));
    }

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
    if (request.size() != 1) {
      session->Close(system_abstractions::make_Package(make_invalid()));
    }

    Leave(session);
    // We need to update the other client's state.
    BroadcastPackage(system_abstractions::make_Package(
      GetCurrentState().dump()
    ));
    session->delegate_ = Server::GetInstance().Register(session);
  }

  // If we're here it means we've received an unidentified package.
  session->Write(system_abstractions::make_Package(make_unidentified()));
}

}  // namespace fusion_server