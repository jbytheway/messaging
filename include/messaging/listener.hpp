#ifndef MESSAGING__LISTENER_HPP
#define MESSAGING__LISTENER_HPP

#include <boost/enable_shared_from_this.hpp>

namespace messaging {

class listener :
  public boost::enable_shared_from_this<listener>,
  private boost::noncopyable {
  public:
    typedef boost::shared_ptr<listener> ptr;

    virtual ~listener() = 0;

    virtual generic_endpoint endpoint() const = 0;
    virtual void close() = 0;
};

inline listener::~listener() {}

}

#endif // MESSAGING__LISTENER_HPP

