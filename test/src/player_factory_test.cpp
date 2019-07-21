/**
 * @file player_factory_test.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the implementation of the unit tests for the PlayerFactory class.
 *
 * Copyright 2019 Kamil Rusin
 */

#include <gtest/gtest.h>

#include <fusion_server/ui/player_factory.hpp>
#include <fusion_server/ui/player.hpp>
#include <fusion_server/ui/abstract.hpp>

using namespace fusion_server;

TEST(PlayerFactoryTest, CreatePlayerWithDefaultConfigTestColor) {
  // Arrange
  ui::PlayerFactory factory;
  ui::Color color;
  auto config = factory.GetConfiguration();

  // Act
  auto player = factory.Create("nick", 1);
  color.Deserialize(player->Serialize()["color"]);

  // Assert
  EXPECT_EQ(config.color_default_, color);
}

TEST(PlayerFactoryTest, CreatePlayerWithDefaultConfigTestHealth) {
  // Arrange
  ui::PlayerFactory factory;
  auto config = factory.GetConfiguration();

  // Act
  auto player = factory.Create("nick", 1);

  // Assert
  EXPECT_EQ(config.health_default_, player->Serialize()["health"]);
}

TEST(PlayerFactoryTest, CreatePlayerWithDefaultConfigTestAngle) {
  // Arrange
  ui::PlayerFactory factory;
  auto config = factory.GetConfiguration();

  // Act
  auto player = factory.Create("nick", 1);

  // Assert
  EXPECT_EQ(config.angle_default_, player->Serialize()["angle"]);
}

TEST(PlayerFactoryTest, CreatePlayerWithDefaultConfigTestPoint) {
  // Arrange
  ui::PlayerFactory factory;
  ui::Point point;
  auto config = factory.GetConfiguration();

  // Act
  auto player = factory.Create("nick", 1);
  point.Deserialize(player->Serialize()["color"]);

  // Assert
  EXPECT_EQ(config.position_default_, point);
}

TEST(PlayerFactoryTest, PassedValidConfigurationIsSaved) {
  // Arrange
  ui::PlayerFactory factory;
  auto json = json::JSON({
                           {"health", 56.7},
                           {"angle", 3.14},
                           {"position", json::JSON({-9001, 9001})},
                           {"color", json::JSON({65, 126, 13})},
    }, false, json::JSON::value_t::object);

  // Act
  ASSERT_TRUE(factory.Configure(json));

  // Assert
  auto result = factory.GetConfiguration();
  EXPECT_EQ(56.7, result.health_default_);
  EXPECT_EQ(3.14, result.angle_default_);
  EXPECT_EQ(ui::Point({-9001, 9001}), result.position_default_);
  EXPECT_EQ(ui::Color({65, 126, 13}), result.color_default_);
}

TEST(PlayerFactoryTest, PassedValidConfigurationAndCreateNewPlayer) {
  // Arrange
  ui::PlayerFactory factory;
  auto json = json::JSON({
                           {"health", 56.7},
                           {"angle", 3.14},
                           {"position", json::JSON({-9001, 9001})},
                           {"color", json::JSON({65, 126, 13})},
                         }, false, json::JSON::value_t::object);
  factory.Configure(json);

  // Act
  auto player = factory.Create("nick", 1337);

  // Assert
  auto result = player->Serialize();
  EXPECT_EQ(56.7, result["health"]);
  EXPECT_EQ(3.14, result["angle"]);
  EXPECT_TRUE(json::JSON({-9001, 9001}) == result["position"]);
  EXPECT_TRUE(json::JSON({65, 126, 13}) == result["color"]);
}

// TODO(nathiss): complete tests