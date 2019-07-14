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
#include <fusion_server/logger_types.hpp>

namespace fusion_server {

Listener::Listener(boost::asio::io_context &ioc) noexcept
  : ioc_{ioc}, acceptor_{ioc}, socket_{ioc_}, is_open_{false},
    number_of_connections_{0}, logger_{spdlog::default_logger()} {}


std::shared_ptr<Listener> Listener::SetLogger(std::shared_ptr<spdlog::logger> logger) noexcept {
  logger_ = std::move(logger);
  return shared_from_this();
}

std::shared_ptr<spdlog::logger> Listener::GetLogger() const noexcept {
  return logger_;
}

bool Listener::Bind(boost::asio::ip::tcp::endpoint endpoint) noexcept {
  endpoint_ = std::move(endpoint);
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

  endpoint_ = {std::move(address), port};
  return InitAcceptor();
}

bool Listener::Bind(std::uint16_t port) noexcept {
  endpoint_ = {boost::asio::ip::tcp::v4(), port};
  return InitAcceptor();
}


const boost::asio::ip::tcp::endpoint &Listener::GetEndpoint() const noexcept {
  return endpoint_;
}

bool Listener::Run() noexcept {
  if (!is_open_) {
    return false;
  }

  logger_->info("Starting asynchronous accepting on {}.", endpoint_);
  acceptor_.async_accept(
    socket_,
    [self = shared_from_this()](const boost::system::error_code &ec) {
      self->HandleAccept(ec);
    });
  return true;
}

std::size_t Listener::GetNumberOfConnections() const noexcept {
  return number_of_connections_;
}

std::size_t Listener::GetMaxListenConnections() const noexcept {
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
    number_of_connections_++;
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

  acceptor_.open(endpoint_.protocol(), ec);
  if (ec) {
    logger_->error("Open: {}", ec.message());
    return false;
  }

  acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec) {
    logger_->error("Set option (reuse address): {}", ec.message());
    return false;
  }

  acceptor_.bind(endpoint_, ec);
  if (ec == boost::asio::error::access_denied) {
    // This means we don't have permission to bind acceptor to the this endpoint.
    logger_->error("Cannot bind acceptor to {} (permission denied).",
      endpoint_);
    return false;
  }
  if (ec == boost::asio::error::address_in_use) {
    // This means the endpoint is already is use.
    logger_->error("Cannot bind acceptor to {} (address in use).",
      endpoint_);
    return false;
  }
  if (ec) {
    logger_->error("Bind: {}", ec.message());
    return false;
  }

  acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
  if (ec) {
    logger_->error("Listen: {}", ec.message());
    return false;
  }

  is_open_ = true;
  logger_->info("Acceptor is bind to {}.", endpoint_);
  return true;
}

}  // namespace fusion_server
