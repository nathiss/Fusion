/**
 * @file player_factory.hpp
 *
 * This module is a part of Fusion Server project.
 * It declares the PlayerFactory class.
 *
 * Copyright 2019 Kamil Rusin
 */

#pragma once

#include <memory>

#include <fusion_server/json.hpp>
#include <fusion_server/ui/abstract.hpp>

namespace fusion_server::ui {

/**
 * This is the forward declaration of the Player class.
 */
class Player;

/**
 * This class represents the factory of the Player objects.
 *
 * @see fusion_server::ui::Player
 */
class PlayerFactory {
 public:
  /**
   * This structure holds the configuration of an instance of PlayerFactory class.
   */
  struct Configuration {
    /**
     * This constructor sets the default configuration.
     */
    Configuration() noexcept;

    /**
     * The id of the next player.
     */
    std::size_t next_id_;

    /**
     * The default amount of health points.
     */
    double health_default_;

    /**
     * The default player's angle.
     */
    double angle_default_;

    /**
     * This is the default player's position.
     */
    Point position_default_;

    /**
     * This is the default player's color.
     */
    Color color_default_;
  };

  /**
   * @brief Configures the object.
   * This method configures this object using the given config JSON object.
   * If the configuration was unsuccessful or it's not complete the method
   * returns false.
   *
   * @param[in] config
   *   A JSON object used to configure this instance.
   *
   * @return true
   *   The object has been successfully configured.
   *
   * @return false
   *   Configuration failed. The config object was not complete or corrupted.
   *
   * @note
   *   If the method fails, it is guaranteed that default config will be used
   *   for creating a new logger.
   */
  bool Configure(const json::JSON& config) noexcept;

  /**
   * @brief Returns configuration.
   * This method returns the configuration of this instance.
   *
   * @return
   *   The configuration of this instance is returned.
   */
  [[nodiscard]] const Configuration& GetConfiguration() const noexcept;

  /**
   * @brief Creates a Player instance.
   * This method creates a Player instance using the stored configuration.
   *
   * @param nick
   *   The nick of a new player.
   *
   * @param team_id
   *   The id of the player's team.
   *
   * @return
   *   A pointer to the new player is returned.
   */
  [[nodiscard]] std::shared_ptr<Player>
  Create(std::string nick, std::size_t team_id) noexcept;

 private:
  /**
   * This holds the configuration of this object.
   */
  Configuration configuration_;
};

}  // namespace fusion_server::ui