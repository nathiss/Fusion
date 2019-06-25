#include <thread>

#include <gtest/gtest.h>

#include <fusion_server/listener.hpp>
#include <fusion_server/server.hpp>

TEST(aa, bb) {
  fusion_server::Server::GetInstance().StartAccepting();
  auto& ioc = fusion_server::Server::GetInstance().GetIOContext();





  ioc.run();
  std::thread([&ioc]{ ioc.run(); }).detach();
  std::thread([&ioc]{ ioc.run(); }).detach();
  std::thread([&ioc]{ ioc.run(); }).detach();
}