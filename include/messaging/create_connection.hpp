#ifndef MESSAGING__CREATE_CONNECTION_HPP
#define MESSAGING__CREATE_CONNECTION_HPP

#include <messaging/tcp_connection.hpp>

namespace messaging {

struct create_connection_impl : boost::static_visitor<connection::ptr> {
  template<
    typename Io,
    typename Callback,
    typename ErrorCallback,
    typename Endpoint
  >
  struct result {
    typedef connection::ptr type;
  };

  template<typename Callback, typename ErrorCallback>
  connection::ptr operator()(
      asio::io_service& io,
      const Callback& callback,
      const ErrorCallback& error_callback,
      const asio::ip::tcp::endpoint& ep
    ) const {
    typedef tcp_connection<Callback, ErrorCallback> conn;
    boost::shared_ptr<conn> p(new conn(io, callback, error_callback, ep));
    p->postconstructor();
    return p;
  }

  // Written as a template to prevent implicit conversion from other things to
  // generic_endpoint
  template<typename Callback, typename ErrorCallback, typename GenericEndpoint>
  typename boost::enable_if<
      typename boost::is_same<GenericEndpoint, generic_endpoint>::type,
      listener::ptr
    >::type
  operator()(
      asio::io_service& io,
      const Callback& callback,
      const ErrorCallback& error_callback,
      const GenericEndpoint& ep
    ) const {
    return ep.apply_visitor(
        create_connection(io, callback, error_callback, _1)
      );
  }

  BOOST_STATIC_ASSERT(PHOENIX_LIMIT >= 4);
};

px::function<create_connection_impl> create_connection;

}

#endif // MESSAGING__CREATE_CONNECTION_HPP

