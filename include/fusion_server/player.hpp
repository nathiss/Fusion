/**
 * @file player.hpp
 *
 * This module is a part of Fusion Server project.
 * It declares all UI types.
 *
 * (c) 2019 by Kamil Rusin
 */

#pragma once

#include <cstdint>

#include <optional>
#include <string>

#include <fusion_server/package_parser.hpp>

namespace fusion_server {

/**
 * This structure is used to represent colors on the map.
 */
struct Color {
  /**
   * This is red channel.
   */
  std::uint8_t r{};

  /**
   * This is green channel.
   */
  std::uint8_t g{};

  /**
   * This is blue channel.
   */
  std::uint8_t b{};

  /**
   * This method returns a dump of this object encoded as JSON.
   *
   * @return
   *  A dump of this object encoded as JSON is returned.
   */
  PackageParser::JSON ToJson() const noexcept {
    PackageParser::JSON ret = PackageParser::JSON::array();
    ret.push_back(r);
    ret.push_back(g);
    ret.push_back(b);
    return ret;
  }
};

/**
 * This structure represents a point on the map.
 */
struct Point {
  /**
   * This is the x coordinate of this point.
   */
  int64_t x{};

  /**
   * This is the y coordinate of this point.
   */
  int64_t y{};

  /**
   * @brief Extract data from a JSON object.
   * This method sets the inner fields of this object extracted from the given
   * JSON object. It returns a reference to this object.
   *
   * @param[in] array
   *   This is the JSON object containing values under `x` and `y` keys.
   *
   * @return
   *   Reference to this object is returned.
   */
  Point& operator=(const PackageParser::JSON& array) noexcept {
    x = array[0];
    y = array[1];
    return *this;
  }

  /**
   * This method returns a dump of this object encoded as JSON.
   *
   * @return
   *  A dump of this object encoded as JSON is returned.
   */
  PackageParser::JSON ToJson() const noexcept {
    PackageParser::JSON ret = PackageParser::JSON::array();
    ret.push_back(x);
    ret.push_back(y);
    return ret;
  }
};

/**
 * This class represents a ray in a game.
 */
class Ray {
 public:
  /**
   * This is the unique id of this ray.
   */
  std::size_t id_{};

  /**
   * This is the source point of this ray.
   */
  Point src_{};

  /**
   * This is the destination point of this ray.
   */
  Point dst_{};

  /**
   * This is the color of this ray.
   */
  Color color_{};

  /**
   * This is the gradient of this ray.
   */
  std::optional<double> gradient_;

  /**
   * This is the intercept of this ray.
   */
  std::optional<double> intercept_;

  /**
   * This method returns a dump of this object encoded as JSON.
   *
   * @return
   *  A dump of this object encoded as JSON is returned.
   */
  PackageParser::JSON ToJson() const noexcept {
    PackageParser::JSON ret = PackageParser::JSON::object();
    ret["id"] = id_;
    ret["source"] = src_.ToJson();
    ret["destination"] = dst_.ToJson();
    ret["color"] = color_.ToJson();
    return ret;
  }
};

/**
 * This class represents a player in a game.
 */
class Player {
 public:
  /**
   * This is the default constructor.
   */
  Player() noexcept = default;

  /**
   * This constructor sets the id of this player.
   *
   * @param[in] id
   *   The given id of this player.
   */
  explicit Player(std::size_t id) noexcept : id_{id} {}

  /**
   * This is the unique id of this ray.
   */
  std::size_t id_{};

  /**
   * This is the player's team id.
   */
  std::size_t team_id_{};

  /**
   * This stores the player's nick.
   */
  std::string nick_;

  /**
   * This stores the player's health points.
   */
  int32_t health_{100};

  /**
   * This is the player's position on the map.
   */
  Point position_{};

  /**
   * This is the player's angle (relative to the y axis).
   */
  double angle_{};

  /**
   * This is the color of this player.
   */
  Color color_;

  /**
   * This indicates whether or not the player has the laser active.
   */
  bool is_firing_{false};

  /**
   * This method returns a dump of this object encoded as JSON.
   *
   * @return
   *  A dump of this object encoded as JSON is returned.
   */
  PackageParser::JSON ToJson() const noexcept {
    PackageParser::JSON ret = PackageParser::JSON::object();
    ret["player_id"] = id_;
    ret["team_id"] = team_id_;
    ret["nick"] = nick_;
    ret["health"] = health_;
    ret["color"] = color_.ToJson();
    ret["position"] = position_.ToJson();
    ret["angle"] = angle_;
    return ret;
  }
};

}  // namespace fusion_server