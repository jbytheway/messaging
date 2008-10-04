#ifndef MESSAGING__TCP_LISTENER_HPP
#define MESSAGING__TCP_LISTENER_HPP

#include <messaging/listener.hpp>

namespace messaging {

class tcp_listener : public listener {
  public:
    tcp_listener(const asio::ip::tcp::endpoint& ep) : endpoint_(ep)
    {}

    virtual generic_endpoint endpoint() const {
      return endpoint_;
    }
  private:
    const asio::ip::tcp::endpoint endpoint_;
};

}

#endif // MESSAGING__TCP_LISTENER_HPP

