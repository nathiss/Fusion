#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include <boost/asio.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <fusion_server/server.hpp>

void HandleSignal(boost::asio::io_context& ioc,
  const boost::system::error_code& ec, int signal) noexcept {
  if (ec) {
    spdlog::get("server")->error("An error occured during signal handling. [Boost: {}]",
      ec.message());
  }
  spdlog::get("server")->warn("Received a signal ({}). Stoping the io_context.",
    signal);
  ioc.stop();
}

void InitLogger() noexcept {
  auto pattern = std::string{"[%H:%M:%S:%e] [thread %t] [%^%l@%n%$] %v"};
#ifdef DEBUG
  auto console_log_level = spdlog::level::debug;
#else
  auto console_log_level = spdlog::level::warn;
#endif


  // All
  auto all_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/all.log", true);
  all_file_sink->set_pattern(pattern);

  // Server
  auto server_console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  server_console_sink->set_level(console_log_level);
  server_console_sink->set_pattern(pattern);

  auto server_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/server.log", true);
  server_file_sink->set_level(spdlog::level::debug);
  server_file_sink->set_pattern(pattern);

  spdlog::sinks_init_list server_sink_list{server_console_sink, server_file_sink, all_file_sink};

  auto server_logger = std::make_shared<spdlog::logger>("server", std::move(server_sink_list));
  server_logger->set_level(spdlog::level::debug);
  server_logger->flush_on(console_log_level);
  spdlog::register_logger(server_logger);

  // Listener
  auto listener_console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  listener_console_sink->set_level(console_log_level);
  listener_console_sink->set_pattern(pattern);

  auto listener_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/listener.log", true);
  listener_file_sink->set_level(spdlog::level::debug);
  listener_file_sink->set_pattern(pattern);

  spdlog::sinks_init_list listener_sink_list{listener_console_sink, listener_file_sink, all_file_sink};

  auto listener_logger = std::make_shared<spdlog::logger>("listener", std::move(listener_sink_list));
  listener_logger->set_level(spdlog::level::debug);
  listener_logger->flush_on(console_log_level);
  spdlog::register_logger(listener_logger);

  // Game
  auto game_console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  game_console_sink->set_level(console_log_level);
  game_console_sink->set_pattern(pattern);

  auto game_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/game.log", true);
  game_file_sink->set_level(spdlog::level::debug);
  game_file_sink->set_pattern(pattern);

  spdlog::sinks_init_list game_sink_list{game_console_sink, game_file_sink, all_file_sink};

  auto game_logger = std::make_shared<spdlog::logger>("game", std::move(game_sink_list));
  game_logger->set_level(spdlog::level::debug);
  game_logger->flush_on(console_log_level);
  spdlog::register_logger(game_logger);

  // HTTP
  auto http_console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  http_console_sink->set_level(console_log_level);
  http_console_sink->set_pattern(pattern);

  auto http_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/http.log", true);
  http_file_sink->set_level(spdlog::level::debug);
  http_file_sink->set_pattern(pattern);

  spdlog::sinks_init_list http_sink_list{http_console_sink, http_file_sink, all_file_sink};

  auto http_logger = std::make_shared<spdlog::logger>("http", std::move(http_sink_list));
  http_logger->set_level(spdlog::level::debug);
  http_logger->flush_on(console_log_level);
  spdlog::register_logger(http_logger);

  // WebSocket
  auto websocket_console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  websocket_console_sink->set_level(console_log_level);
  websocket_console_sink->set_pattern(pattern);

  auto websocket_file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/websocket.log", true);
  websocket_file_sink->set_level(spdlog::level::debug);
  websocket_file_sink->set_pattern(pattern);

  spdlog::sinks_init_list websocket_sink_list{websocket_console_sink, websocket_file_sink, all_file_sink};

  auto websocket_logger = std::make_shared<spdlog::logger>("websocket", std::move(websocket_sink_list));
  websocket_logger->set_level(spdlog::level::debug);
  websocket_logger->flush_on(console_log_level);
  spdlog::register_logger(websocket_logger);
}

int main() {
  InitLogger();

  auto& server = fusion_server::Server::GetInstance();
  server.StartAccepting();

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
    auto id = worker.get_id();
    worker.join();
    spdlog::get("server")->info("Worker {} has joined.", id);
  }

  return EXIT_SUCCESS;
}