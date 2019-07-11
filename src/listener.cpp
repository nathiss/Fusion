#include <cstdint>

#include <memory>
#include <string_view>
#include <utility>

#include <boost/asio.hpp>

#include <fusion_server/http_session.hpp>
#include <fusion_server/listener.hpp>
#include <fusion_server/logger_types.hpp>

namespace fusion_server {

Listener::Listener(boost::asio::io_context& ioc, std::string_view ip_address, std::uint16_t port_numer) noexcept
    : ioc_{ioc}, acceptor_{ioc_}, socket_{ioc_},
      endpoint_{boost::asio::ip::make_address(ip_address), port_numer},
      is_open_{false} {
  logger_ = spdlog::get("listener");
  OpenAcceptor();
}

Listener::Listener(boost::asio::io_context& ioc, boost::asio::ip::tcp::endpoint endpoint) noexcept
    : ioc_{ioc}, acceptor_{ioc_}, socket_{ioc_}, endpoint_{std::move(endpoint)},
    is_open_{false} {
  logger_ = spdlog::get("listener");
  OpenAcceptor();
}

Listener::Listener(boost::asio::io_context& ioc, std::uint16_t port_number) noexcept
    : ioc_{ioc}, acceptor_{ioc_}, socket_{ioc_},
      endpoint_{boost::asio::ip::tcp::v4(), port_number}, is_open_{false} {
  logger_ = spdlog::get("listener");
  OpenAcceptor();
}

Listener::~Listener() noexcept = default;

bool Listener::Run() noexcept {
  if (!is_open_) {
    logger_->critical("The listener is not ready to listen.");
    return false;
  }

  logger_->info("Starting asynchronous accepting on {}.", endpoint_);
  acceptor_.async_accept(
    socket_,
    [self = shared_from_this()](const boost::system::error_code& ec) {
      self->HandleAccept(ec);
    }
  );

  return true;
}

void Listener::HandleAccept(const boost::system::error_code& ec) noexcept {
  if (ec == boost::asio::error::already_open) {
    // This means the socket used to hadle new connections in already open.
    // We sacrifice one connection to make the accepter work properly.
    logger_->warn("The socket used to accept new connections is aleary open.");
    socket_.close();
  }

  if (ec == boost::asio::error::no_recovery) {
    // Unidentified no recovery error occured.
    logger_->critical("A non-recoverable error occured during handling a new connection. [Boost:{}]",
      ec.message());
    return;
  }
  if (ec) {
    logger_->error("An error occured during handling a new connection. [Boost:{}]",
      ec.message());
    // TODO: find out if any other error can cause the listener to stop working.
  }
  else {
    logger_->debug("Accepted a new connection from {}.", socket_.remote_endpoint());
    std::make_shared<HTTPSession>(std::move(socket_))->Run();
  }

  acceptor_.async_accept(
    socket_,
    [self = shared_from_this()](const boost::system::error_code& ec) {
      self->HandleAccept(ec);
    }
  );
}

void Listener::OpenAcceptor() noexcept {
  boost::system::error_code ec;

  acceptor_.open(endpoint_.protocol(), ec);
  if (ec) {
    logger_->error("Open: {}", ec.message());
    return;
  }

  acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec) {
    logger_->error("Set option (reuse address): {}", ec.message());
    return;
  }

  acceptor_.bind(endpoint_, ec);
  if (ec == boost::asio::error::access_denied) {
    // This means we don't have permision to bind acceptor to the this endpoint.
    logger_->critical("Cannot bind acceptor to {} (permition denied).",
      endpoint_);
    return;
  }
  if (ec == boost::asio::error::address_in_use) {
    // This means the endpoint is already is use.
    logger_->critical("Cannot bind acceptor to {} (address in use).",
      endpoint_);
    return;
  }
  if (ec) {
    logger_->error("Bind: {}", ec.message());
    return;
  }

  acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
  if (ec) {
    logger_->error("Listen: {}", ec.message());
    return;
  }

  is_open_ = true;
  logger_->info("Acceptor is bind to {}.", endpoint_);
}

}  // namespace fusio_server