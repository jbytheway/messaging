#ifndef MESSAGING__DETAIL__GET_MESSAGE_TYPE_HPP
#define MESSAGING__DETAIL__GET_MESSAGE_TYPE_HPP

namespace messaging { namespace detail {

template<typename Protocol, typename Index>
struct get_message_type
{
  typedef typename Protocol::template message_type_from_index<Index>::type type;
};

}}

#endif // MESSAGING__DETAIL__GET_MESSAGE_TYPE_HPP

