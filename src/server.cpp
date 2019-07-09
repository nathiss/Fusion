#ifdef DEBUG
#include <iostream>
#endif
#include <string_view>
#include <utility>

#include <fusion_server/server.hpp>
#include <fusion_server/websocket_session.hpp>

namespace fusion_server {

std::unique_ptr<Server> Server::instance_ = nullptr;

Server& Server::GetInstance() noexcept {
  if (instance_ == nullptr) {
    instance_.reset(new Server);
  }
  return *instance_;
}

boost::asio::io_context& Server::GetIOContext() noexcept {
  return ioc_;
}

auto Server::Register(WebSocketSession* new_session) noexcept
    -> system_abstractions::IncommingPackageDelegate& {
  std::lock_guard l{unidentified_sessions_mtx_};
  auto [it, took_place] = unidentified_sessions_.insert(new_session);
#ifdef DEBUG
  std::cout << "[Server: " << this << "] New session registered: " << new_session << std::endl;
#endif

  if (!took_place) {
    std::cerr << "Server::Register: the session " << new_session
    << " has already been registered." << std::endl;
  } else {
    std::lock_guard l{sessions_correlation_mtx_};
    sessions_correlation_[new_session] = {};
  }

  return unjoined_delegate_;
}

void Server::Unregister(WebSocketSession* session) noexcept {
  decltype(sessions_correlation_)::value_type::second_type game_name{};
  {
    std::lock_guard l{sessions_correlation_mtx_};
    game_name = sessions_correlation_[session];
    sessions_correlation_.erase(session);
  }

  if (!game_name) {
#ifdef DEBUG
  std::cout << "[Server: " << this << "] Removing session: " << session << std::endl;
#endif
    std::lock_guard l{unidentified_sessions_mtx_};
    unidentified_sessions_.erase(session);
    return;
  }

  std::lock_guard l{games_mtx_};
  auto& game = games_[game_name.value()];
  game.Leave(session);
  if (game.GetPlayersCount() == 0) {
#ifdef DEBUG
    std::cout << "[Server: " << this << "] Game " << &game << " has no more playes. Removing." << std::endl;
#endif
    games_.erase(game_name.value());
  }
}

void Server::StartAccepting() noexcept {
  // TODO: read the local endpoint from a config file.
  std::make_shared<Listener>(ioc_, "127.0.0.1", 8080)->Run();
}

Server::Server() noexcept {
  unjoined_delegate_ = [this](
    Package package, WebSocketSession* src) {
#ifdef DEBUG
    std::cout << "[Server " << this << "] A new package from " << src << std::endl;
#endif
    auto parsed = package_parser_.Parse(*package);
    if (!parsed) {
      PackageParser::JSON response = {
        {"error_message", "Package does not contain a valid request."},
        {"closed", true}
      };
      src->Close(system_abstractions::make_Package(response.dump()));
      return;
    }

    auto [should_close, response] = MakeResponse(src, std::move(parsed.value()));
    if (should_close)
      src->Close(system_abstractions::make_Package(response.dump()));
    else
      src->Write(system_abstractions::make_Package(response.dump()));
  };
}

std::pair<bool, PackageParser::JSON>
Server::MakeResponse(WebSocketSession* src, const PackageParser::JSON& request) noexcept {
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
    return std::make_pair(true, make_invalid());

  if (request["type"] == "join") {
    if (!(request.contains("id") && request.contains("game") &&
        request.contains("nick") && request.size() == 4)) {
      return std::make_pair(true, make_invalid("join"));
    }

    Game::join_result_t join_result;
    {
      std::lock_guard l{games_mtx_};
      auto& game = games_[request["game"]];
      join_result = game.Join(src);
    }
    if (!join_result) {  // The game is full.
      PackageParser::JSON response = {{"id", request["id"]}, {"result", "full"}};
      return std::make_pair(false, std::move(response));
    }
    // If we're here it means the join was successful.
    src->delegate_ = join_result.value().first;

    {
      std::lock_guard l{unidentified_sessions_mtx_};
      unidentified_sessions_.erase(src);
    }
    {
      std::lock_guard l{sessions_correlation_mtx_};
      sessions_correlation_[src] = request["game"];
    }

    PackageParser::JSON response = {
      {"id", request["id"]},
      {"result", "joined"},
      std::move(join_result.value().second),  // the game's current state
    };

    return std::make_pair(false, std::move(response));
  }  // "join"

  // If we're here it means we've received an unidentified package.
  return std::make_pair(false, make_unidentified());
}

}  // namespace fusion_server