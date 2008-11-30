#ifndef MESSAGING__MESSAGE_TYPE_HPP
#define MESSAGING__MESSAGE_TYPE_HPP

#include <boost/mpl/range_c.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/begin.hpp>

#include <messaging/core.hpp>
#include <messaging/detail/get_message_type.hpp>

namespace messaging {

template<typename Protocol, typename Message>
inline typename Protocol::message_type_type message_type()
{
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
  typedef typename mpl::find<messages, Message>::type message_it;
  typedef typename mpl::distance<
    typename mpl::begin<messages>::type, message_it
  >::type this_index;
  BOOST_STATIC_ASSERT((this_index::value >= 0));
  BOOST_STATIC_ASSERT((this_index::value < Protocol::max_message_type));
  return type_type(this_index::value);
}

}

#endif // MESSAGING__MESSAGE_TYPE_HPP

