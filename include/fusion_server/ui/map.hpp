/**
 * @file map.hpp
 *
 * This module is a part of Fusion Server project.
 * It declares the Map class.
 *
 * Copyright 2019 Kamil Rusin
 */

#pragma once

namespace fusion_server::ui {

/**
 * This enumerates all possible directions in which players can move.
 */
enum Direction {
  /**
   * This indicates that a player is not moving.
   */
  none = 0x0,

  /**
   * This indicates that a player is moving up.
   */
  up = 0x1,

  /**
   * This indicates that a player is moving right.
   */
  right = 0x2,

  /**
   * This indicates that a player is moving down.
   */
  down = 0x4,

  /**
   * This indicates that a player is moving left.
   */
  left = 0x8,
};

/**
 * This class represents the map loaded in a game.
 */
class Map {};

}  // namespace fusion_server::ui