#ifndef MESSAGING__TCP_CONNECTION_HPP
#define MESSAGING__TCP_CONNECTION_HPP

#include <boost/bind.hpp>
#include <boost/scoped_array.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/placeholders.hpp>

#include <messaging/detail/message_variant.hpp>
#include <messaging/connection.hpp>
#include <messaging/create_message.hpp>
#include <messaging/error_source.hpp>

namespace messaging {

template<typename Protocol>
class tcp_listener;

namespace detail {
struct create_connection_impl;
}

template<typename Protocol>
class tcp_connection : public connection {
  public:
    typedef Protocol protocol;

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

    virtual void reset_callbacks() {
      callbacks_.reset(new null_callbacks());
    }

    template<typename Callback>
    void reset_callbacks(
        const Callback& callback
      ) {
      callbacks_.reset(new specific_callbacks<Callback, Callback>(
            callback, callback
          ));
    }

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
    friend struct detail::create_connection_impl;
    friend class tcp_listener<Protocol>;

    typedef typename detail::message_variant<Protocol>::type message_variant;

    struct callbacks {
      virtual void message(const message_variant&, tcp_connection&) = 0;
      virtual void error(const error_source, const error_code&) = 0;
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
      virtual void error(const error_source es, const error_code& ec) {
        error_callback_(es, ec);
      }

      const Callback message_callback_;
      const ErrorCallback error_callback_;
    };

    struct null_callbacks : callbacks {
      virtual void message(const message_variant&, tcp_connection&) {}
      virtual void error(const error_source, const error_code&) {}
    };

    tcp_connection(asio::io_service& io) :
      socket_(io),
      read_(new uint8_t[256]),
      read_len_(0),
      read_capacity_(256),
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
        handle_error(error_source_connect, ec);
      } else {
        start_read();
        flush_output();
      }
    }

    void postconstructor_accepted(const asio::ip::tcp::endpoint& ep) {
      endpoint_ = ep;
      start_read();
      flush_output();
    }

    void start_read() {
      assert(read_len_ < read_capacity_);
      socket_.async_read_some(
          asio::buffer(read_.get()+read_len_, read_capacity_-read_len_),
          boost::bind(
            &tcp_connection::handle_read, this,
            asio::placeholders::error,
            asio::placeholders::bytes_transferred,
            this->shared_from_this()
          )
        );
    }

    void handle_read(
        const boost::system::error_code& ec,
        const std::size_t bytes,
        const ptr&
      );

    void handle_write(
        const boost::system::error_code& ec,
        const size_t bytes_transferred,
        const ptr&
      ) {
      if (ec) {
        handle_error(error_source_write, ec);
      } else {
        assert(bytes_transferred == writing_.size());
        writing_.clear();
        flush_output();
      }
    }

    void handle_error(const error_source es, const error_code& ec) {
      callbacks_->error(es, ec);
      error_ = true;
      if (closing_) {
        close();
      }
    }

    void flush_output();

    asio::ip::tcp::endpoint endpoint_;
    asio::ip::tcp::socket socket_;
    boost::scoped_array<uint8_t> read_;
    std::size_t read_len_;
    std::size_t read_capacity_;
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
void tcp_connection<Protocol>::handle_read(
    const boost::system::error_code& ec,
    const std::size_t bytes,
    const ptr&
  )
{
  if (ec) {
    handle_error(error_source_read, ec);
  } else {
    read_len_ += bytes;
    size_t packet_len;
    while (read_len_ >= 2+(packet_len = ((read_[0]<<8)|read_[1]))) {
      callbacks_->message(
          create_message<Protocol>(
            std::string(read_.get()+2, read_.get()+2+packet_len)
          ),
          *this
        );
      memmove(read_.get(), read_.get()+2+packet_len, read_len_-packet_len-2);
      read_len_ -= (packet_len+2);
    }
    if (read_len_ == read_capacity_) {
      boost::scoped_array<uint8_t> new_read(new uint8_t[read_capacity_*2]);
      read_capacity_ *= 2;
      memcpy(new_read.get(), read_.get(), read_len_);
      boost::swap(read_, new_read);
    }
    start_read();
  }
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
        &tcp_connection::handle_write, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred,
        this->shared_from_this()
      ));
}

}

#endif // MESSAGING__TCP_CONNECTION_HPP

