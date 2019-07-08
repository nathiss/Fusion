#pragma once

#include <cstdint>

#include <optional>
#include <string>

namespace fusion_server {

/**
 * This structure is used to represent colors on the map.
 */
struct Color {
  /**
   * This is red channel.
   */
  uint8_t r{};

  /**
   * This is green channel.
   */
  uint8_t g{};

  /**
   * This is blue channel.
   */
  uint8_t b{};

  /**
   * This method returns a dump of this object encoded as JSON.
   *
   * @return
   *  A dump of this object encoded as JSON is returned.
   */
  std::string ToJson() const noexcept {
    std::string ret{"["};
    ret += std::to_string(r);
    ret += ", ";
    ret += std::to_string(g);
    ret += ", ";
    ret += std::to_string(b);
    ret += "]";
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
   * This method returns a dump of this object encoded as JSON.
   *
   * @return
   *  A dump of this object encoded as JSON is returned.
   */
  std::string ToJson() const noexcept {
    std::string ret{"["};
    ret += std::to_string(x);
    ret += ", ";
    ret += std::to_string(y);
    ret += "]";
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
  std::string ToJson() const noexcept {
    std::string ret{"{"};
    ret += "\"id\": " + std::to_string(id_) + ",";
    ret += "\"source\": " + src_.ToJson() + ",";
    ret += "\"destination\": " + dst_.ToJson() + ",";
    ret += "\"color\": " + color_.ToJson();
    ret += "}";
    return ret;
  }
};

/**
 * This class represents a player in a game.
 */
class Player {
 public:
  /**
   * This is the unique id of this ray.
   */
  std::size_t id_{};

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
  std::string ToJson() const noexcept {
    std::string ret{"{"};
    ret += "\"id\": " + std::to_string(id_) + ",";
    ret += "\"nick\": " + nick_ + ",";
    ret += "\"health\": " + std::to_string(health_) + ",";
    ret += "\"color\": " + color_.ToJson() + ", ";
    ret += "\"position\": " + position_.ToJson() + ", ";
    ret += "\"angle\": " + std::to_string(angle_);
    ret += "}";
    return ret;
  }
};

}  // namespace fusion_server