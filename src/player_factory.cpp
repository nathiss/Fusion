/**
 * @file player_factory.cpp
 *
 * This module is a part of Fusion Server project.
 * It defines the PlayerFactory class.
 *
 * Copyright 2019 Kamil Rusin
 */

#include <fusion_server/ui/player_factory.hpp>
#include <fusion_server/ui/player.hpp>

namespace fusion_server::ui {

PlayerFactory::Configuration::Configuration() noexcept
  : next_id_{0}, health_default_{100.0}, angle_default_{0} {}

bool PlayerFactory::Configure(const json::JSON& config) noexcept {
  if (!config.is_object()) {
    return false;
  }

  if(config.contains("health")) {
    if (config["health"].type() != json::JSON::value_t::number_float) {
      return false;
    }
    configuration_.health_default_ = config["health"];
  }

  if(config.contains("angle")) {
    if (config["angle"].type() != json::JSON::value_t::number_float) {
      return false;
    }
    configuration_.angle_default_ = config["angle"];
  }

  // TODO(nathiss): configure color & position

  return true;
}

auto PlayerFactory::GetConfiguration() const noexcept
-> const PlayerFactory::Configuration&  {
  return configuration_;
}

std::shared_ptr<Player>
PlayerFactory::Create(std::string nick, std::size_t team_id) noexcept {
  auto p = std::make_shared<Player>();

  p->id_ = configuration_.next_id_++;
  p->team_id_ = team_id;
  p->nick_ = std::move(nick);
  p->health_ = configuration_.health_default_;
  p->angle_ = configuration_.angle_default_;
  p->position_ = configuration_.position_default_;
  p->color_ = configuration_.color_default_;

  return p;
}

}  // namespace fusion_server::ui