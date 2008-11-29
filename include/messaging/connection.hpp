#ifndef MESSAGING__CONNECTION_HPP
#define MESSAGING__CONNECTION_HPP

namespace messaging {

class connection :
  public boost::enable_shared_from_this<connection>,
  private boost::noncopyable {
  public:
    typedef boost::shared_ptr<connection> ptr;

    virtual void close() = 0;
    virtual void close_gracefully() = 0;
    virtual void send(const std::string& data) = 0;
  protected:
    connection() {}
};

}

#endif // MESSAGING__CONNECTION_HPP

