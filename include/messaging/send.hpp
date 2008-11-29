#ifndef MESSAGING__SEND_HPP
#define MESSAGING__SEND_HPP

#include <sstream>

#include <messaging/message_type.hpp>

namespace messaging {

template<typename Protocol, typename Message, typename ConnectionPtr>
void send(const Message& message, const ConnectionPtr& connection)
{
  typedef typename Protocol::oarchive_type archive_type;
  std::ostringstream os;
  archive_type oa(os);
  typename Protocol::message_type_type type = message_type<Protocol, Message>();
  oa << BOOST_SERIALIZATION_NVP(type) << BOOST_SERIALIZATION_NVP(message);
  connection->send(os.str());
}

}

#endif // MESSAGING__SEND_HPP

