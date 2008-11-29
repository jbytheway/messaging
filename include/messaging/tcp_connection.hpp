#ifndef MESSAGING__TCP_CONNECTION_HPP
#define MESSAGING__TCP_CONNECTION_HPP

#include <boost/asio/write.hpp>

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
    virtual void close_gracefully() {
      closing_ = true;
      flush_output();
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

    void write_handler(
        const boost::system::error_code& ec,
        const size_t bytes_transferred
      )
    {
      if (ec) {
        error_callback_(ec);
      } else {
        assert(bytes_transferred == writing_.size());
        flush_output();
      }
    }

    void flush_output();

    const asio::ip::tcp::endpoint endpoint_;
    asio::ip::tcp::socket socket_;
    std::string outgoing_;
    std::string writing_;
    bool closing_;
    const Callback callback_;
    const ErrorCallback error_callback_;
};

template<typename Callback, typename ErrorCallback>
void tcp_connection<Callback, ErrorCallback>::send(const std::string& s)
{
  if (s.size() >= (size_t(1)<<16)) {
    throw std::logic_error("too many bytes in message");
  }
  outgoing_ += static_cast<unsigned char>(s.size()/256);
  outgoing_ += static_cast<unsigned char>(s.size()%256);
  outgoing_ += s;
  flush_output();
}

template<typename Callback, typename ErrorCallback>
void tcp_connection<Callback, ErrorCallback>::flush_output()
{
  if (!writing_.empty()) {
    return;
  }

  if (outgoing_.empty()) {
    if (closing_) {
      close();
    }
    return;
  }

  swap(writing_, outgoing_);

  asio::async_write(socket_, asio::buffer(writing_), boost::bind(
        &tcp_connection::write_handler, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred
      ));
}

}

#endif // MESSAGING__TCP_CONNECTION_HPP

