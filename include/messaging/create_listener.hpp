#ifndef MESSAGING__CREATE_LISTENER_HPP
#define MESSAGING__CREATE_LISTENER_HPP

#include <boost/spirit/home/phoenix/core.hpp>
#include <boost/spirit/home/phoenix/bind.hpp>
#include <boost/spirit/home/phoenix/function.hpp>

#include <messaging/detail/protocol_wrapper.hpp>
#include <messaging/tcp_listener.hpp>
#include <messaging/udp_listener.hpp>

namespace messaging {

struct create_listener_impl : boost::static_visitor<listener::ptr> {
  template<
      typename Io,
      typename Protocol,
      typename Endpoint,
      typename Callback,
      typename ErrorCallback
    >
  struct result {
    typedef listener::ptr type;
  };

  template<typename Protocol, typename Callback, typename ErrorCallback>
  listener::ptr operator()(
      asio::io_service&,
      const detail::protocol_wrapper<Protocol>&,
      const boost::blank&,
      const Callback&,
      const ErrorCallback&
    ) const {
    return listener::ptr();
  }

  template<typename Protocol, typename Callback, typename ErrorCallback>
  listener::ptr operator()(
      asio::io_service& io,
      const detail::protocol_wrapper<Protocol>&,
      const asio::ip::tcp::endpoint& ep,
      const Callback& c,
      const ErrorCallback& ec
    ) const {
    typedef tcp_listener<Protocol> l;
    boost::shared_ptr<l> p(new l(io, ep, c, ec));
    p->postconstructor();
    return p;
  }

  template<typename Protocol, typename Callback, typename ErrorCallback>
  listener::ptr operator()(
      asio::io_service& io,
      const detail::protocol_wrapper<Protocol>&,
      const asio::ip::udp::endpoint& ep,
      const Callback& c,
      const ErrorCallback& ec
    ) const {
    return listener::ptr(new udp_listener<Protocol>(io, ep, c, ec));
  }

  // Written as a template to prevent implicit conversion from other things to
  // generic_endpoint
  template<
      typename Protocol,
      typename GenericEndpoint,
      typename Callback,
      typename ErrorCallback
    >
  typename boost::enable_if<
      typename boost::is_same<GenericEndpoint, generic_endpoint>::type,
      listener::ptr
    >::type
  operator()(
      asio::io_service& io,
      const detail::protocol_wrapper<Protocol>& p,
      const GenericEndpoint& ep,
      const Callback& c,
      const ErrorCallback& ec
    ) const {
    return ep.apply_visitor(boost::bind(*this, boost::ref(io), p, _1, c, ec));
  }
};

px::function<create_listener_impl> const create_listener;

}

#endif // MESSAGING__CREATE_LISTENER_HPP

