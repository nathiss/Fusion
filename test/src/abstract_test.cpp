/**
 * @file abstract_test.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the implementation of the unit tests for the Point and Color
 * classes.
 *
 * Copyright 2019 Kamil Rusin
 */

#include <gtest/gtest.h>

#include <fusion_server/ui/abstract.hpp>

using namespace fusion_server;

// Point

TEST(PointTest, CreateDefaultPoint) {
  // Arrange
  ui::Point point;

  // Act

  // Assert
  EXPECT_EQ(0, point.x_);
  EXPECT_EQ(0, point.y_);
}

TEST(PointTest, SerializeDefaultPoint) {
  // Arrange
  ui::Point point;

  // Act
  auto json = point.Serialize();

  // Assert
  ASSERT_TRUE(json.type() == decltype(json)::value_t::array);
  ASSERT_EQ(2, json.size());
  EXPECT_EQ(0, json[0]);
  EXPECT_EQ(0, json[1]);
}

TEST(PointTest, DeserialiseDefaultPoint) {
  // Arrange
  ui::Point point;
  auto json = point.Serialize();

  // Act
  ASSERT_TRUE(point.Deserialize(json));

  // Assert
  EXPECT_EQ(0, point.x_);
  EXPECT_EQ(0, point.y_);
}

TEST(PointTest, SerializeCustomPoint) {
  // Arrange
  ui::Point point{1337, 9001};


  // Act
  auto json = point.Serialize();

  // Assert
  ASSERT_TRUE(json.type() == decltype(json)::value_t::array);
  ASSERT_EQ(2, json.size());
  EXPECT_EQ(1337, json[0]);
  EXPECT_EQ(9001, json[1]);
}

TEST(PointTest, DeserializeCustomPoint) {
  // Arrange
  ui::Point point{1337, 9001};
  ui::Point testee;


  // Act
  auto json = point.Serialize();
  ASSERT_TRUE(testee.Deserialize(json));

  // Assert
  EXPECT_EQ(1337, testee.x_);
  EXPECT_EQ(9001, testee.y_);
}

// Color

TEST(PointTest, CreateDefaultColor) {
  // Arrange
  ui::Color color;

  // Act

  // Assert
  EXPECT_EQ(0, color.r_);
  EXPECT_EQ(0, color.g_);
  EXPECT_EQ(0, color.b_);
}

TEST(PointTest, SerializeDefaultColor) {
  // Arrange
  ui::Color color;

  // Act
  auto json = color.Serialize();

  // Assert
  ASSERT_TRUE(json.type() == decltype(json)::value_t::array);
  ASSERT_EQ(3, json.size());
  EXPECT_EQ(0, json[0]);
  EXPECT_EQ(0, json[1]);
  EXPECT_EQ(0, json[2]);
}

TEST(PointTest, DeserialiseDefaultColor) {
  // Arrange
  ui::Color color;
  auto json = color.Serialize();

  // Act
  ASSERT_TRUE(color.Deserialize(json));

  // Assert
  EXPECT_EQ(0, color.r_);
  EXPECT_EQ(0, color.g_);
  EXPECT_EQ(0, color.b_);
}

TEST(PointTest, SerializeCustomColor) {
  // Arrange
  ui::Color color{255, 0, 127};


  // Act
  auto json = color.Serialize();

  // Assert
  ASSERT_TRUE(json.type() == decltype(json)::value_t::array);
  ASSERT_EQ(3, json.size());
  EXPECT_EQ(255, json[0]);
  EXPECT_EQ(0, json[1]);
  EXPECT_EQ(127, json[2]);
}

TEST(PointTest, DeserializeCustomColor) {
  // Arrange
  ui::Color color{255, 0, 127};
  ui::Color testee;


  // Act
  auto json = color.Serialize();
  ASSERT_TRUE(testee.Deserialize(json));

  // Assert
  EXPECT_EQ(255, testee.r_);
  EXPECT_EQ(0, testee.g_);
  EXPECT_EQ(127, testee.b_);
}