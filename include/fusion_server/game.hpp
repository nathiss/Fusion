#pragma once

#include <cstdlib>

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <variant>

#include <spdlog/spdlog.h>

#include <fusion_server/player.hpp>
#include <fusion_server/system_abstractions.hpp>

using fusion_server::system_abstractions::Package;

namespace fusion_server {

/**
 * This is the forward declaration of the WebSocketSession class.
 */
class WebSocketSession;

/**
 * This class represents a game. It creates a common context for all joined
 * clients.
 */
class Game {
 public:
  /**
   * This enum contains the teams' identifiers.
   */
  enum class Team {
    /**
     * This identifies the first team in the game.
     */
    kFirst,

    /**
     * This identifies the second team in the game.
     */
    kSecond,

    /**
     * This indicates that a WebSocketSession should be assigned to a random team.
     */
    kRandom,
  };

  /**
   * This is the return type of the Join method.
   */
  using join_result_t =
  std::optional<std::tuple<system_abstractions::IncommingPackageDelegate&, PackageParser::JSON, std::size_t>>;

  /**
   * @brief Explicitly deleted copy constructor.
   * It's deleted due to presence of unique_ptr in class hierarchy.
   *
   * @param[in] other
   *   Copied object.
   */
  Game(const Game& other) noexcept = delete;

  /**
   * @brief Explicitly deleted copy operator.
   * It's deleted due to presence of unique_ptr in class hierarchy.
   *
   * @param[in] other
   *   Copied object.
   *
   * @return
   *   Reference to `this` object.
   */
  Game& operator=(const Game& other) noexcept = delete;

  /**
   * This constructor creates the asynchronous reading delegate.
   */
  Game() noexcept;

  /**
   * This method joins the client to this game and adds its session to the
   * proper team. If the joining was successful it returns a pair of a new
   * incomming package delegate and a JSON object containing information about
   * the current state of the game, otherwise the returned object is in its
   * invalid state.
   *
   * @param[in] session
   *   This is the WebSocket session connected to a client.
   *
   * @param[in] team
   *   This identifies the team, to which the client will be assigned.
   *   The default value indicates that the client will be assigned to a random
   *   team.
   *
   * @return
   *   If the joining was successful pair of a new incomming package delegate
   *   and a JSON object containing information about the current state of the
   *   game is returned, otherwise the returned object is in its invalid state.
   *
   * @note
   *   If a client has already joined to this game, the method does nothing and
   *   returns an invalid state object.
   */
  [[ nodiscard ]] join_result_t
  Join(WebSocketSession *session, Team team = Team::kRandom) noexcept;

  /**
   * This method removes the given session from this game.
   * It returns a indication whether or not the session has been removed.
   *
   * @param[in] session
   *   The session to be removed from this game.
   *
   * @return
   *   A indication whether or not the session has been removed is returned.
   *
   * @note
   *   If the session has not been assigned to this game, the method does
   *   nothing.
   */
  bool Leave(WebSocketSession *session) noexcept;

  /**
   * This method broadcasts the given package to all clients connected to this
   * game.
   *
   * @param[in] package
   *   The package to be broadcasted.
   */
  void BroadcastPackage(Package package) noexcept;

  /**
   * This method returns the amount of players in this game.
   *
   * @return
   *   The amount of players in this game is returned.
   */
  std::size_t GetPlayersCount() const noexcept;

  /**
   * This constant contains the number of players that can be assigned to
   * a team.
   */
  static constexpr size_t kMaxPlayersPerTeam = 5;

 private:
  /**
   * This method returns an indication whether or not the client identified by
   * the given session has already joined to this game.
   *
   * @param[in] session
   *   The WebSocketSession connected to the client.
   *
   * @return
   *   An indication whether or not the client identified by the given session
   *   has already joined to this game is returned.
   */
  bool IsInGame(WebSocketSession *session) const noexcept;

  /**
   * This method returns a JSON object containg an encoded current state of this
   * game.
   *
   * @return
   *   A JSON object containg an encoded current state of this game is returned.
   */
  PackageParser::JSON GetCurrentState() const noexcept;

  /**
   * This method prepairs a response for the given request and either sends it
   * or broadcasts it.
   *
   * @param[in] session
   *   The WebSocket session connected to the client.
   *
   * @param[in] request
   *   This is a client's request.
   */
  void
  DoResponse(WebSocketSession* session, const PackageParser::JSON& request) noexcept;

  /**
   * This set contains the pairs of WebSocket sessions and their roles in the
   * game of the first team.
   */
  std::set<std::pair<WebSocketSession*, std::unique_ptr<Player>>> first_team_;

  /**
   * This mutex is used to synchronise all oprations done on the collection
   * containing the first team.
   */
  mutable std::mutex first_team_mtx_;

  /**
   * This set contains the pairs of WebSocket sessions and their roles in the
   * game of the second team.
   */
  std::set<std::pair<WebSocketSession*, std::unique_ptr<Player>>> second_team_;

  /**
   * This mutex is used to synchronise all oprations done on the collection
   * containing the second team.
   */
  mutable std::mutex second_team_mtx_;

  /**
   * @brief Players' team cache.
   * This maps the WebSocket session to the team identifier in which the player
   * currently is.
   */
  std::map<WebSocketSession*, Team> players_cache_;

   /**
   * This mutex is used to synchronise all oprations done on the map
   * of WebSocket sessions and their players' team identifiers.
   */
  mutable std::mutex players_cache_mtx_;

  /**
   * This is used to set the id of the next player. It holds the next free id.
   */
  std::atomic<std::size_t> next_player_id_;

  /**
   * This map contains all rays in the game. The key is the ray's parent which
   * is either a player or another ray.
   */
  std::map <std::variant<Ray*, Player*>, Ray> rays_;

  /**
   * This mutex is used to synchronise all oprations done on the map
   * containing the rays.
   */
  mutable std::mutex rays_mtx_;

  /**
   * This callable object is used as a callback to the asynchronous reading of
   * all clients in this game.
   */
  system_abstractions::IncommingPackageDelegate delegete_;

  /**
   * @brief Game's logger.
   * This is a pointer to the logger used in Game class.
   */
  std::shared_ptr<spdlog::logger> logger_;
};

}  // namespace fusion_server