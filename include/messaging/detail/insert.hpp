#ifndef MESSAGING__DETAIL__INSERT_HPP
#define MESSAGING__DETAIL__INSERT_HPP

namespace messaging { namespace detail {

// This one isn't messaging-library specific; it's a generic Phoenix insertion
// function, the likes of which will doubtless soon be in Phoenix proper (if
// they're not already)
struct insert_impl {
  template<typename Container, typename Item>
  struct result {
    typedef std::pair<typename Container::iterator, bool> type;
  };

  template<typename Container, typename Item>
  std::pair<typename Container::iterator, bool>
  operator()(Container& c, const Item& i) {
    return c.insert(i);
  }
};

px::function<insert_impl> insert;

}}

#endif // MESSAGING__DETAIL__INSERT_HPP

