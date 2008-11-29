#ifndef MESSAGING__TCP_CONNECTION_HPP
#define MESSAGING__TCP_CONNECTION_HPP

#include <messaging/connection.hpp>

namespace messaging {

template<typename Callback, typename ErrorCallback>
class tcp_connection : public connection {
  public:
    virtual generic_endpoint endpoint() const {
      return endpoint_;
    }
    virtual void close() {
      socket_.close();
    }
    virtual void send(const std::string&);
  private:
    friend class create_connection_impl;

    tcp_connection(
        asio::io_service& io,
        const Callback& callback,
        const ErrorCallback& error_callback,
        const asio::ip::tcp::endpoint& ep
      ) :
      endpoint_(ep),
      socket_(io),
      callback_(callback),
      error_callback_(error_callback)
    {
    }

    void postconstructor() {
      socket_.async_connect(endpoint_, boost::bind(
            &tcp_connection::connect_handler,
            this,
            asio::placeholders::error,
            this->shared_from_this()
          ));
    }

    void connect_handler(
        const boost::system::error_code& ec,
        const ptr&
      ) {
      if (ec) {
        error_callback_(ec);
      } else {
        flush_output();
      }
    }

    void flush_output();

    const asio::ip::tcp::endpoint endpoint_;
    asio::ip::tcp::socket socket_;
    const Callback callback_;
    const ErrorCallback error_callback_;
};

template<typename Callback, typename ErrorCallback>
void tcp_connection<Callback, ErrorCallback>::flush_output()
{
  footle
}

}

#endif // MESSAGING__TCP_CONNECTION_HPP

