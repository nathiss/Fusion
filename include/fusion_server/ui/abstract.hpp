/**
 * @file abstract.hpp
 *
 * This module is a part of Fusion Server project.
 * It declares all abstract structures used in UI.
 *
 * Copyright 2019 Kamil Rusin
 */

#pragma once

#include <cstdint>

#include <fusion_server/json.hpp>

namespace fusion_server::ui {

/**
 * This structure represents a single point.
 */
struct Point {
  /**
   * This is the x coordinate of this point.
   */
  int64_t x_{};

  /**
   * This is the y coordinate of this point.
   */
  int64_t y_{};

  /**
   * @brief Serializes this object.
   * This method serializes this object into a JSON object.
   *
   * @return
   *   This object serialized into a JSON object.
   */
  [[nodiscard]] json::JSON Serialize() const noexcept {
    return json::JSON({x_, y_}, false, json::JSON::value_t::array);
  }

  /**
   * @brief Deserializes this object from a JSON object.
   * This method loads the inner state of this object from the given JSON
   * object.
   *
   * @param json
   *   Object of this class serialised into a JSON object.
   *
   * @return
   *   An indication of whether or not the deserialization went successful.
   */
  bool Deserialize(const json::JSON& json) noexcept {
    if (json.type() != json::JSON::value_t::array) {
      return false;
    }

    if (json.size() != 2) {
      return false;
    }

    x_ = json[0];
    y_ = json[1];

    return true;
  }
};

/**
 * This structure represents color.
 */
struct Color {
  /**
   * This is red channel.
   */
  uint8_t r_{};

  /**
   * This is green channel.
   */
  uint8_t g_{};

  /**
   * This is blue channel.
   */
  uint8_t b_{};

  /**
   * @brief Serializes this object.
   * This method serializes this object into a JSON object.
   *
   * @return
   *   This object serialized into a JSON object.
   */
  [[nodiscard]] json::JSON Serialize() const noexcept {
    return json::JSON({r_, g_, b_}, false, json::JSON::value_t::array);
  }

  /**
   * @brief Deserializes this object from a JSON object.
   * This method loads the inner state of this object from the given JSON
   * object.
   *
   * @param json
   *   Object of this class serialised into a JSON object.
   *
   * @return
   *   An indication of whether or not the deserialization went successful.
   */
  bool Deserialize(const json::JSON& json) noexcept {
    if (json.type() != json::JSON::value_t::array) {
      return false;
    }

    if (json.size() != 3) {
      return false;
    }

    r_ = json[0];
    g_ = json[1];
    b_ = json[2];

    return true;
  }
};

}  // namespace fusion_server::ui