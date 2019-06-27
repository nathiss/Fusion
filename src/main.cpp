#include <thread>
#include <vector>

#include <fusion_server/server.hpp>

int main() {
  auto& server = fusion_server::Server::GetInstance();
  server.StartAccepting();

  auto& ioc = server.GetIOContext();

  std::vector<std::thread> workers;
  workers.reserve(std::thread::hardware_concurrency() - 1);

  for (std::size_t i = 0; i < std::thread::hardware_concurrency(); i++)
    workers.emplace_back([&ioc]{ ioc.run(); });

  ioc.run();

  return EXIT_SUCCESS;
}