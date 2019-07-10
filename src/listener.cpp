#include <cstdint>

#ifdef DEBUG
#include <iostream>
#endif
#include <memory>
#include <string_view>
#include <utility>

#include <boost/asio.hpp>

#include <fusion_server/http_session.hpp>
#include <fusion_server/listener.hpp>

namespace fusion_server {

Listener::Listener(boost::asio::io_context& ioc, std::string_view ip_address, std::uint16_t port_numer) noexcept
    : ioc_{ioc}, acceptor_{ioc_}, socket_{ioc_},
      endpoint_{boost::asio::ip::make_address(ip_address), port_numer},
      is_open_{false} {
  OpenAcceptor();
}

Listener::Listener(boost::asio::io_context& ioc, boost::asio::ip::tcp::endpoint endpoint) noexcept
    : ioc_{ioc}, acceptor_{ioc_}, socket_{ioc_}, endpoint_{std::move(endpoint)},
    is_open_{false} {
  OpenAcceptor();
}

Listener::Listener(boost::asio::io_context& ioc, std::uint16_t port_number) noexcept
    : ioc_{ioc}, acceptor_{ioc_}, socket_{ioc_},
      endpoint_{boost::asio::ip::tcp::v4(), port_number}, is_open_{false} {
  OpenAcceptor();
}

Listener::~Listener() noexcept = default;

void Listener::Run() noexcept {
  if (!is_open_) {
    std::cerr << "The acceptor is not ready to accept." << std::endl;
    return;
  }

#ifdef DEBUG
  std::cout << "Starting asynchronous accepting." << std::endl;
#endif
  acceptor_.async_accept(
    socket_,
    [self = shared_from_this()](const boost::system::error_code& ec) {
      self->HandleAccept(ec);
    }
  );
}

void Listener::HandleAccept(const boost::system::error_code& ec) noexcept {
  if (ec == boost::asio::error::already_open) {
    // This means the socket used to hadle new connections in already open.
    // We sacrifice one connection to make the accepter work properly.
    std::cerr << "Listener::HandleAccept: a socket used to handle new connections was open."
              << std::endl;
    socket_.close();
  }
  if (ec) {
    std::cerr << "Listener::HandleAccept: " << ec.message() << std::endl;
    // TODO: find out if any error can cause the listener to stop working.
  }
  else {
#ifdef DEBUG
    std::cout << "[Listener: " << this << "] New connection from " << socket_.remote_endpoint() << std::endl;
#endif
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
    std::cerr << "Listener::OpenAcceptor: " << ec.message() << std::endl;
    return;
  }

  acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec) {
    std::cerr << "Listener::OpenAcceptor: " << ec.message() << std::endl;
    return;
  }

  acceptor_.bind(endpoint_, ec);
  if (ec == boost::asio::error::access_denied) {
    // This means we don't have permision to bind acceptor to the this endpoint.
    std::cerr << "Listener::OpenAcceptor: don't have permision to bind acceptor to: "
              << endpoint_ << std::endl;
    return;
  }
  if (ec == boost::asio::error::address_in_use) {
    // This means the endpoint is already is use.
    std::cerr << "Listener::OpenAcceptor: endpoint" << endpoint_
              << " is already is use." << std::endl;
    return;
  }
  if (ec) {
    std::cerr << "Listener::OpenAcceptor: " << ec.message() << std::endl;
    return;
  }

  acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
  if (ec) {
    std::cerr << "Listener::OpenAcceptor: " << ec.message() << std::endl;
    return;
  }

  is_open_ = true;
#ifdef DEBUG
  std::cout << "Listening on " << endpoint_ << std::endl;
#endif
}

}  // namespace fusio_server