#ifndef MESSAGING__DETAIL__HASH_HPP
#define MESSAGING__DETAIL__HASH_HPP

#include <boost/spirit/phoenix/functions.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>

#include <messaging/core.hpp>

// Need (alas) to define some stuff in namespace owned by boost
namespace boost {

template<>
struct hash<asio::ip::address> {
  size_t operator()(const asio::ip::address& a) const {
    // TODO: better efficiency possible
    return hash_value(a.to_string());
  }
};

}

namespace messaging { namespace detail {

// We need an overload of hash_value for boost::blank
inline size_t hash_value(const boost::blank&) {
  // Value courtesy of /dev/random
  return 0xe5c5b3a7;
}

// and for various asio types
template<typename Protocol>
struct hash_tag;

template<>
struct hash_tag<asio::ip::tcp> {
  static const size_t value = 0xa816b2ea;
};

template<>
struct hash_tag<asio::ip::udp> {
  static const size_t value = 0x086f62e9;
};

template<typename Protocol>
inline size_t hash_value(const asio::ip::basic_endpoint<Protocol>& ep) {
  size_t seed = hash_tag<Protocol>::value;
  boost::hash_combine(seed, ep.address());
  boost::hash_combine(seed, ep.port());
  return seed;
}

struct hash_impl : boost::static_visitor<size_t> {
  template<typename T>
  struct result {
    typedef size_t type;
  };

  template<typename T>
  size_t operator()(const T& t) {
    using boost::hash_value;
    using detail::hash_value;
    return hash_value(t);
  }
};

}}

#endif // MESSAGING__DETAIL__HASH_HPP

