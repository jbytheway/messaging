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
#include <boost/serialization/string.hpp>
#include <messaging/server.hpp>
#include <messaging/create_connection.hpp>
#include <messaging/send.hpp>
#include <messaging/error_source.hpp>

#define BOOST_TEST_MODULE echo test
#include <boost/test/unit_test.hpp>

namespace asio = boost::asio;
namespace m = messaging;

namespace echo {

using boost::system::error_code;

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
    message() {}
    message(const std::string& v) : value_(v) {}
    const std::string& value() const { return value_; }
  private:
    friend class boost::serialization::access;

    std::string value_;

    template<typename Archive>
    void serialize(Archive& ar, unsigned int const /*version*/) {
      ar & BOOST_SERIALIZATION_NVP(value_);
    }
};

struct protocol {
  typedef boost::archive::xml_oarchive oarchive_type;
  typedef boost::archive::xml_iarchive iarchive_type;
  typedef message_type message_index_type;
  static const message_type max_message_index = message_type_max;
  template<typename type_index>
  struct message_type_from_index {
    typedef message<message_type(type_index::value)> type;
  };
};

struct ptr_hash {
  template<typename T>
  size_t operator()(const boost::shared_ptr<T>& p) const {
    return reinterpret_cast<size_t>(p.get());
  }
};

class client :
  public boost::enable_shared_from_this<client>,
  private boost::noncopyable {
  public:
    typedef boost::shared_ptr<client> ptr;
    typedef boost::weak_ptr<client> wptr;

    template<typename Connection>
    client(Connection& c, std::ostream& out) :
      out_(out),
      joined_(false)
    {
      c.reset_callbacks(callback_helper(*this), error_callback_helper(*this));
    }
  private:
    struct callback_helper {
      typedef void result_type;
      callback_helper(client& c) : client_(c) {}
      template<typename Message, typename Connection>
      void operator()(const Message& m, Connection& c) const {
        client_.message(m, c);
      }
      client& client_;
    };

    struct error_callback_helper {
      error_callback_helper(client& c) : client_(c) {}
      void operator()(const m::error_source es, const error_code& ec) const {
        client_.error(es, ec);
      }
      client& client_;
    };

    template<typename Message, typename Connection>
    void message(const Message&, Connection& connection) {
      std::cerr << "client: invalid message of type " <<
        m::message_type<protocol, Message>() << std::endl;
      connection.close();
    }

    template<typename Connection>
    void message(
        const echo::message<message_type_join>&,
        Connection&
      ) {
      if (joined_) {
        std::cerr << "client: too many joins!" << std::endl;
      }
      out_ << "client: recieved join" << std::endl;
      joined_ = true;
    }

    template<typename Connection>
    void message(
        const echo::message<message_type_string>& m,
        Connection& connection
      ) {
      if (!joined_) {
        std::cerr << "client: text before join!" << std::endl;
      }
      out_ << "client: recieved '" << m.value() << "'" << std::endl;
      m::send(m, connection);
    }

    void error(const m::error_source es, const error_code& ec) {
      std::cerr << "client: " << es << ": " << ec.message() << std::endl;
    }

    std::ostream& out_;
    bool joined_;
};

static const uint16_t port = 4567;

class server {
  public:
    server(asio::io_service& io, std::ostream& out) :
      message_server_(
          io,
          callback_helper(*this),
          error_callback_helper(*this),
          asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port),
          asio::ip::udp::endpoint(asio::ip::udp::v4(), port)
        ),
      out_(out)
    {
    }

    void close_listeners() {
      message_server_.close_listeners();
    }
  private:
    struct callback_helper {
      callback_helper(server& s) : server_(s) {}
      template<typename Connection>
      void operator()(Connection& c) const {
        server_.new_connection(c);
      }
      server& server_;
    };

    struct error_callback_helper {
      error_callback_helper(server& s) : server_(s) {}
      void operator()(const m::error_source es, const error_code& ec) const {
        server_.error(es, ec);
      }
      server& server_;
    };

    template<typename Connection>
    void new_connection(Connection& c) {
      client::ptr cl(new client(c, out_));
      clients_.insert(cl);
    }

    void error(const m::error_source es, const error_code& ec) {
      std::cerr << "server: " << es << ": " << ec.message() << std::endl;
    }

    m::server<protocol, callback_helper, error_callback_helper> message_server_;
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
          m::create_connection<protocol>(
            io,
            asio::ip::tcp::endpoint(
              asio::ip::address_v4::from_string("127.0.0.1"), port
            ),
            callback_helper(*this),
            error_callback_helper(*this)
          )
        ),
      out_(out)
    {
      m::send<protocol>(echo::message<message_type_join>(), connection_);
    }

    void send(const std::string& text) {
      m::send<protocol>(echo::message<message_type_string>(text), connection_);
    }

    void close() {
      connection_->close_gracefully();
    }

    void confirm_receipt(const std::string& text) {
      BOOST_CHECK_EQUAL(received_, text);
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
      void operator()(m::error_source es, const error_code& ec) const {
        server_interface_.error(es, ec);
      }
      server_interface& server_interface_;
    };

    template<typename Message, typename Connection>
    void message(const Message&, Connection& connection) {
      std::cerr << "server interface: invalid message" << std::endl;
      connection.close();
    }

    template<typename Connection>
    void message(
        const echo::message<message_type_string>& message,
        Connection&
      ) {
      out_ << "server_interface: recieved '" << message.value() << "'" <<
        std::endl;
      received_ += message.value();
    }

    void error(m::error_source es, const boost::system::error_code& ec) {
      std::cerr << "server_interface: " << es << ": " << ec.message() <<
        std::endl;
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
  io_thread(boost::asio::io_service& io, server& server, server_interface& si) :
    io_(io),
    server_(server),
    si_(si),
    timer_(io, boost::posix_time::millisec(500)),
    interrupted_(false)
  {}
  asio::io_service& io_;
  server& server_;
  server_interface& si_;
  asio::deadline_timer timer_;
  bool interrupted_;
  void operator()() {
    timer_.async_wait(boost::bind(
          &io_thread::timerExpired, this, boost::asio::placeholders::error
        ));
    io_.run();
    std::cout << "IO thread completed" << std::endl;
  }
  void timerExpired(const boost::system::error_code& e) {
    if (interrupted_ || e) {
      std::cout << "IO thread interrupted" << std::endl;
      si_.close();
      server_.close_listeners();
    } else {
      timer_.expires_from_now(boost::posix_time::millisec(500));
      timer_.async_wait(boost::bind(
            &io_thread::timerExpired, this, boost::asio::placeholders::error
          ));
    }
  }
  void interrupt() {
    interrupted_ = true;
  }
};

BOOST_AUTO_TEST_CASE(echo_conversation)
{
  asio::io_service io;
  server serve(io, std::cout);
  server_interface si(io, std::cout);
  std::string test("test message");
  si.send(test);
  io_thread io_thread_obj(io, serve, si);
  boost::thread io_t(boost::ref(io_thread_obj));
  io_thread_obj.interrupt();
  io_t.join();
  si.confirm_receipt(test);
}

} // namespace echo

