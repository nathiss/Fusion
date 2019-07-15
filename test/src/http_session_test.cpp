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
#include <boost/beast.hpp>
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

  boost::asio::io_context* ioc_;  // NOLINT
};

struct HttpSessionTestWithConnection : public ::testing::Test {
  HttpSessionTestWithConnection() : ioc_{nullptr} {}

  void SetUp() override {
    ioc_ = new boost::asio::io_context;
    work_ = new boost::asio::executor_work_guard<boost::asio::io_context::executor_type>(
      ioc_->get_executor()
      );
    client_endpoint_ = new boost::asio::ip::tcp::socket{*ioc_};
    thread_ = new std::thread([this]{ ioc_->run(); });

    auto endpoint = boost::asio::ip::tcp::endpoint{
      boost::asio::ip::make_address_v4("127.0.0.1"), 9001
    };
    boost::asio::ip::tcp::acceptor acceptor{*ioc_};
    acceptor.open(endpoint.protocol());
    acceptor.set_option(boost::asio::socket_base::reuse_address(true));
    acceptor.bind(endpoint);
    acceptor.listen();

    client_endpoint_->connect(endpoint);
    server_endpoint_ = new boost::asio::ip::tcp::socket{acceptor.accept()};
  }

  void TearDown() override {
    ioc_->stop();
    thread_->join();
    delete thread_;
    delete work_;
    delete ioc_;
    delete client_endpoint_;
    delete server_endpoint_;
  }

  std::thread* thread_;

  boost::asio::io_context* ioc_;  // NOLINT
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type>* work_;

  boost::asio::ip::tcp::socket* client_endpoint_;
  boost::asio::ip::tcp::socket* server_endpoint_;
};

struct HTTPClient {
  using Request_t = boost::beast::http::request<boost::beast::http::string_body>;

  using Response_t = boost::beast::http::response<boost::beast::http::string_body>;

  explicit HTTPClient(boost::asio::ip::tcp::socket socket)
      : socket_{std::move(socket)} {}

  void Write(const std::string& msg) {
    socket_.write_some(boost::asio::buffer(msg));
  }

  std::string Read() {
    boost::asio::streambuf buf;
    auto n = boost::asio::read_until(socket_, buf, "\r\n\r\n");
    return std::string{
      boost::asio::buffers_begin(buf.data()),
      boost::asio::buffers_begin(buf.data()) + n
    };
  }

  void Close() {
    socket_.shutdown(decltype(socket_)::shutdown_both);
    socket_.close();
  }

  boost::asio::ip::tcp::socket socket_;
  boost::beast::flat_buffer buffer_;
  Response_t response;
};

TEST_F(HttpSessionTest, SocketNotConnected) {  // NOLINT
  // Arrange
  auto socket = boost::asio::ip::tcp::socket(*ioc_);

  // Act
  auto http = std::make_shared<fusion_server::HTTPSession>(std::move(socket));

  // Assert
  EXPECT_FALSE(*http);
}

TEST_F(HttpSessionTestWithConnection, NoDataInitialyFromServer) {  // NOLINT
  // Arrange
  auto session = std::make_shared<fusion_server::HTTPSession>(
    std::move(*server_endpoint_));
  auto client = HTTPClient{std::move(*client_endpoint_)};

  // Act
  session->Run();

  // Assert
  EXPECT_EQ(0, client.socket_.available());
}

TEST_F(HttpSessionTestWithConnection, SendValidRequestGetRoot) {  // NOLINT
  // Arrange
  auto session = std::make_shared<fusion_server::HTTPSession>(
    std::move(*server_endpoint_));
  auto client = HTTPClient{std::move(*client_endpoint_)};
  session->Run();
  auto req = []{
    HTTPClient::Request_t req;
    req.method(boost::beast::http::verb::get);
    req.version(11);
    req.target("/");
    req.set(boost::beast::http::field::host, "example.com");
    req.keep_alive(false);
    req.prepare_payload();
    return req;
  }();

  // Act
  boost::beast::http::write(client.socket_, req);
  boost::beast::http::read(client.socket_, client.buffer_, client.response);

  // Assert
  EXPECT_EQ(req.version(), client.response.version());
  EXPECT_EQ(200, client.response.result_int());
  EXPECT_EQ("OK", client.response.reason());
  EXPECT_TRUE(client.response.has_content_length());
  EXPECT_NE(std::string{""}, client.response.body());
}

TEST_F(HttpSessionTestWithConnection, KeepAliveTrue) {  // NOLINT
  // Arrange
  auto session = std::make_shared<fusion_server::HTTPSession>(
    std::move(*server_endpoint_));
  auto client = HTTPClient{std::move(*client_endpoint_)};
  session->Run();
  auto req = []{
    HTTPClient::Request_t req;
    req.method(boost::beast::http::verb::get);
    req.version(11);
    req.target("/");
    req.set(boost::beast::http::field::host, "example.com");
    req.keep_alive(true);
    req.prepare_payload();
    return req;
  }();

  // Act
  boost::beast::http::write(client.socket_, req);
  boost::beast::http::read(client.socket_, client.buffer_, client.response);

  // Assert
  EXPECT_TRUE(client.response.keep_alive());
}

TEST_F(HttpSessionTestWithConnection, SendRequetGet404) {  // NOLINT
  // Arrange
  auto session = std::make_shared<fusion_server::HTTPSession>(
    std::move(*server_endpoint_));
  auto client = HTTPClient{std::move(*client_endpoint_)};
  session->Run();
  auto req = []{
    HTTPClient::Request_t req;
    req.method(boost::beast::http::verb::get);
    req.version(11);
    req.target("/this/path/does/not/exist");
    req.set(boost::beast::http::field::host, "example.com");
    req.keep_alive(true);
    req.prepare_payload();
    return req;
  }();

  // Act
  boost::beast::http::write(client.socket_, req);
  boost::beast::http::read(client.socket_, client.buffer_, client.response);

  // Assert
  EXPECT_TRUE(client.response.keep_alive());
  EXPECT_EQ(req.version(), client.response.version());
  EXPECT_EQ(404, client.response.result_int());
  EXPECT_EQ("Not Found", client.response.reason());
}

TEST_F(HttpSessionTestWithConnection, BadRequestBadVersion) {  // NOLINT
  // Arrange
  auto session = std::make_shared<fusion_server::HTTPSession>(
    std::move(*server_endpoint_));
  auto client = HTTPClient{std::move(*client_endpoint_)};
  session->Run();
  auto req = []{
    HTTPClient::Request_t req;
    req.method(boost::beast::http::verb::get);
    req.version(15);
    req.target("/this/path/does/not/exist");
    req.set(boost::beast::http::field::host, "example.com");
    req.keep_alive(true);
    req.prepare_payload();
    return req;
  }();

  // Act
  boost::beast::http::write(client.socket_, req);
  boost::beast::http::read(client.socket_, client.buffer_, client.response);

  // Assert
  EXPECT_TRUE(client.response.keep_alive());
  EXPECT_EQ(400, client.response.result_int());
  EXPECT_EQ("Bad Request", client.response.reason());
}

TEST_F(HttpSessionTestWithConnection, BadRequestMissingHostField) {  // NOLINT
  // Arrange
  auto session = std::make_shared<fusion_server::HTTPSession>(
    std::move(*server_endpoint_));
  auto client = HTTPClient{std::move(*client_endpoint_)};
  session->Run();
  auto req = []{
    HTTPClient::Request_t req;
    req.method(boost::beast::http::verb::get);
    req.version(11);
    req.target("/this/path/does/not/exist");
    req.keep_alive(true);
    req.prepare_payload();
    return req;
  }();

  // Act
  boost::beast::http::write(client.socket_, req);
  boost::beast::http::read(client.socket_, client.buffer_, client.response);

  // Assert
  EXPECT_TRUE(client.response.keep_alive());
  ASSERT_EQ(400, client.response.result_int());
  EXPECT_EQ("Bad Request", client.response.reason());
}
