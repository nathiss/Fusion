#include <cstdint>

#include <iostream>
#include <memory>
#include <string_view>
#include <utility>

#include <boost/asio.hpp>

#include <fusion_server/http_session.hpp>
#include <fusion_server/listener.hpp>

namespace fusion_server {

Listener::Listener(boost::asio::io_context& ioc, std::string_view ip_address, uint16_t port_numer) noexcept
    : ioc_{ioc}, acceptor_{ioc_}, socket_{ioc_},
      endpoint_{boost::asio::ip::make_address(ip_address), port_numer} {
  OpenAcceptor();
}

Listener::Listener(boost::asio::io_context& ioc, boost::asio::ip::tcp::endpoint endpoint) noexcept
    : ioc_{ioc}, acceptor_{ioc_}, socket_{ioc_},
      endpoint_{std::move(endpoint)} {
  OpenAcceptor();
}

Listener::Listener(boost::asio::io_context& ioc, uint16_t port_number) noexcept
    : ioc_{ioc}, acceptor_{ioc_}, socket_{ioc_},
      endpoint_{boost::asio::ip::tcp::v4(), port_number} {
  OpenAcceptor();
}

Listener::~Listener() noexcept = default;

void Listener::Run() noexcept {
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
    std::cerr << "Listener::OpenAcceptor" << ec.message() << std::endl;
    return;
  }

  acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec) {
    std::cerr << "Listener::OpenAcceptor" << ec.message() << std::endl;
    return;
  }

  acceptor_.bind(endpoint_, ec);
  if (ec) {
    std::cerr << "Listener::OpenAcceptor" << ec.message() << std::endl;
    return;
  }

  acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
  if (ec) {
    std::cerr << "Listener::OpenAcceptor" << ec.message() << std::endl;
    return;
  }

#ifdef DEBUG
  std::cout << "Listening on " << endpoint_ << std::endl;
#endif
}

}  // namespace fusio_server