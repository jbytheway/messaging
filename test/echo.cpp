#include <iostream>
#include <tr1/unordered_set>
#include <boost/bind.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/thread.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <messaging/server.hpp>
#include <messaging/create_connection.hpp>
#include <messaging/send.hpp>

#define BOOST_TEST_MODULE echo test
#include <boost/test/unit_test.hpp>

namespace asio = boost::asio;
namespace m = messaging;

namespace echo {

enum message_type {
  message_type_join,
  message_type_string,
  message_type_max
};

template<message_type>
class message;

template<>
class message<message_type_join> {
  private:
    friend class boost::serialization::access;

    template<typename Archive>
    void serialize(Archive& /*ar*/, unsigned int const /*version*/) {
    }
};

template<>
class message<message_type_string> {
  public:
    message(const std::string& v) : value_(v) {}
    const std::string& value() const { return value_; }
  private:
    friend class boost::serialization::access;

    const std::string value_;

    template<typename Archive>
    void serialize(Archive& ar, unsigned int const /*version*/) {
      ar & BOOST_SERIALIZATION_NVP(value_);
    }
};

struct protocol {
  typedef boost::archive::xml_oarchive oarchive_type;
  typedef boost::archive::xml_iarchive iarchive_type;
  typedef message_type message_type_type;
  static const message_type max_message_type = message_type_max;
  template<typename type_index>
  struct message_type_from_index {
    typedef message<message_type(type_index::value)> type;
  };
};

struct ptr_hash {
  template<typename T>
  size_t operator()(const boost::shared_ptr<T>& p) {
    return reinterpret_cast<size_t>(p.get());
  }
};

class client : boost::enable_shared_from_this<client>, boost::noncopyable {
  public:
    typedef boost::shared_ptr<client> ptr;
    typedef boost::weak_ptr<client> wptr;

    template<typename Connection>
    ptr create(const Connection& connection);
  protected:
    client()
    {}
  private:
};

template<typename Connection>
class client_impl : client {
  private:
    friend class client;
    client_impl(Connection& connection) :
      connection_(connection.shared_from_this())
    {}
    boost::shared_ptr<Connection> connection_;
};

template<typename Connection>
inline client::ptr client::create(const Connection& connection) {
  return ptr(new client_impl<Connection>(connection));
}

static const uint16_t port = 4567;

class server {
  public:
    server(asio::io_service& io, std::ostream& out) :
      message_server_(
          io,
          callback_helper(*this),
          asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port),
          asio::ip::udp::endpoint(asio::ip::udp::v4(), port)
        ),
      out_(out)
    {
    }
  private:
    struct callback_helper {
      callback_helper(server& s) : server_(s) {}
      template<typename Message, typename Connection>
      void operator()(const Message& message, Connection& c) const {
        server_.message(message, c);
      }
      server& server_;
    };

    template<typename Message, typename Connection>
    void message(const Message& message, Connection& connection) {
      out_ << "invalid message" << std::endl;
      connection.close();
    }

    template<typename Connection>
    void message(
        const echo::message<message_type_join>& message,
        Connection& connection
      ) {
      clients_.insert(client::create(connection));
    }

    m::server<callback_helper> message_server_;
    std::ostream& out_;
    std::tr1::unordered_set<client::ptr, ptr_hash> clients_;
};

BOOST_AUTO_TEST_CASE(echo_server)
{
  asio::io_service io;
  server s(io, std::cout);
}

class server_interface {
  public:
    server_interface(asio::io_service& io, std::ostream& out) :
      connection_(
          m::create_connection(
            phoenix::var(io),
            callback_helper(*this),
            error_callback_helper(*this),
            asio::ip::tcp::endpoint(
              asio::ip::address_v4::from_string("127.0.0.1"), port
            )
          )()
        ),
      out_(out)
    {
    }

    void send(const std::string& text) {
      m::send<protocol>(echo::message<message_type_string>(text), connection_);
    }

    void close() {
      connection_->close_gracefully();
    }

    void confirm_receipt(const std::string& text) {
      assert(received_ == text);
    }
  private:
    struct callback_helper {
      callback_helper(server_interface& s) : server_interface_(s) {}
      template<typename Message, typename Connection>
      void operator()(const Message& message, Connection& c) const {
        server_interface_.message(message, c);
      }
      server_interface& server_interface_;
    };

    struct error_callback_helper {
      error_callback_helper(server_interface& s) : server_interface_(s) {}
      void operator()(const boost::system::error_code& ec) const {
        server_interface_.error(ec);
      }
      server_interface& server_interface_;
    };

    template<typename Message, typename Connection>
    void message(const Message& message, Connection& connection) {
      out_ << "invalid message" << std::endl;
      connection.close();
    }

    template<typename Connection>
    void message(
        const echo::message<message_type_string>& message,
        Connection& connection
      ) {
      received_ += message.value();
    }

    void error(const boost::system::error_code& ec) {
      std::cerr << "sever_interface: " << ec.message() << std::endl;
    }

    m::connection::ptr connection_;
    std::string received_;
    std::ostream& out_;
};

BOOST_AUTO_TEST_CASE(echo_client)
{
  asio::io_service io;
  server_interface s(io, std::cout);
}

struct io_thread {
  io_thread(boost::asio::io_service& io) :
    io_(io),
    timer_(io, boost::posix_time::seconds(1))
  {}
  asio::io_service& io_;
  asio::deadline_timer timer_;
  void operator()() {
    timer_.async_wait(boost::bind(
          &io_thread::timerExpired, this, boost::asio::placeholders::error
        ));
    io_.run();
    std::cout << "IO thread completed" << std::endl;
  }
  void timerExpired(const boost::system::error_code& e) {
    if (e) {
      std::cout << "IO thread interrupted" << std::endl;
    } else {
      timer_.expires_from_now(boost::posix_time::seconds(1));
      timer_.async_wait(boost::bind(
            &io_thread::timerExpired, this, boost::asio::placeholders::error
          ));
    }
  }
  void interrupt() {
    timer_.cancel();
  }
};

BOOST_AUTO_TEST_CASE(echo_conversation)
{
  asio::io_service io;
  server serve(io, std::cout);
  server_interface si(io, std::cout);
  io_thread io_thread_obj(io);
  boost::thread io_t(boost::ref(io_thread_obj));
  std::string test("test message");
  si.send(test);
  si.close();
  io_thread_obj.interrupt();
  io_t.join();
  si.confirm_receipt(test);
}

}

