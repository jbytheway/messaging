#ifndef MESSAGING__CREATE_LISTENER_HPP
#define MESSAGING__CREATE_LISTENER_HPP

#include <boost/spirit/phoenix/functions.hpp>

#include <messaging/tcp_listener.hpp>
#include <messaging/udp_listener.hpp>

namespace messaging {

struct create_listener_impl : boost::static_visitor<listener::ptr> {
  template<typename Endpoint>
  struct result {
    typedef listener::ptr type;
  };

  listener::ptr operator()(const boost::blank&) const {
    return listener::ptr();
  }

  listener::ptr operator()(const asio::ip::tcp::endpoint& ep) const {
    return listener::ptr(new tcp_listener(ep));
  }

  listener::ptr operator()(const asio::ip::udp::endpoint& ep) const {
    return listener::ptr(new udp_listener(ep));
  }

  // Written as a template to prevent implicit conversion from other things to
  // generic_endpoint
  template<typename GenericEndpoint>
  typename boost::enable_if<
      typename boost::is_same<GenericEndpoint, generic_endpoint>::type,
      listener::ptr
    >::type
  operator()(const GenericEndpoint& ep) const {
    return ep.apply_visitor(*this);
  }
};

px::function<create_listener_impl> const create_listener;

}

#endif // MESSAGING__CREATE_LISTENER_HPP

