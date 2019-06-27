#include <functional>
#include <iostream>
#include <thread>
#include <vector>

#include <boost/asio.hpp>

#include <fusion_server/server.hpp>

void HandleSignal(boost::asio::io_context& ioc,
  const boost::system::error_code& ec, int signal) noexcept {
  if (ec) {
    std::cerr << "HandleSignal: " << ec.message() << std::endl;
    // We do nothing, since we want to exit.
  }
  std::cerr << "[Warning] Received signal: " << signal << ". Shutting down." << std::endl;
  ioc.stop();
}

int main() {
  auto& server = fusion_server::Server::GetInstance();
  server.StartAccepting();

  auto& ioc = server.GetIOContext();

  boost::system::error_code ec;
  boost::asio::signal_set signals{ioc};
  signals.add(SIGINT, ec);
  if (ec) {
    std::cerr << "Error: " << ec.message() << std::endl;
    return EXIT_FAILURE;
  }
  signals.add(SIGTERM, ec);
  if (ec) {
    std::cerr << "Error: " << ec.message() << std::endl;
    return EXIT_FAILURE;
  }
  signals.async_wait(
    [&ioc] (const boost::system::error_code& ec, int signal) {
      HandleSignal(ioc, ec, signal);
    }
  );

  std::vector<std::thread> workers;
  workers.reserve(std::thread::hardware_concurrency() - 1);

  for (std::size_t i = 0; i < std::thread::hardware_concurrency() - 1; i++)
    workers.emplace_back([&ioc]{ ioc.run(); });

  ioc.run();

  for (auto& worker : workers)
    worker.join();

  return EXIT_SUCCESS;
}