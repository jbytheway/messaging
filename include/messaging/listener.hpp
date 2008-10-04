#ifndef MESSAGING__LISTENER_HPP
#define MESSAGING__LISTENER_HPP

namespace messaging {

class listener : boost::noncopyable {
  public:
    typedef boost::shared_ptr<listener> ptr;

    virtual ~listener() = 0;

    virtual generic_endpoint endpoint() const = 0;
};

inline listener::~listener() {}

}

#endif // MESSAGING__LISTENER_HPP

