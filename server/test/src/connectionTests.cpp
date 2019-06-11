#include <chrono>
#include <memory>
#include <thread>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <gtest/gtest.h>

#include <fusion_server/connection.hpp>
#include <fusion_server/Iconnection.hpp>

struct ConnectionTests : ::testing::Test {
  /**
   * This is the io_context used in the testing.
   */
  boost::asio::io_context ioc_;

  /**
   * This indicates that where is work to do.
   */
  std::unique_ptr<boost::asio::io_context::work> work_;

  /**
   * This is the socket connected to the client.
   */
  boost::asio::ip::tcp::socket mock_socket_{ioc_};

  /**
   * This thread calls the ioc.run().
   */
  //std::unique_ptr<std::thread> thread_;

  virtual void SetUp() override {
    work_ = std::make_unique<boost::asio::io_context::work>(ioc_);
    //thread_ = std::make_unique<std::thread>([this]{ ioc_.run(); });

    //std::cout << "Joinable: " << thread_->joinable() << std::endl;

    boost::asio::ip::tcp::resolver resolver{ioc_};
    auto it = resolver.resolve("echo.websocket.org", "80");

    boost::asio::connect(mock_socket_, it.begin(), it.end());
  }

  virtual void TearDown() override {
    work_ = nullptr;
    //thread_->join();
  }
};

TEST_F(ConnectionTests, TestConnectionOnline) {
  // TODO: fix this
  auto connection = std::make_shared<fusion_server::Connection>(std::move(mock_socket_));
  connection->Run();

  connection->Write(std::make_shared<const std::string>("Hello World!"));
  std::thread{[this]{ ioc_.run(); }}.detach();
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  auto response = connection->Pop();

  ASSERT_EQ(std::string("Hello World!"), *response);

  connection->Close();
}