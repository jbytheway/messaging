#include <boost/asio/ip/tcp.hpp>
#include <messaging/server.hpp>

#define BOOST_TEST_MODULE runtime_array test
#include <boost/test/unit_test.hpp>

namespace asio = boost::asio;
namespace m = messaging;

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

class server {
  public:
    server(asio::io_service& io) :
      message_server_(
          callback_helper(*this),
          asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 4567),
          asio::ip::udp::endpoint(asio::ip::udp::v4(), 4567)
        )
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
      out << "invalid message" << endl;
      connection.close();
    }

    template<typename Connection>
    void message(const ::message<join>& message, Connection& connection) {
      clients_.insert(client::create(connection));
    }

    m::server message_server_;
};

BOOST_AUTO_TEST_CASE(server)
{
  asio::io_service io;
}

