#ifndef MESSAGING__CREATE_CONNECTION_HPP
#define MESSAGING__CREATE_CONNECTION_HPP

#include <boost/spirit/home/phoenix/core/reference.hpp>

#include <messaging/tcp_connection.hpp>

namespace messaging {

namespace detail {

struct create_connection_impl : boost::static_visitor<connection::ptr> {
  template<
    typename Io,
    typename Protocol,
    typename Endpoint,
    typename Callback,
    typename ErrorCallback
  >
  struct result {
    typedef connection::ptr type;
  };

  template<typename Protocol, typename Callback, typename ErrorCallback>
  connection::ptr operator()(
      asio::io_service& io,
      const detail::protocol_wrapper<Protocol>&,
      const asio::ip::tcp::endpoint& ep,
      const Callback& callback,
      const ErrorCallback& error_callback
    ) const {
    typedef tcp_connection<Protocol> conn;
    boost::shared_ptr<conn> p(new conn(io));
    p->postconstructor_connect(ep, callback, error_callback);
    return p;
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
      const protocol_wrapper<Protocol>& p,
      const GenericEndpoint& ep,
      const Callback& callback,
      const ErrorCallback& error_callback
    ) const {
    return ep.apply_visitor(
        create_connection_px(io, p, arg1, callback, error_callback)
      );
  }

  BOOST_STATIC_ASSERT(PHOENIX_COMPOSITE_LIMIT >= 5);
};

px::function<create_connection_impl> create_connection_px;

} // end namespace detail

template<
    typename Protocol,
    typename GenericEndpoint,
    typename Callback
  >
inline connection::ptr create_connection(
      asio::io_service& io,
      const GenericEndpoint& ep,
      const Callback& callback
  )
{
  return detail::create_connection_px(
      px::ref(io),
      detail::protocol_wrapper<Protocol>(),
      ep,
      callback,
      callback
    )();
}

template<
    typename Protocol,
    typename GenericEndpoint,
    typename Callback,
    typename ErrorCallback
  >
inline connection::ptr create_connection(
      asio::io_service& io,
      const GenericEndpoint& ep,
      const Callback& callback,
      const ErrorCallback& error_callback
  )
{
  return detail::create_connection_px(
      px::ref(io),
      detail::protocol_wrapper<Protocol>(),
      ep,
      callback,
      error_callback
    )();
}

}

#endif // MESSAGING__CREATE_CONNECTION_HPP

