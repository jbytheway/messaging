#ifndef MESSAGING__ERROR_SOURCE_HPP
#define MESSAGING__ERROR_SOURCE_HPP

namespace messaging {

enum error_source {
  error_source_accept,
  error_source_connect,
  error_source_read,
  error_source_write
};

inline std::ostream& operator<<(std::ostream& o, const error_source es)
{
  switch (es) {
#define MESSAGING_CASE(val)  \
    case error_source_##val: \
      o << #val;             \
      break;
    MESSAGING_CASE(accept)
    MESSAGING_CASE(connect)
    MESSAGING_CASE(read)
    MESSAGING_CASE(write)
#undef MESSAGING_CASE
    default:
      o << int(es);
  }
  return o;
}

}

#endif // MESSAGING__ERROR_SOURCE_HPP

