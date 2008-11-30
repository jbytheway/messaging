#ifndef MESSSAGING__CORE_HPP
#define MESSSAGING__CORE_HPP

#include <boost/spirit/home/phoenix/core/argument.hpp>

namespace boost {
  namespace asio {}
  namespace fusion {}
  namespace mpl {}
}

namespace messaging {
  namespace asio = boost::asio;
  namespace fusion = boost::fusion;
  namespace mpl = boost::mpl;
  namespace px = boost::phoenix;

  using px::arg_names::arg1;
  using px::arg_names::arg2;
  using px::arg_names::arg3;
  using px::arg_names::arg4;
  using px::arg_names::arg5;

  using boost::system::error_code;
}

#endif // MESSSAGING__CORE_HPP

