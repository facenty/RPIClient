#include <wiringPi.h>

#include <memory>
#include <iostream>
#include <functional>

#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/log/trivial.hpp>

#include "gprs.hpp"

namespace
{
  constexpr const char kATCommand[] = "AT\r\n";
  constexpr const char kOKReply[] = "OK";
  constexpr const char kSerialName[] = "/dev/serial0";

  auto initialize()
  {
    if (wiringPiSetup() != 0)
    {
      std::cerr << "wiringPiSetup() error" << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }

  class App {
  public:
    App() : ioService_(), serialPort_(ioService_), gprs_(serialPort_) {};

    void DoStuff() {
      serialPort_.open(kSerialName, ec_);
      if (ec_) {
        BOOST_LOG_TRIVIAL(fatal) << "serial port open(), failed port name " << kSerialName;
        std::exit(EXIT_FAILURE);
      }
      serialPort_.set_option(boost::asio::serial_port::baud_rate(115200));
      // gprs_ = std::make_unique<Gprs>(ioService_);
      gprs_.Init(std::bind(&App::InitCb, this, std::placeholders::_1));
      ioService_.run();
    }

    void InitCb(bool success) {
      if (!success) {
        std::exit(EXIT_FAILURE);
        return;
      }
      gprs_.Join("plus", std::bind(&App::JoinCb, this, std::placeholders::_1));
    }

    void JoinCb(bool success) {
      if (!success) {
        std::exit(EXIT_FAILURE);
        return;
      }
      // gprs_.GetIPAddress(std::bind(&App::GetIpCb, this, std::placeholders::_1));
      gprs_.StartConnection("chodowicz.pl", 9999, Gprs::ConnectionType::TCP, std::bind(&App::GetIpCb, this, std::placeholders::_1));
    }

    void GetIpCb(Gprs::OptionalString result) {
      if (result) {
        std::cout << "IP size: " << result->size() << std::endl;
        std::cout << "IP: " << result.value() << std::endl;
        return;
      }
      std::cout << "IP get error" << std::endl;
      std::exit(EXIT_FAILURE);
    }

    void StartConnectionCb(bool result) {
      if (!result) {
        std::cout << "Failed to connect;" << std::endl;
        std::exit(EXIT_FAILURE);
        return;
      }
    }

    boost::system::error_code ec_;
    boost::asio::io_service ioService_;
    ExtendedSerialPort serialPort_;
    Gprs gprs_;
  };

} // namespace

int main(int argc, char* argv[])
{
  initialize();

  // std::cout << "Boost version: "
  //         << BOOST_VERSION / 100000
  //         << "."
  //         << BOOST_VERSION / 100 % 1000
  //         << "."
  //         << BOOST_VERSION % 100
  //         << std::endl;

  // boost::system::error_code ec;
  // boost::asio::io_service io_service;
  // ExtendedSerialPort serial_port(io_service);
  // serial_port.open(kSerialName, ec);
  // if (ec) {
  //   BOOST_LOG_TRIVIAL(fatal) << "serial port open(), failed port name " << kSerialName;
  //   return EXIT_FAILURE;
  // }
  // serial_port.set_option(boost::asio::serial_port::baud_rate(115200));

  App app;
  app.DoStuff();

  return EXIT_SUCCESS;
}