/**
 * @file http_session_tests.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the implementation of the unit tests for the HttpSession class.
 *
 * Copyright 2019 Kamil Rusin
 */

#include <memory>

#include <boost/asio.hpp>
#include <gtest/gtest.h>

#include <fusion_server/http_session.hpp>

struct HttpSessionTest : public ::testing::Test {
  HttpSessionTest() : ioc_{nullptr} {}

  void SetUp() override {
    ioc_ = new boost::asio::io_context;
  }

  void TearDown() override {
    ioc_->stop();
    delete ioc_;
  }

  boost::asio::io_context* ioc_;
};

TEST_F(HttpSessionTest, SocketNotConnected) {
  // Arrange
  auto socket = boost::asio::ip::tcp::socket(*ioc_);

  // Act
  auto http = std::make_shared<fusion_server::HTTPSession>(std::move(socket));

  // Assert
  EXPECT_FALSE(*http);
}