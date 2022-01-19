#include <wiringPi.h>

#include <memory>
#include <iostream>
#include <functional>
#include <iomanip>
#include <ctime>

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

  enum class ClientType : bool {
    PUBLISHER = 0,
    SUBSCRIBER = 1,
  };

  std::string ClientTypeToString(ClientType clientType) {
    if (clientType == ClientType::PUBLISHER) return "PUBLISHER";
    return "SUBSCRIBER";
  }

  ClientType DeduceClientType(const std::string& ct) {
    if (ct == "PUBLISHER") return ClientType::PUBLISHER;
    return ClientType::SUBSCRIBER;
  }

  class App {
  public:
    using Timeout = boost::asio::high_resolution_timer;
    App(ClientType ct) : ioService_(), serialPort_(ioService_), gprs_(serialPort_), ct_(ct), timeout_(ioService_) {};

    void DoStuff() {
      serialPort_.open(kSerialName, ec_);
      if (ec_) {
        BOOST_LOG_TRIVIAL(fatal) << "serial port open(), failed port name " << kSerialName;
        std::exit(EXIT_FAILURE);
      }
      serialPort_.set_option(boost::asio::serial_port::baud_rate(115200));
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
      gprs_.StartConnection("chodowicz.pl", 9999, Gprs::ConnectionType::TCP, std::bind(&App::StartConnectionCb, this, std::placeholders::_1));
    }

    void OnConnectionClosed(bool success) {
      if (!success) {
        std::exit(EXIT_FAILURE);
        return;
      }
      gprs_.ShutConnection(std::bind(&App::OnConnectionShut, this, std::placeholders::_1));
    }

    void OnConnectionShut(bool success) {
      if (!success) {
        std::exit(EXIT_FAILURE);
        return;
      }
      std::exit(EXIT_SUCCESS);
    }

    void GetIpCb(Gprs::OptionalString result) {
      if (result) {
        BOOST_LOG_TRIVIAL(info) << "IP size: " << result->size();
        BOOST_LOG_TRIVIAL(info) << "IP: " << result.value();
        return;
      }
      std::exit(EXIT_FAILURE);
    }

    void StartConnectionCb(bool result) {
      if (!result) {
        std::exit(EXIT_FAILURE);
        return;
      }
      std::string data = "{\"ClientType\":\"" + ClientTypeToString(ct_) + "\"}";
      gprs_.SendData({ data.begin(), data.end() }, std::bind(&App::OnHandshakeSend, this, std::placeholders::_1));
    }

    void OnHandshakeSend(bool result) {
      if (!result) {
        BOOST_LOG_TRIVIAL(error) << "Failed to send handshake";
        gprs_.ShutConnection(std::bind(&App::OnConnectionShut, this, std::placeholders::_1));
        return;
      }
      gprs_.StartReading(std::bind(&App::OnHandshakeResponse, this, std::placeholders::_1));
    }

    void OnHandshakeResponse(Gprs::OptionalString result) {
      if (!result || result.value().find("OK") == std::string::npos) {
        BOOST_LOG_TRIVIAL(error) << "Failed to handshake";
        gprs_.ShutConnection(std::bind(&App::OnConnectionShut, this, std::placeholders::_1));
        return;
      }
      if (ct_ == ClientType::SUBSCRIBER) {
        gprs_.StartReading(std::bind(&App::OnData, this, std::placeholders::_1));
        return;
      }
      SendData();
    }

    void OnData(Gprs::OptionalString result) {
      if (!result) {
        BOOST_LOG_TRIVIAL(error) << "Failed to read or connection closed";
        gprs_.ShutConnection(std::bind(&App::OnConnectionShut, this, std::placeholders::_1));
        return;
      }
      BOOST_LOG_TRIVIAL(info) << "Data: [ " << result.value() << " ]";
      // if (count_ < 3) {
      // count_++;
      gprs_.StartReading(std::bind(&App::OnData, this, std::placeholders::_1));
      // return;
      // }
    }

    void SendData() {
      auto t = std::time(nullptr);
      auto tm = *std::localtime(&t);
      std::ostringstream oss;
      oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
      std::string data = oss.str();
      gprs_.SendData({ data.begin(), data.end() }, std::bind(&App::OnDataSend, this, std::placeholders::_1));
    }

    void OnDataSend(bool result) {
      if (!result) {
        BOOST_LOG_TRIVIAL(error) << "Failed to send data";
        gprs_.ShutConnection(std::bind(&App::OnConnectionShut, this, std::placeholders::_1));
        return;
      }
      if (ct_ == ClientType::SUBSCRIBER) {
        gprs_.StartReading(std::bind(&App::OnData, this, std::placeholders::_1));
        return;
      }
      using namespace std::chrono_literals;
      timeout_.expires_from_now(2s);
      timeout_.async_wait(std::bind(&App::OnTimeout, this, std::placeholders::_1));
    }


    void OnTimeout(const boost::system::error_code& error) {
      if (!error) {
        SendData();
      }
    }

    boost::system::error_code ec_;
    boost::asio::io_service ioService_;
    ExtendedSerialPort serialPort_;
    Gprs gprs_;
    ClientType ct_;
    int count_ = 0;
    Timeout timeout_;
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
  if (argc != 2) {
    BOOST_LOG_TRIVIAL(fatal) << "Wrong number of parameters";
    return EXIT_FAILURE;
  }
  App app(DeduceClientType(argv[1]));
  app.DoStuff();

  return EXIT_SUCCESS;
}