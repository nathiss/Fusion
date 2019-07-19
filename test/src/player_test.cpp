/**
 * @file player_test.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the implementation of the unit tests for the Player class.
 *
 * Copyright 2019 Kamil Rusin
 */

#include <gtest/gtest.h>

#include <fusion_server/ui/player.hpp>

using namespace fusion_server;

TEST(PlayerTest, SerializeIncludesAllFields) {
  // Arrange
  ui::Player player{0, 0, "", 0.0, {}, 0.0, {}};

  // Act
  auto json = player.Serialize();

  // Assert
  ASSERT_EQ(decltype(json)::value_t::object, json.type());
  EXPECT_TRUE(json.contains("player_id"));
  EXPECT_TRUE(json.contains("team_id"));
  EXPECT_TRUE(json.contains("nick"));
  EXPECT_TRUE(json.contains("color"));
  EXPECT_TRUE(json.contains("health"));
  EXPECT_TRUE(json.contains("position"));
  EXPECT_TRUE(json.contains("angle"));
}

TEST(PlayerTest, SerializeAllFieldsHaveProperTypes) {
  // Arrange
  ui::Player player{0, 0, "", 0.0, {}, 0.0, {}};

  // Act
  auto json = player.Serialize();

  // Assert
  EXPECT_EQ(decltype(json)::value_t::number_unsigned, json["player_id"].type());
  EXPECT_EQ(decltype(json)::value_t::number_unsigned, json["team_id"].type());
  EXPECT_EQ(decltype(json)::value_t::string, json["nick"].type());
  EXPECT_EQ(decltype(json)::value_t::number_float, json["health"].type());
  EXPECT_EQ(decltype(json)::value_t::number_float, json["angle"].type());

  // Color
  ASSERT_EQ(decltype(json)::value_t::array, json["color"].type());
  ASSERT_EQ(3, json["color"].size());
  EXPECT_EQ(decltype(json)::value_t::number_unsigned, json["color"][0].type());
  EXPECT_EQ(decltype(json)::value_t::number_unsigned, json["color"][1].type());
  EXPECT_EQ(decltype(json)::value_t::number_unsigned, json["color"][2].type());

  // Position
  ASSERT_EQ(decltype(json)::value_t::array, json["position"].type());
  ASSERT_EQ(2, json["position"].size());
  EXPECT_EQ(decltype(json)::value_t::number_integer, json["position"][0].type());
  EXPECT_EQ(decltype(json)::value_t::number_integer, json["position"][1].type());
}

TEST(PlayerTest, SetPositionWithSeperatedCoordinates) {
  // Arrange
  ui::Player player{0, 0, "", 0.0, {}, 0.0, {}};

  // Act
  player.SetPosition(-1337, 9001);

  // Assert
  auto json = player.Serialize();
  EXPECT_TRUE(-1337 == json["position"][0]);
  EXPECT_TRUE(9001 == json["position"][1]);
}

TEST(PlayerTest, SetPositionWithPointClassArgument) {
  // Arrange
  ui::Player player{0, 0, "", 0.0, {}, 0.0, {}};

  // Act
  player.SetPosition({-1337, 9001});

  // Assert
  auto json = player.Serialize();
  EXPECT_TRUE(-1337 == json["position"][0]);
  EXPECT_TRUE(9001 == json["position"][1]);
}

TEST(PlayerTest, SetAngle) {
  // Arrange
  ui::Player player{0, 0, "", 0.0, {}, 0.0, {}};

  // Act
  player.SetAngle(3.14);

  // Assert
  auto json = player.Serialize();
  EXPECT_EQ(3.14, json["angle"]);
}

TEST(PlayerTest, GetId) {
  // Arrange
  ui::Player player1{0, 0, "", 0.0, {}, 0.0, {}};
  ui::Player player2{1337, 0, "", 0.0, {}, 0.0, {}};

  // Act

  // Assert
  EXPECT_EQ(0, player1.GetId());
  EXPECT_EQ(1337, player2.GetId());
}

TEST(PlayerTest, GetIdTheSameAsSerialise) {
  // Arrange
  ui::Player player{1337, 0, "", 0.0, {}, 0.0, {}};
  auto json = player.Serialize();

  // Act

  // Assert
  EXPECT_EQ(1337, player.GetId());
  EXPECT_EQ(1337, json["player_id"]);
}