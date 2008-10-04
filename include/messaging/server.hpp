#ifndef MESSAGING__SERVER_HPP
#define MESSAGING__SERVER_HPP

#include <boost/noncopyable.hpp>
#include <boost/fusion/include/make_vector.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/spirit/phoenix/primitives.hpp>
#include <boost/spirit/phoenix/statements.hpp>
#include <boost/spirit/phoenix/operators.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include <messaging/generic_endpoint.hpp>
#include <messaging/listener.hpp>
#include <messaging/create_listener.hpp>
#include <messaging/detail/insert.hpp>
//#include <messaging/detail/ptr_hash.hpp>

namespace messaging {

template<typename Callback>
class server : boost::noncopyable {
  public:
    server(
        asio::io_service& io,
        const Callback& callback,
        const generic_endpoint& ep1 = generic_endpoint(),
        const generic_endpoint& ep2 = generic_endpoint(),
        const generic_endpoint& ep3 = generic_endpoint(),
        const generic_endpoint& ep4 = generic_endpoint()
      ) :
      io_(io),
      callback_(callback)
    {
      initialize_listeners(fusion::make_vector(ep1, ep2, ep3, ep4));
    }

    template<typename ForwardSequence>
    server(
        asio::io_service& io,
        const Callback& callback,
        const ForwardSequence& endpoints
      ) :
      io_(io),
      callback_(callback)
    {
      initialize_listeners(endpoints);
    }
  private:
    template<typename ForwardSequence>
    void initialize_listeners(const ForwardSequence& endpoints) {
      listener::ptr t;
      fusion::for_each(
          endpoints,
          px::if_(px::var(t) = create_listener(px::arg1)) [
            detail::insert(px::var(listeners_), px::var(t))
          ]
        );
    }

    asio::io_service& io_;
    const Callback& callback_;
    typedef boost::multi_index_container<
      listener::ptr,
      boost::multi_index::indexed_by<
        boost::multi_index::hashed_unique<
          BOOST_MULTI_INDEX_CONST_MEM_FUN(listener, generic_endpoint, endpoint)
        >/*,
      boost::multi_index::hashed_unique<
          boost::multi_index::tag<PointerTag>,
          boost::multi_index::identity<listener::ptr>,
          detail::ptr_hash
        >*/
      >
    > Listeners;
    Listeners listeners_;
};

}

#endif // MESSAGING__SERVER_HPP

