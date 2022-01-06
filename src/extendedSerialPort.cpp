#include "extendedSerialPort.hpp"

ExtendedSerialPort::ExtendedSerialPort(boost::asio::io_service& ioService) : boost::asio::serial_port(ioService) {}

bool ExtendedSerialPort::IsDataAvailable()
{
    int value = 0;
    ::ioctl(native_handle(), FIONREAD, &value);
    return value != 0;
}