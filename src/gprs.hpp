#ifndef GPRS_HPP
#define GPRS_HPP

#include <chrono>

#include <boost/asio.hpp>
#include <boost/asio/high_resolution_timer.hpp>

#include "extendedSerialPort.hpp"

namespace
{
    using namespace std::chrono_literals;
    std::chrono::milliseconds kDefaultTimeout = 2s;
}

class Sim800Gprs
{
public:
    using Timeout = boost::asio::high_resolution_timer;
    using BoolResultCallback = std::function<void(bool)>;

    Sim800Gprs(ExtendedSerialPort& serialPort);
    void Init();
    void Join(const std::string& apnName);
    void StartTCP(const std::string& address, std::size_t port);
    void ReadTCPData();
    void CloseTCP();
    void GetIPAddress();
    void Execute(const std::string& atCommand, std::vector<std::string> expectedResult, BoolResultCallback cb, std::chrono::milliseconds timeout = kDefaultTimeout);

private:
    bool ContainsError();
    bool ContainsExpectedResult();
    bool Contains(const std::vector<std::string>& searchWords);
    void ReadSomeUntilResultOrTimeout(const boost::system::error_code& error, std::size_t readBytes);
    void OnTimeout(const boost::system::error_code& error);
    void PostCallbackWithResult(bool success);

private:
    boost::asio::serial_port& serialPort_;
    boost::asio::io_service& ioService_;
    std::vector<std::string> expectedResult_;
    std::vector<char> tmpBuffer_ = std::vector<char>(1024);
    std::vector<char> result;
    Timeout timeout_;
    bool timeouted_;
    BoolResultCallback cb_;
};

#endif // GPRS_HPP