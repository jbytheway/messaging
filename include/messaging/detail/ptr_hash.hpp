#ifndef MESSAGING__DETAIL__PTR_HASH_H
#define MESSAGING__DETAIL__PTR_HASH_H

namespace messaging { namespace detail {

struct ptr_hash {
  template<typename T>
  size_t operator()(const boost::shared_ptr<T>& p) const {
    return reinterpret_cast<size_t>(p.get());
  }
};

}}

#endif // MESSAGING__DETAIL__PTR_HASH_H
