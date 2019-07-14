#include <memory>
#include <vector>

#include <boost/asio.hpp>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <fusion_server/listener.hpp>

struct ListenerTest : public ::testing::Test {
  ListenerTest() noexcept : ioc_{nullptr} {}

  void SetUp() override {
    ioc_ = new boost::asio::io_context;
  }

  void TearDown() override {
    ioc_->stop();
    delete ioc_;
  }

  boost::asio::io_context *ioc_;
};

TEST_F(ListenerTest, SetLoggerCheck) {
  // Arrange
  auto listener1 = std::make_shared<fusion_server::Listener>(*ioc_);
  auto listener2 = std::make_shared<fusion_server::Listener>(*ioc_);
  auto listener3 = std::make_shared<fusion_server::Listener>(*ioc_);
  auto logger = std::make_shared<spdlog::logger>("test_logger");

  // Act
  listener2->SetLogger(logger);
  listener3->SetLogger(logger);
  listener3->SetLogger(nullptr);


  // Assert
  EXPECT_EQ(spdlog::default_logger(), listener1->GetLogger());
  EXPECT_EQ(logger, listener2->GetLogger());
  EXPECT_EQ(nullptr, listener3->GetLogger());
}

TEST_F(ListenerTest, BindToValidEndpoint) {
  // Arrange
  auto listener1 = std::make_shared<fusion_server::Listener>(*ioc_);
  auto listener2 = std::make_shared<fusion_server::Listener>(*ioc_);
  auto listener3 = std::make_shared<fusion_server::Listener>(*ioc_);
  auto endpoint = boost::asio::ip::tcp::endpoint{
    boost::asio::ip::make_address_v4("127.0.0.1"), 1337};

  // Act

  // Assert
  EXPECT_TRUE(listener1->Bind(std::move(endpoint)));
  EXPECT_TRUE(listener2->Bind("0.0.0.0", 2121));
  EXPECT_TRUE(listener3->Bind(9001));
}

TEST_F(ListenerTest, BindToNotValidEndpoint) {
  // Arrange
  auto listener1 = std::make_shared<fusion_server::Listener>(*ioc_);
  auto listener2 = std::make_shared<fusion_server::Listener>(*ioc_);
  auto listener3 = std::make_shared<fusion_server::Listener>(*ioc_);
  auto endpoint = boost::asio::ip::tcp::endpoint{
    boost::asio::ip::make_address_v4("8.8.8.8"), 1337};

  auto listener4 = std::make_shared<fusion_server::Listener>(*ioc_);
  ASSERT_TRUE(listener4->Bind(2121));

  // Act

  // Assert
  EXPECT_FALSE(listener1->Bind(std::move(endpoint)));
  EXPECT_FALSE(listener2->Bind("0.0.0.0", 21));
  EXPECT_FALSE(listener3->Bind(2121));
}

TEST_F(ListenerTest, GetEndpoint) {
  // Arrange
  auto listener1 = std::make_shared<fusion_server::Listener>(*ioc_);
  auto listener2 = std::make_shared<fusion_server::Listener>(*ioc_);
  auto listener3 = std::make_shared<fusion_server::Listener>(*ioc_);
  auto endpoint1 = boost::asio::ip::tcp::endpoint{
    boost::asio::ip::make_address_v4("127.0.0.1"), 2121};
  auto endpoint2 = boost::asio::ip::tcp::endpoint{
    boost::asio::ip::make_address_v4("127.0.0.1"), 9001};
  auto endpoint3 = boost::asio::ip::tcp::endpoint{
    boost::asio::ip::tcp::v4(), 1337};

  // Act
  listener1->Bind(endpoint1);
  listener2->Bind(endpoint2.address().to_string(), endpoint2.port());
  listener3->Bind(endpoint3.port());

  // Assert
  EXPECT_EQ(endpoint1, listener1->GetEndpoint());
  EXPECT_EQ(endpoint2, listener2->GetEndpoint());
  EXPECT_EQ(endpoint3, listener3->GetEndpoint());
}

TEST_F(ListenerTest, DoSuccessfulRun) {
  // Arrange
  auto listener = std::make_shared<fusion_server::Listener>(*ioc_);
  auto endpoint = boost::asio::ip::tcp::endpoint{
    boost::asio::ip::make_address_v4("127.0.0.1"), 2121};

  // Act
  listener->Bind(std::move(endpoint));

  // Assert
  ASSERT_TRUE(listener->Run());
}

TEST_F(ListenerTest, DoFailureRun) {
  // Arrange
  auto listener = std::make_shared<fusion_server::Listener>(*ioc_);
  auto endpoint = boost::asio::ip::tcp::endpoint{
    boost::asio::ip::make_address_v4("127.0.0.1"), 21};

  // Act
  listener->Bind(std::move(endpoint));

  // Assert
  ASSERT_FALSE(listener->Run());
}

TEST_F(ListenerTest, AcceptConnection) {
  // Arrange
  auto listener1 = std::make_shared<fusion_server::Listener>(*ioc_);
  auto listener2 = std::make_shared<fusion_server::Listener>(*ioc_);
  auto socket = boost::asio::ip::tcp::socket{*ioc_};

  // Act
  ASSERT_TRUE(listener2->Bind("127.0.0.1", 9001));
  ASSERT_TRUE(listener2->Run());
  socket.connect(listener2->GetEndpoint());
  ioc_->run_one();

  // Assert
  ASSERT_EQ(0, listener1->GetNumberOfConnections());
  ASSERT_EQ(1, listener2->GetNumberOfConnections());
}

TEST_F(ListenerTest, AcceptConnectionFromBeforeRun) {
  // Arrange
  auto listener1 = std::make_shared<fusion_server::Listener>(*ioc_);
  auto listener2 = std::make_shared<fusion_server::Listener>(*ioc_);
  auto socket = boost::asio::ip::tcp::socket{*ioc_};

  // Act
  ASSERT_TRUE(listener2->Bind("127.0.0.1", 9001));
  socket.connect(listener2->GetEndpoint());
  ASSERT_TRUE(listener2->Run());
  ioc_->run_one();

  // Assert
  ASSERT_EQ(0, listener1->GetNumberOfConnections());
  ASSERT_EQ(1, listener2->GetNumberOfConnections());
}

TEST_F(ListenerTest, AcceptMaxConnections) {
  // Arrange
  auto listener = std::make_shared<fusion_server::Listener>(*ioc_);
  auto endpoint = boost::asio::ip::tcp::endpoint{
    boost::asio::ip::make_address_v4("127.0.0.1"), 9001
  };
  auto max_connections = listener->GetMaxListenConnections();
  listener->Bind(endpoint);
  listener->Run();
  std::vector<boost::asio::ip::tcp::socket> sockets;
  sockets.reserve(max_connections);
  for (std::size_t i{}; i < sockets.capacity(); i++) {
    auto &ref = sockets.emplace_back(*ioc_);
    ref.connect(endpoint);
  }

  // Act
  for (std::size_t i{}; i < sockets.size(); i++) {
    ioc_->run_one();
  }

  // Assert
  EXPECT_EQ(max_connections, listener->GetNumberOfConnections());
}
