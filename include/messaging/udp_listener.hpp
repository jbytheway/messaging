#ifndef MESSAGING__UDP_LISTENER_HPP
#define MESSAGING__UDP_LISTENER_HPP

#include <messaging/listener.hpp>

namespace messaging {

template<typename Protocol>
class udp_listener : public listener {
  public:
    virtual generic_endpoint endpoint() const {
      return endpoint_;
    }

    virtual void close() {
    }
  private:
    friend class create_listener_impl;

    template<typename Callback, typename ErrorCallback>
    udp_listener(
        asio::io_service&,
        const asio::ip::udp::endpoint& ep,
        const Callback&,
        const ErrorCallback&
      ) : endpoint_(ep)
    {}

    const asio::ip::udp::endpoint endpoint_;
};

}

#endif // MESSAGING__UDP_LISTENER_HPP

