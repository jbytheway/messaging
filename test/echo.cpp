#include <iostream>
#include <tr1/unordered_set>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <messaging/server.hpp>

#define BOOST_TEST_MODULE echo test
#include <boost/test/unit_test.hpp>

namespace asio = boost::asio;
namespace m = messaging;

namespace echo {

enum message_type {
  join,
  string
};

template<message_type>
class message;

template<>
class message<join> {
};

template<>
class message<string> {
  public:
    message(const std::string& v) : value_(v) {}
    const std::string& value() const { return value_; }
  private:
    const std::string value_;
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

class server {
  public:
    server(asio::io_service& io, std::ostream& out) :
      message_server_(
          io,
          callback_helper(*this),
          asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 4567),
          asio::ip::udp::endpoint(asio::ip::udp::v4(), 4567)
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
    void message(const echo::message<join>& message, Connection& connection) {
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

}

