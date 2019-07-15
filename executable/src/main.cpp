/**
 * @file main.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the implementation of main function.
 *
 * Copyright 2019 Kamil Rusin
 */

#include <cstring>

#include <chrono>
#include <fstream>
#include <functional>
#include <memory>
#include <thread>
#include <vector>
#include <optional>

#include <boost/asio.hpp>

#include <fusion_server/server.hpp>
#include <fusion_server/json.hpp>
#include <fusion_server/system_abstractions.hpp>
#include <fusion_server/logger_manager.hpp>

using namespace fusion_server;

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
  auto& server = Server::GetInstance();
  auto logger = server.GetLogger();
  if (ec) {
    logger->error("An error occurred during signal handling. [Boost: {}]",
      ec.message());
  }
  logger->warn("Received a signal ({}). Stopping the I/O context.",
    strsignal(signal));

  ioc.stop();
  server.Shutdown();
}

/**
 * @brief Reads the config.
 * This method read the config file and parses its content to a JSON object.
 *
 * @param file_name
 *   A path to the config file.
 *
 * @return
 *   If the parsing was successful it returns the parsed object, otherwise it
 *   returns an "invalid-state" object.
 */
std::optional<json::JSON> ReadConfig(const char* file_name) noexcept {
  std::ifstream file{file_name};
  std::string content{
    std::istreambuf_iterator<char>{file},
    std::istreambuf_iterator<char>{}
  };
  if (!file) {
    return {};
  }
  return json::Parse(
    content.begin(),
    content.end()
  );
}

/**
 * @brief The program's entry point.
 * This function is the program's entry point. It creates the server instance,
 * registers handled signals and creates worker threads.
 *
 * @param[in] argc
 *   The amount of command-line arguments.
 *
 * @param[in] argv
 *   The array of command-line arguments.
 *
 * @return 0
 *   No error occurred.
 *
 * @return
 *   An error number.
 */
int main(int argc, char** argv) {
  if (argc != 2) {
    LoggerManager::Get()->error("Usage: ./FusionServer /path/to/config");
    return EXIT_FAILURE;
  }

  auto config = ReadConfig(argv[1]);
  if (!config) {
    LoggerManager::Get()->error("The config file was ill-formed.");
    return EXIT_FAILURE;
  }

  std::size_t number_of_workers = std::thread::hardware_concurrency() - 1;
  auto& server = Server::GetInstance();

  if (!config.value().contains("number_of_additional_threads")) {
    server.GetLogger()->critical("[Config] Field \"number_of_additional_threads\" is required.");
    return EXIT_FAILURE;
  }

  if (!config.value()["number_of_additional_threads"].is_number_integer()) {
    server.GetLogger()->critical("[Config] A value of \"number_of_additional_threads\" field must be an integer.");
    return EXIT_FAILURE;
  }

  if (config.value()["number_of_additional_threads"] >= 0) {
    number_of_workers = config.value()["number_of_additional_threads"];
  }

  if (!server.Configure(std::move(config.value()))) {
    return EXIT_FAILURE;
  }

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
  server.GetLogger()->info("Registered the signal handler.");

  std::vector<std::thread> workers;
  workers.reserve(number_of_workers);
  for (std::size_t i = 0; i < number_of_workers; i++)
    workers.emplace_back([&ioc]{ ioc.run(); });
  server.GetLogger()->info("Created {} threads.", number_of_workers);

  ioc.run();

  server.GetLogger()->info("No more tasks. Waiting for threads to join.");
  for (auto& worker : workers) {
    auto id = worker.native_handle();
    worker.join();
    server.GetLogger()->info("Worker {} has joined.", id);
  }

  return EXIT_SUCCESS;
}
