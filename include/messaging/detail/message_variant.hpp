#ifndef MESSAGING__DETAIL__MESSAGE_VARIANT_HPP
#define MESSAGING__DETAIL__MESSAGE_VARIANT_HPP

#include <boost/mpl/range_c.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/variant/variant.hpp>

#include <messaging/detail/get_message_type.hpp>

namespace messaging { namespace detail {

// Constructs from the protocol a variant which can be any of the message
// types
template<typename Protocol>
class message_variant {
  private:
    typedef typename Protocol::message_type_type type_type;
    typedef typename boost::make_unsigned<type_type>::type int_equivalent;
    typedef mpl::range_c<
      int_equivalent, 0, int_equivalent(Protocol::max_message_type)
    > message_types;
    typedef typename mpl::fold<
      message_types,
      mpl::vector<>::type,
      mpl::push_back<mpl::_1, detail::get_message_type<Protocol, mpl::_2> >
    >::type messages;
  public:
    typedef typename boost::make_variant_over<messages>::type type;
};

}}

#endif // MESSAGING__DETAIL__MESSAGE_VARIANT_HPP

