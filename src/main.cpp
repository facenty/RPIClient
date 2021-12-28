#include <wiringPi.h>

#include <utility>
#include <memory>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <functional>
#include <experimental/optional>
#include <experimental/string_view>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/log/trivial.hpp>
#include <boost/asio/high_resolution_timer.hpp>

namespace
{

  constexpr const char kATCommand[] = "AT\r\n";
  constexpr const char kOKReply[] = "OK";
  constexpr const char kErrorReply[] = "ERROR";
  constexpr const char kSerialName[] = "/dev/serial0";
  using namespace std::chrono_literals;
  // using namespace std::placeholders;

  auto initialize()
  {
    if (wiringPiSetup() != 0)
    {
      std::cerr << "wiringPiSetup() error" << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }

  struct ValueOrError {
    bool success() {
      return bool(result);
    }
    std::experimental::optional<std::string> error;
    std::experimental::optional<std::string> result;
  };

  class Sim800Gprs {
    public:
      using ResultCallback = std::function<void(ValueOrError)>;
      using Timeout = boost::asio::high_resolution_timer;
      Sim800Gprs(boost::asio::serial_port& serialPort) : serialPort_(serialPort), ioService_(serialPort_.get_io_service()), timeout_(ioService_) {
        // tmpBuffer_.reserve(1024);
        // result = {'A', 'T', '\n', 'O', 'K'};
        // std::cout << ContainsOk() << std::endl;

      }
      void ExecuteAtCommand(const std::vector<char>& atCommand, ResultCallback cb, std::chrono::milliseconds timeout = 2s) {
        cb_ = std::move(cb);
        timeout_.expires_from_now(timeout);
        serialPort_.write_some(boost::asio::buffer(atCommand));
        serialPort_.async_read_some(boost::asio::buffer(tmpBuffer_),
            boost::bind(&Sim800Gprs::ReadSomeUntilResultOrTimeout, this, boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
        timeout_.async_wait(boost::bind(&Sim800Gprs::OnTimeout, this, boost::asio::placeholders::error));
        timeouted_ = false;
      }

    private:
      bool ContainsError() {
        return Contains(kErrorReply);
      }

      bool ContainsOk() {
        return Contains(kOKReply);
      }

      bool Contains(const std::string& searchWord) {
        auto it = std::search(result.begin(), result.end(), searchWord.begin(), searchWord.end());
        return it != result.end();
      }

      void ReadSomeUntilResultOrTimeout(const boost::system::error_code &error, std::size_t readBytes) {
        if (readBytes) {
          std::cout << readBytes << std::endl;
        }
        if (timeouted_ == true) {
          return;
        }
        if (error) {
          PostCallbackWithError(error.message());
          return;
        }
        result.insert(result.end(), tmpBuffer_.begin(), tmpBuffer_.begin() + readBytes );
        std::cout << "##############" << std::endl;
        for (auto it : result) {
          std::cout << "[" << std::hex << static_cast<int>(it) << "] ";
        }
        std::cout << std::endl;
        std::cout << "OK: " << ContainsOk() << " ERROR: " << ContainsError() << std:: endl;
        if (!ContainsError() && !ContainsOk()) {
        std::cout << "Here reading" << std::endl;
          serialPort_.async_read_some(boost::asio::buffer(tmpBuffer_),
            boost::bind(&Sim800Gprs::ReadSomeUntilResultOrTimeout, this, boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));

          return;
        }
        if (ContainsError()) {
          PostCallbackWithError(std::string(result.begin(), result.end()));
          return; 
        }
        std::cout << "Here" << std::endl;
        PostCallbackWithResult(std::string(result.begin(), result.end()));
      }

      void OnTimeout(const boost::system::error_code& error)
      {
        if (!error) {
          timeouted_ = true;
          PostCallbackWithError("Request timeouted");
        }
      }

      void PostCallbackWithError(const std::string& error) {
        ValueOrError vor;
        vor.error = error;
        PostCallbackWithValueOrError(std::move(vor));
      }

      void PostCallbackWithResult(const std::string& result) {
        ValueOrError vor;
        vor.result = result;
        PostCallbackWithValueOrError(std::move(vor));
      }

      void PostCallbackWithValueOrError(ValueOrError vor) {
        timeout_.cancel();
        ioService_.post(std::bind(std::move(cb_), std::move(vor)));
      }

    private:
      boost::asio::serial_port& serialPort_;
      boost::asio::io_service& ioService_;
      std::vector<char> tmpBuffer_ = std::vector<char>(1024);
      std::vector<char> result;
      Timeout timeout_;
      bool timeouted_;
      ResultCallback cb_;
  };

} // namespace

int main(int argc, char *argv[])
{
  initialize();

  // std::cout << "Boost version: " 
  //         << BOOST_VERSION / 100000
  //         << "."
  //         << BOOST_VERSION / 100 % 1000
  //         << "."
  //         << BOOST_VERSION % 100 
  //         << std::endl;

  // std::vector<std::string> args;
  // args.reserve(argc);
  // for (auto i = 0u; i < static_cast<uint>(argc); ++i)
  //   args.push_back(argv[i]);

  boost::system::error_code ec;
  boost::asio::io_service io_service;
  boost::asio::serial_port serial_port(io_service);
  serial_port.open(kSerialName, ec);
  if (ec)
  {
    BOOST_LOG_TRIVIAL(fatal) << "serial port open(), failed port name " << kSerialName;
    return EXIT_FAILURE;
  }
  serial_port.set_option(boost::asio::serial_port::baud_rate(115200));


  Sim800Gprs Sim800Gprs(serial_port);
  Sim800Gprs.ExecuteAtCommand({kATCommand, kATCommand + strlen(kATCommand)}, [](ValueOrError voe) {
    std::cout << "Success: " << voe.success() ? "YES" : "NO";
    if (voe.success()) {
      std::cout << *voe.result << std::endl;
      return;
    }
    std::cout << *voe.error << std::endl;
  });
  
  io_service.run();
  return EXIT_SUCCESS;
}