/**
 * @file listener.cpp
 *
 * This module is a part of Fusion Server project.
 * It contains the implementation of the Listener class.
 *
 * Copyright 2019 Kamil Rusin
 */

#include <cstdint>

#include <memory>
#include <string_view>
#include <utility>

#include <boost/asio.hpp>

#include <fusion_server/http_session.hpp>
#include <fusion_server/listener.hpp>

namespace fusion_server {

Listener::Listener(boost::asio::io_context &ioc) noexcept
  : ioc_{ioc}, acceptor_{ioc}, socket_{ioc_}, is_open_{false}, logger_{LoggerManager::Get()} {
  configuration_.number_of_connections_ = 0;
  configuration_.max_queued_connections_ = boost::asio::socket_base::max_listen_connections;
}


bool Listener::Configure(const json::JSON& config) noexcept {
  if (!config.contains("max_queued_connections")) {
    logger_->critical("[Config::Listener] A config object must have \"max_queued_connections\" field.");
    return false;
  }
  if (!config["max_queued_connections"].is_number_integer()) {
    logger_->critical("[Config::Listener] A value of \"max_queued_connections\" must be an integer.");
    return false;
  }
  configuration_.max_queued_connections_ = config["max_queued_connections"];

  if (!config.contains("interface")) {
    logger_->critical("[Config::Listener] A config object must have \"interface\" field.");
    return false;
  }
  if (!config["interface"].is_string()) {
    logger_->critical("[Config::Listener] A value of \"interface\" must be a string.");
    return false;
  }

  if (!config.contains("port")) {
    logger_->critical("[Config::Listener] A config object must have \"port\" field.");
    return false;
  }
  if (!config["port"].is_number_unsigned()) {
    logger_->critical("[Config::Listener] A value of \"port\" must be an unsigned.");
    return false;
  }

  boost::system::error_code ec;
  configuration_.endpoint_ = {
    boost::asio::ip::make_address(config["interface"], ec),
    config["port"]
  };

  if (ec) {
    logger_->critical("[Config::Listener] A value of \"interface\" is not a valid interface. [Boost: {}]",
      ec.message());
    return false;
  }

  return true;
}

void Listener::SetLogger(LoggerManager::Logger logger) noexcept {
  logger_ = std::move(logger);
}

LoggerManager::Logger Listener::GetLogger() const noexcept {
  return logger_;
}

bool Listener::Bind() noexcept {
  return InitAcceptor();
}

bool Listener::Bind(boost::asio::ip::tcp::endpoint endpoint) noexcept {
  configuration_.endpoint_ = std::move(endpoint);
  return InitAcceptor();
}

bool Listener::Bind(std::string_view address_str, std::uint16_t port) noexcept {
  boost::system::error_code ec;
  auto address = boost::asio::ip::make_address(address_str, ec);
  if (ec) {
    logger_->error("An error occurred during address parsing. [Boost: {}]",
                   ec.message());
    return false;
  }

  configuration_.endpoint_ = {std::move(address), port};
  return InitAcceptor();
}

bool Listener::Bind(std::uint16_t port) noexcept {
  configuration_.endpoint_ = {boost::asio::ip::tcp::v4(), port};
  return InitAcceptor();
}


const boost::asio::ip::tcp::endpoint &Listener::GetEndpoint() const noexcept {
  return configuration_.endpoint_;
}

bool Listener::Run() noexcept {
  if (!is_open_) {
    return false;
  }

  logger_->info("Starting asynchronous accepting on {}.", configuration_.endpoint_);
  acceptor_.async_accept(
    socket_,
    [self = shared_from_this()](const boost::system::error_code &ec) {
      self->HandleAccept(ec);
    });
  return true;
}

std::size_t Listener::GetNumberOfConnections() const noexcept {
  return configuration_.number_of_connections_;
}

std::size_t Listener::GetMaxQueuedConnections() const noexcept {
  return decltype(acceptor_)::max_listen_connections;
}

void Listener::HandleAccept(const boost::system::error_code& ec) noexcept {
  if (ec == boost::asio::error::already_open) {
    // This means the socket used to handle new connections in already open.
    // We sacrifice one connection to make the acceptor work properly.
    logger_->warn("The socket used to accept new connections is already open.");
    socket_.close();
  }

  if (ec == boost::asio::error::no_recovery) {
    logger_->error("A non-recoverable error occurred during handling a new connection. [Boost:{}]",
                   ec.message());
    return;
  }
  if (ec) {
    logger_->error("An error occurred during handling a new connection. [Boost:{}]",
                   ec.message());
    // TODO(nathiss): find out if any other error can cause the listener to stop working.
  } else {
    logger_->debug("Accepted a new connection from {}.", socket_.remote_endpoint());
    configuration_.number_of_connections_++;
    std::make_shared<HTTPSession>(std::move(socket_))->Run();
  }

  acceptor_.async_accept(
    socket_,
    [self = shared_from_this()](const boost::system::error_code& ec) {
      self->HandleAccept(ec);
  });
}

bool Listener::InitAcceptor() noexcept {
  boost::system::error_code ec;

  acceptor_.open(configuration_.endpoint_.protocol(), ec);
  if (ec) {
    logger_->error("Open: {}", ec.message());
    return false;
  }

  acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec) {
    logger_->error("Set option (reuse address): {}", ec.message());
    return false;
  }

  acceptor_.bind(configuration_.endpoint_, ec);
  if (ec == boost::asio::error::access_denied) {
    logger_->error("Cannot bind acceptor to {} (permission denied).",
                   configuration_.endpoint_);
    return false;
  }
  if (ec == boost::asio::error::address_in_use) {
    logger_->error("Cannot bind acceptor to {} (address in use).",
                   configuration_.endpoint_);
    return false;
  }
  if (ec) {
    logger_->error("Bind: {}", ec.message());
    return false;
  }

  acceptor_.listen(configuration_.max_queued_connections_, ec);
  if (ec) {
    logger_->error("Listen: {}", ec.message());
    return false;
  }

  is_open_ = true;
  logger_->info("Acceptor is bind to {}.", configuration_.endpoint_);
  return true;
}

}  // namespace fusion_server
