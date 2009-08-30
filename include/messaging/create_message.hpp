#ifndef MESSAGING__CREATE_MESSAGE_HPP
#define MESSAGING__CREATE_MESSAGE_HPP

#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/mpl/integral_c.hpp>
#include <boost/integer_traits.hpp>
#include <boost/serialization/nvp.hpp>

#include <messaging/detail/message_variant.hpp>

#ifndef MESSAGING_MAX_MESSAGE_INDEX
#define MESSAGING_MAX_MESSAGE_INDEX 128
#endif

namespace messaging {

namespace detail {

template<typename Protocol, typename Protocol::message_index_type index>
inline typename boost::enable_if<
  mpl::bool_<(index < Protocol::max_message_index)>,
  typename detail::message_variant<Protocol>::type
>::type
create_specific_message(typename Protocol::iarchive_type& ia)
{
  typedef boost::mpl::integral_c<
      typename Protocol::message_index_type, index
    > t_index;
  typename Protocol::template message_type_from_index<t_index>::type message;
  ia >> BOOST_SERIALIZATION_NVP(message);
  return message;
}

template<typename Protocol, typename Protocol::message_index_type index>
inline typename boost::disable_if<
  mpl::bool_<(index < Protocol::max_message_index)>,
  typename detail::message_variant<Protocol>::type
>::type
create_specific_message(typename Protocol::iarchive_type&)
{
  std::ostringstream os;
  os << "invalid message type " << index;
  throw std::runtime_error(os.str());
}

}

template<typename Protocol>
typename detail::message_variant<Protocol>::type
create_message(const std::string& s)
{
  std::istringstream is(s);
  typename Protocol::iarchive_type ia(is);
  typedef typename Protocol::message_index_type index_type;
  typedef typename boost::make_unsigned<index_type>::type unsigned_index_type;
  index_type type;
  ia >> BOOST_SERIALIZATION_NVP(type);
  BOOST_MPL_ASSERT_RELATION(
      MESSAGING_MAX_MESSAGE_INDEX, <=,
      boost::integer_traits<unsigned_index_type>::const_max
    );
  BOOST_MPL_ASSERT_RELATION(
      Protocol::max_message_index, <=, MESSAGING_MAX_MESSAGE_INDEX
    );
  switch (unsigned_index_type(type)) {
#define MESSAGING_CASE(z, value, _) \
    case value:                     \
      return detail::create_specific_message<Protocol, index_type(value)>(ia);
    BOOST_PP_REPEAT(MESSAGING_MAX_MESSAGE_INDEX, MESSAGING_CASE, _)
#undef MESSAGING_CASE
    default:
      {
        std::ostringstream os;
        os << "invalid message type " << type;
        throw std::runtime_error(os.str());
      }
  }
}

}

#endif // MESSAGING__CREATE_MESSAGE_HPP

