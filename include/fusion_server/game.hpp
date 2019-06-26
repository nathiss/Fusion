#pragma once

#include <cstdlib>

#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>

#include <fusion_server/package_parser.hpp>
#include <fusion_server/system_abstractions.hpp>

using fusion_server::system_abstractions::Package;

namespace fusion_server {

/**
 * This is the forward declaration of the WebSocketSession class.
 */
class WebSocketSession;

/**
 * This type represents the roles of the players.
 */
struct Role {};

/**
 * This class represents a game. It creates a common context for at least two
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

  Game(const Game&) noexcept = delete;

  Game& operator=(const Game&) noexcept = delete;

  /**
   * This constructor creates the asynchronous reading delegate.
   */
  Game() noexcept;

  /**
   * This method joins the client to this game and adds its session to the
   * proper team. It returns a pair of boolean value, that indicates whether or
   * not the joining was successful and a callback to the client's asynchronous
   * reading.
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
   *   A pair of boolean value, that indicates whether or not the joining was
   *   successful and a callback to the client's asynchronous reading is
   *   returned.
   *
   * @note
   *   If a client has already joined to this game, the method does nothing and
   *   returns a pair of true and a callback to the client's asynchronous
   *   reading.
   */
  [[ nodiscard ]]
  std::pair<bool, system_abstractions::IncommingPackageDelegate&>
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
   * This set contains the pairs of WebSocket sessions and their roles in the
   * game of the first team.
   */
  std::set<std::pair<WebSocketSession*, std::unique_ptr<Role>>> first_team_;

  /**
   * This mutex is used to synchronise all oprations done on the collection
   * containing the first team.
   */
  mutable std::mutex first_team_mtx_;

  /**
   * This set contains the pairs of WebSocket sessions and their roles in the
   * game of the second team.
   */
  std::set<std::pair<WebSocketSession*, std::unique_ptr<Role>>> second_team_;

  /**
   * This mutex is used to synchronise all oprations done on the collection
   * containing the  team.
   */
  mutable std::mutex second_team_mtx_;

  /**
   * This callable object is used as a callback to the asynchronous reading of
   * all clients in this game.
   */
  system_abstractions::IncommingPackageDelegate delegete_;

  /**
   * This is used to parse packages from the clients.
   */
  PackageParser package_parser_;
};

}  // namespace fusion_server