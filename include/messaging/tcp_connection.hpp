#ifndef MESSAGING__TCP_CONNECTION_HPP
#define MESSAGING__TCP_CONNECTION_HPP

#include <boost/asio/write.hpp>

#include <messaging/detail/message_variant.hpp>
#include <messaging/connection.hpp>

namespace messaging {

template<typename Protocol>
class tcp_listener;

namespace detail {
class create_connection_impl;
}

template<typename Protocol>
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
      if (error_) {
        close();
      } else {
        flush_output();
      }
    }
    virtual void send(const std::string&);

    template<typename Callback, typename ErrorCallback>
    void reset_callbacks(
        const Callback& callback,
        const ErrorCallback& error_callback
      ) {
      callbacks_.reset(new specific_callbacks<Callback, ErrorCallback>(
            callback, error_callback
          ));
    }
  private:
    friend class detail::create_connection_impl;
    friend class tcp_listener<Protocol>;

    typedef typename detail::message_variant<Protocol>::type message_variant;

    struct callbacks {
      virtual void message(const message_variant&, tcp_connection&) = 0;
      virtual void error(const boost::system::error_code&) = 0;
    };

    template<typename Callback, typename ErrorCallback>
    struct specific_callbacks : callbacks {
      specific_callbacks(const Callback& c, const ErrorCallback& ec) :
        message_callback_(c), error_callback_(ec)
      {}

      struct message_visitor : boost::static_visitor<> {
        typedef void result_type;
        message_visitor(const Callback& callback, tcp_connection& conn) :
          callback_(callback), connection_(conn)
        {}
        template<typename Message>
        void operator()(const Message& m) const {
          callback_(m, connection_);
        }
        const Callback& callback_;
        tcp_connection& connection_;
      };

      virtual void message(const message_variant& v, tcp_connection& c) {
        message_visitor vis(message_callback_, c);
        v.apply_visitor(vis);
      }
      virtual void error(const boost::system::error_code& ec) {
        error_callback_(ec);
      }

      const Callback message_callback_;
      const ErrorCallback error_callback_;
    };

    tcp_connection(asio::io_service& io) :
      socket_(io),
      closing_(false),
      error_(false)
    {
    }

    template<typename Callback, typename ErrorCallback>
    void postconstructor_connect(
        const asio::ip::tcp::endpoint& ep,
        const Callback& callback,
        const ErrorCallback& error_callback
      ) {
      endpoint_ = ep;
      reset_callbacks(callback, error_callback);
      boost::system::error_code ec;
      socket_.connect(endpoint_, ec);
      if (ec) {
        error_handler(ec);
      } else {
        flush_output();
      }
    }

    void postconstructor_accepted(const asio::ip::tcp::endpoint& ep) {
      endpoint_ = ep;
      flush_output();
    }

    void write_handler(
        const boost::system::error_code& ec,
        const size_t bytes_transferred,
        const ptr&
      )
    {
      if (ec) {
        error_handler(ec);
      } else {
        assert(bytes_transferred == writing_.size());
        writing_.clear();
        flush_output();
      }
    }

    void error_handler(const boost::system::error_code& ec) {
      callbacks_->error(ec);
      error_ = true;
      if (closing_) {
        close();
      }
    }

    void flush_output();

    asio::ip::tcp::endpoint endpoint_;
    asio::ip::tcp::socket socket_;
    std::string outgoing_;
    std::string writing_;
    bool closing_; // We've been asked to close gracefully
    bool error_; // An error has been encountered
    boost::scoped_ptr<callbacks> callbacks_;
};

template<typename Protocol>
void tcp_connection<Protocol>::send(const std::string& s)
{
  if (s.size() >= (size_t(1)<<16)) {
    throw std::logic_error("too many bytes in message");
  }
  outgoing_ += static_cast<unsigned char>(s.size()/256);
  outgoing_ += static_cast<unsigned char>(s.size()%256);
  outgoing_ += s;
  flush_output();
}

template<typename Protocol>
void tcp_connection<Protocol>::flush_output()
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
        boost::asio::placeholders::bytes_transferred,
        this->shared_from_this()
      ));
}

}

#endif // MESSAGING__TCP_CONNECTION_HPP

