/**
 * @file player.hpp
 *
 * This module is a part of Fusion Server project.
 * It declares the Player class.
 *
 * Copyright 2019 Kamil Rusin
 */

#pragma once

#include <cstdint>
#include <cstddef>

#include <string>

#include <fusion_server/json.hpp>
#include <fusion_server/ui/abstract.hpp>

namespace fusion_server::ui {

/**
 * This is the forward declaration of the PlayerFactory class.
 */
class PlayerFactory;

/**
 * This class represents a player in a game.
 */
class Player {
 public:
  /**
   * @brief The default constructor.
   */
  Player() noexcept = default;

  /**
   * @brief Constructs a player.
   *
   * @param id
   *   The player's id.
   *
   * @param team_id
   *   The id of the player's team.
   *
   * @param nick
   *   The player's nick.
   *
   * @param health
   *   The amount of starting health points.
   *
   * @param position
   *   The spawn position of the player.
   *
   * @param angle
   *   The starting angle of player's avatar.
   *
   * @param color
   *   The color of player's avatar.
   */
  Player(std::size_t id, std::size_t team_id, std::string nick, double health,
    Point position, double angle, Color color) noexcept : id_{id},
    team_id_{team_id}, nick_{std::move(nick)}, health_{health},
    position_{position}, angle_{angle}, color_{color} {}

  /**
   * @brief Serializes this object.
   * This method serializes this object into a JSON object.
   *
   * @return
   *   This object serialized into a JSON object.
   */
  [[nodiscard]] json::JSON Serialize() const noexcept {
    return json::JSON({
                        {"player_id", id_},
                        {"team_id", team_id_},
                        {"nick", nick_},
                        {"color", color_.Serialize()},
                        {"health", health_},
                        {"position", position_.Serialize()},
                        {"angle", angle_},
      }, false, json::JSON::value_t::object);
  }

  /**
   * @brief Sets the player's position.
   * This method sets the new player's position.
   *
   * @param x
   *   The x coordinate of the new position.
   *
   * @param y
   *   The y coordinate of the new position.
   */
  void SetPosition(decltype(ui::Point::x_) x, decltype(ui::Point::y_) y) noexcept {
    SetPosition({x, y});
  }

  /**
   * @brief Sets the player's position.
   * This method sets the new player's position.
   *
   * @param new_position
   *   The new player's position.
   */
  void SetPosition(ui::Point new_position) noexcept {
    position_ = new_position;
  }

  /**
   * @brief Sets the player's angle.
   * This method sets the new player's angle.
   *
   * @param angle
   *   The new player's angle.
   */
  void SetAngle(double angle) noexcept {
    angle_ = angle;
  }

  /**
   * @brief Returns player's id.
   * This method returns the id of this player.
   *
   * @return
   *   The id of this player is returned.
   */
  [[nodiscard]] std::size_t GetId() const noexcept {
    return id_;
  }

 private:
  /**
   * This is the unique id of this player in its game.
   */
  std::size_t id_;

  /**
   * This is the id of the player's team.
   */
  std::size_t team_id_;

  /**
   * This is player's nick.
   */
  std::string nick_;

  /**
   * This holds the current health points of this player.
   */
  double health_;

  /**
   * This is the current position of this player on the map.
   */
  Point position_;

  /**
   * This is current angle of the front of this player.
   */
  double angle_;

  /**
   * This is the color of this player.
   */
  Color color_;

  /**
   * This is the declaration of friendship of this class and PlayerFactory.
   */
  friend class PlayerFactory;
};

}  // namespace fusion_server::ui