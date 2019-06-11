#include <functional>
#include <memory>
#include <utility>

#include <boost/asio.hpp>

#include <fusion_server/http_session.hpp>
#include <fusion_server/listener.hpp>

namespace fusion_server {

struct Listener::Impl {
  // Methods
  /**
   * This method open the acceptor & binds it to the endpoint.
   */
  void OpenAcceptor() noexcept {
    boost::system::error_code ec;
    acceptor_.open(endpoint_.protocol(), ec);

    if (ec) {
      // TODO: handle error
      return;
    }

    acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
    if (ec) {
      // TODO: handle error
      return;
    }

    acceptor_.bind(endpoint_, ec);
    if (ec) {
      // TODO: handle error
      return;
    }

    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec) {
      // TODO: handle error
      return;
    }
  }

  // Attributes

  /**
   * The context for providing core I/O functionality.
   */
  boost::asio::io_context& ioc_;

  /**
   * This is the local endpoint on which new connections will be accepted.
   */
  boost::asio::ip::tcp::endpoint endpoint_;

  /**
   * This acceptor is accepting new connections.
   */
  boost::asio::ip::tcp::acceptor acceptor_{ioc_};

  /**
   * This is the socket used to handle a new incomming connection.
   */
  boost::asio::ip::tcp::socket socket_{ioc_};
};

Listener::Listener(boost::asio::io_context& ioc, std::string_view ip_address, uint32_t port_numer) noexcept
    : impl_{new Impl{ioc}}  {
  impl_->endpoint_ = {boost::asio::ip::make_address(ip_address), port_numer};
  impl_->OpenAcceptor();
}

Listener::Listener(boost::asio::io_context& ioc, boost::asio::ip::tcp::endpoint endpoint) noexcept
    : impl_{new Impl{ioc}} {
  impl_->endpoint_ = std::move(endpoint);
  impl_->OpenAcceptor();
}

Listener::Listener(boost::asio::io_context& ioc, uint32_t port_number) noexcept
    : impl_{new Impl{ioc}} {
  impl_->endpoint_ = {boost::asio::ip::tcp::v4(), port_number};
  impl_->OpenAcceptor();
}

void Listener::Run() noexcept {
  impl_->acceptor_.async_accept(
    impl_->socket_,
    std::bind(
      &Listener::HandleAccept,
      shared_from_this(),
      std::placeholders::_1
    )
  );
}

void Listener::HandleAccept(const boost::system::error_code& ec) noexcept {
  if (ec) {
    // TODO: handle error
  }
  else {
    std::make_shared<HTTPSession>(std::move(impl_->socket_))->Run();
  }

  impl_->acceptor_.async_accept(
    impl_->socket_,
    std::bind(
      &Listener::HandleAccept,
      shared_from_this(),
      std::placeholders::_1
    )
  );
}

}  // namespace fusio_server