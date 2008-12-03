#ifndef MESSAGING__GENERIC_ENDPOINT_HPP
#define MESSAGING__GENERIC_ENDPOINT_HPP

#include <boost/variant/variant.hpp>
#include <boost/variant/apply_visitor.hpp>

#include <messaging/detail/hash.hpp>

namespace messaging {

class generic_endpoint : boost::operators<generic_endpoint> {
  public:
    typedef boost::variant<
        boost::blank,
        asio::ip::tcp::endpoint,
        asio::ip::udp::endpoint
      > some_endpoint;

    generic_endpoint(const some_endpoint& v = boost::blank()) :
      value_(v)
    {}

    template<typename Endpoint>
    generic_endpoint(const Endpoint& v) :
      value_(v)
    {}

    template<typename Visitor>
    typename Visitor::result_type apply_visitor(const Visitor& v) const {
      return boost::apply_visitor(v, value_);
    }

    bool operator==(const generic_endpoint& r) const {
      return value_ == r.value_;
    }

    friend size_t hash_value(const generic_endpoint& ep) {
      detail::hash_impl h;
      size_t seed = boost::apply_visitor(h, ep.value_);
      boost::hash_combine(seed, ep.value_.which());
      return seed;
    }
  private:
    some_endpoint value_;
};

}

#endif // MESSAGING__GENERIC_ENDPOINT_HPP

