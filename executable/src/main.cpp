/**
 * @file main.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the implementation of main function.
 *
 * (c) 2019 by Kamil Rusin
 */

#include <cstring>

#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include <boost/asio.hpp>
#include <spdlog/sinks/basic_file_sink.h>

#include <fusion_server/server.hpp>
#include <fusion_server/system_abstractions.hpp>

/**
 * @brief Asynchronous signal handler.
 * This function used as the asynchronous signal handler.
 *
 * @param ioc
 *   The I/O context used in the program.
 *
 * @param ec
 *   The Boost's error code that indicates whether or not an error occurred.
 *
 * @param signal
 *   The signal code of the received signal.
 */
void HandleSignal(boost::asio::io_context& ioc,
  const boost::system::error_code& ec, int signal) noexcept {
  if (ec) {
    spdlog::get("server")->error("An error occurred during signal handling. [Boost: {}]",
      ec.message());
  }
  spdlog::get("server")->warn("Received a signal ({}). Stopping the I/O context.",
    strsignal(signal));

  ioc.stop();
  fusion_server::Server::GetInstance().Shutdown();
}

/**
 * @brief Initialises loggers.
 * This function initialises loggers for main modules of the program.
 */
void InitLogger() noexcept {
  namespace fs = fusion_server::system_abstractions;

  // TODO: read the path from config file
  auto all_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/all.log", true);
  all_file_sink->set_pattern(fs::kLoggerPattern);

  fs::CreateLogger("server", true, all_file_sink);
  fs::CreateLogger("listener", true, all_file_sink);
  fs::CreateLogger("http", true, all_file_sink);
  fs::CreateLogger("websocket", true, all_file_sink);
}

/**
 * @brief The program's entry point.
 * This function is the program's entry point. It creates the server instance,
 * registers handled signals and creates worker threads.
 *
 * @return 0
 *   No error occurred.
 *
 * @return
 *   A error number.
 */
int main() {
  InitLogger();

  auto& server = fusion_server::Server::GetInstance();
  if (!server.StartAccepting()) {
    return EXIT_FAILURE;
  }

  auto& ioc = server.GetIOContext();

  boost::asio::signal_set signals{ioc, SIGINT, SIGTERM};
  signals.async_wait(
    [&ioc] (const boost::system::error_code& ec, int signal) {
      HandleSignal(ioc, ec, signal);
    }
  );

  // TODO: read from config file
  auto number_of_workers = std::thread::hardware_concurrency() - 1;
  std::vector<std::thread> workers;
  workers.reserve(number_of_workers);

  for (std::size_t i = 0; i < number_of_workers; i++)
    workers.emplace_back([&ioc]{ ioc.run(); });

  spdlog::get("server")->info("Created {} threads.", number_of_workers);

  ioc.run();

  spdlog::get("server")->info("No more tasks. Waiting for threads to join.");

  for (auto& worker : workers) {
    auto id = worker.native_handle();
    worker.join();
    spdlog::get("server")->info("Worker {} has joined.", id);
  }

  return EXIT_SUCCESS;
}
