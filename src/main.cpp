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

  // struct ValueOrError {
  //   bool success() {
  //     return bool(result);
  //   }
  //   std::experimental::optional<std::string> error;
  //   std::experimental::optional<std::string> result;
  // };

  // class Sim800Gprs {
  //   public:
  //     using BoolResultCallback = std::function<void(ValueOrError)>;
  //     using Timeout = boost::asio::high_resolution_timer;

  //     Sim800Gprs(boost::asio::serial_port& serialPort) :
  //         serialPort_(serialPort),
  //         ioService_(serialPort_.get_io_service()),
  //         timeout_(ioService_) {}

  //     void ExecuteAtCommand(const std::vector<char>& atCommand, BoolResultCallback cb, std::chrono::milliseconds timeout = kDefaultTimeout) {
  //       cb_ = std::move(cb);
  //       timeout_.expires_from_now(timeout);
  //       serialPort_.write_some(boost::asio::buffer(atCommand));
  //       serialPort_.async_read_some(boost::asio::buffer(tmpBuffer_),
  //           boost::bind(&Sim800Gprs::ReadSomeUntilResultOrTimeout, this, boost::asio::placeholders::error,
  //           boost::asio::placeholders::bytes_transferred));
  //       timeout_.async_wait(boost::bind(&Sim800Gprs::OnTimeout, this, boost::asio::placeholders::error));
  //       timeouted_ = false;
  //     }

  //   private:
  //     bool ContainsError() {
  //       return Contains({{kErrorReply}});
  //     }

  //     bool ContainsOk() {
  //       return Contains({{kOKReply}});
  //     }

  //     bool Contains(const std::vector<std::string>& searchWords) {
  //       auto it = result.begin();
  //       for (const auto& word : searchWords) {
  //         it = std::search(it, result.end(), word.begin(), word.end());
  //         if (it == result.end()) {
  //           return false;
  //         }
  //         it += word.size();
  //       }
  //       return true;
  //     }

  //     void ReadSomeUntilResultOrTimeout(const boost::system::error_code &error, std::size_t readBytes) {
  //       if (timeouted_ == true) {
  //         return;
  //       }
  //       if (error) {
  //         PostCallbackWithError(error.message());
  //         return;
  //       }
  //       result.insert(result.end(), tmpBuffer_.begin(), tmpBuffer_.begin() + readBytes );
  //       if (!ContainsError() && !ContainsOk()) {
  //         serialPort_.async_read_some(boost::asio::buffer(tmpBuffer_),
  //           boost::bind(&Sim800Gprs::ReadSomeUntilResultOrTimeout, this, boost::asio::placeholders::error,
  //           boost::asio::placeholders::bytes_transferred));

  //         return;
  //       }
  //       if (ContainsError()) {
  //         PostCallbackWithError(std::string(result.begin(), result.end()));
  //         return;
  //       }
  //       PostCallbackWithResult(std::string(result.begin(), result.end()));
  //     }

  //     void OnTimeout(const boost::system::error_code& error)
  //     {
  //       if (!error) {
  //         timeouted_ = true;
  //         PostCallbackWithError("Request timeouted");
  //       }
  //     }

  //     void PostCallbackWithError(const std::string& error) {
  //       ValueOrError vor;
  //       vor.error = error;
  //       PostCallbackWithValueOrError(std::move(vor));
  //     }

  //     void PostCallbackWithResult(const std::string& result) {
  //       ValueOrError vor;
  //       vor.result = result;
  //       PostCallbackWithValueOrError(std::move(vor));
  //     }

  //     void PostCallbackWithValueOrError(ValueOrError vor) {
  //       timeout_.cancel();
  //       ioService_.post(std::bind(std::move(cb_), std::move(vor)));
  //     }

  //   private:
  //     boost::asio::serial_port& serialPort_;
  //     boost::asio::io_service& ioService_;
  //     std::vector<char> tmpBuffer_ = std::vector<char>(1024);
  //     std::vector<char> result;
  //     Timeout timeout_;
  //     bool timeouted_;
  //     BoolResultCallback cb_;
  // };

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

  boost::system::error_code ec;
  boost::asio::io_service io_service;
  ExtendedSerialPort serial_port(io_service);
  serial_port.open(kSerialName, ec);
  if (ec) {
    BOOST_LOG_TRIVIAL(fatal) << "serial port open(), failed port name " << kSerialName;
    return EXIT_FAILURE;
  }
  serial_port.set_option(boost::asio::serial_port::baud_rate(115200));

  Sim800Gprs Sim800Gprs(serial_port);
  Sim800Gprs.Execute({ kATCommand }, { {kOKReply} }, [](bool success) {
    std::cout << "Success: " << (success ? "YES" : "NO");
    });
  io_service.run();
  return EXIT_SUCCESS;
}