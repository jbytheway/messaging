#ifndef MESSAGING__TCP_LISTENER_HPP
#define MESSAGING__TCP_LISTENER_HPP

#include <boost/signal.hpp>

#include <messaging/listener.hpp>
#include <messaging/tcp_connection.hpp>

namespace messaging {

template<typename Protocol>
class tcp_listener : public listener {
  public:
    virtual generic_endpoint endpoint() const {
      return endpoint_;
    }

    virtual void close() {
      acceptor_.close();
    }
  private:
    friend struct create_listener_impl;

    template<typename Callback, typename ErrorCallback>
    tcp_listener(
        asio::io_service& io,
        const asio::ip::tcp::endpoint& ep,
        const Callback& c,
        const ErrorCallback& ec
      ) :
      endpoint_(ep),
      acceptor_(io, ep)
    {
      new_connection_signal_.connect(c);
      error_signal_.connect(ec);
    }

    void postconstructor() {
      start_accept();
    }

    void start_accept() {
      new_connection_.reset(
          new tcp_connection<Protocol>(acceptor_.get_io_service())
        );
      acceptor_.async_accept(new_connection_->socket_, new_endpoint_,
          boost::bind(
            &tcp_listener::handle_accept, this,
            boost::asio::placeholders::error,
            this->shared_from_this()
          ));
    }

    void handle_accept(const boost::system::error_code& ec, const ptr&) {
      if (ec) {
        error_signal_(error_source_accept, ec);
      } else {
        new_connection_signal_(*new_connection_);
        new_connection_->postconstructor_accepted(new_endpoint_);
        new_connection_.reset();
        start_accept();
      }
    }

    const asio::ip::tcp::endpoint endpoint_;
    asio::ip::tcp::acceptor acceptor_;
    boost::shared_ptr<tcp_connection<Protocol> > new_connection_;
    asio::ip::tcp::endpoint new_endpoint_;
    boost::signal<void(tcp_connection<Protocol>&)> new_connection_signal_;
    boost::signal<void(const error_source es, const error_code&)> error_signal_;
};

}

#endif // MESSAGING__TCP_LISTENER_HPP

