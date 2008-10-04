#ifndef MESSAGING__UDP_LISTENER_HPP
#define MESSAGING__UDP_LISTENER_HPP

#include <messaging/listener.hpp>

namespace messaging {

class udp_listener : public listener {
  public:
    udp_listener(const asio::ip::udp::endpoint& ep) : endpoint_(ep)
    {}

    virtual generic_endpoint endpoint() const {
      return endpoint_;
    }
  private:
    const asio::ip::udp::endpoint endpoint_;
};

}

#endif // MESSAGING__UDP_LISTENER_HPP

