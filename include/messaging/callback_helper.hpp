#ifndef MESSAGING__CALLBACK_HELPER_HPP
#define MESSAGING__CALLBACK_HELPER_HPP

#include <messaging/error_source.hpp>

namespace messaging {

template<typename Called>
class callback_helper {
  public:
    callback_helper(Called& c) : called_(c) {}
    void operator()(const error_source es, const error_code& ec) const {
      called_.error(es, ec);
    }
    template<typename Connection>
    void operator()(Connection& c) const {
      called_.new_connection(c);
    }
    template<typename Message, typename Connection>
    void operator()(const Message& message, Connection& c) const {
      called_.message(message, c);
    }
  private:
    Called& called_;
};

}

#endif // MESSAGING__CALLBACK_HELPER_HPP

