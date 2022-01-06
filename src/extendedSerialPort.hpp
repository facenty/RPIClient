#ifndef EXTENDED_SERIAL_PORT_HPP
#define EXTENDED_SERIAL_PORT_HPP

#include <iostream>
#include <boost/asio/serial_port.hpp>
#include <boost/asio/io_service.hpp>

class ExtendedSerialPort : public boost::asio::serial_port
{
public:
  ExtendedSerialPort(boost::asio::io_service& ioService);
  bool IsDataAvailable();
};

#endif // EXTENDED_SERIAL_PORT_HPP