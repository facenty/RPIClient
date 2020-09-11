#include <wiringPi.h>

#include <utility>
#include <memory>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <iomanip>
#include <experimental/string_view>

#include <boost/asio/serial_port.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/log/trivial.hpp>

namespace
{

  constexpr const char kATCommand[] = "AT\n";
  constexpr const char kOKReply[] = "OK";
  constexpr const char kErrorReply[] = "ERROR";
  constexpr const char kSerialName[] = "/dev/serial0";

  auto initialize()
  {
    if (wiringPiSetup() != 0)
    {
      std::cerr << "wiringPiSetup() error" << std::endl;
      std::exit(EXIT_FAILURE);
    }
  }

} // namespace

int main(int argc, char *argv[])
{
  initialize();

  std::vector<std::string> args;
  args.reserve(argc);
  for (auto i = 0u; i < static_cast<uint>(argc); ++i)
    args.push_back(argv[i]);

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

  std::vector<char> buffer(1024);
  serial_port.write_some(boost::asio::buffer(kATCommand));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  serial_port.async_read_some(boost::asio::buffer(buffer),
                              [&](const boost::system::error_code &error, std::size_t read_bytes) {
                                if (!error)
                                {
                                  buffer.resize(read_bytes);
                                  std::experimental::string_view sv(buffer.data(), read_bytes);
                                  BOOST_LOG_TRIVIAL(info) << "Read " << sv.size() << " characters: ";
                                  BOOST_LOG_TRIVIAL(info) << sv;

                                  //               BOOST_LOG_TRIVIAL(info) << "Read " << read_bytes << " characters: ";
                                  // for (auto i = 0u; i < read_bytes; ++i) {
                                  //   BOOST_LOG_TRIVIAL(info) << " [" << i << "] = [" << std::hex << static_cast<int>(buffer[i]) << "]";
                                  // }
                                }

                                else
                                {
                                  BOOST_LOG_TRIVIAL(info) << "Error: " << error.message();
                                }
                              });

  io_service.run();
  return EXIT_SUCCESS;
}