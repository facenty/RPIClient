#include "gprs.hpp"

#include <boost/log/trivial.hpp>
#include <boost/bind.hpp>


namespace
{
  constexpr const char kErrorReply[] = "ERROR";
}

Sim800Gprs::Sim800Gprs(ExtendedSerialPort& serialPort) : serialPort_(serialPort),
ioService_(serialPort_.get_io_service()),
timeout_(ioService_) {}

void Sim800Gprs::Init()
{
}
void Sim800Gprs::Join(const std::string& apnName)
{
}
void Sim800Gprs::StartTCP(const std::string& address, std::size_t port)
{
}
void Sim800Gprs::ReadTCPData()
{
}
void Sim800Gprs::CloseTCP()
{
}
void Sim800Gprs::GetIPAddress()
{
}

void Sim800Gprs::Execute(const std::string& atCommand, std::vector<std::string> expectedResult, BoolResultCallback cb, std::chrono::milliseconds timeout)
{
  cb_ = std::move(cb);
  expectedResult_ = std::move(expectedResult);
  timeout_.expires_from_now(timeout);
  serialPort_.write_some(boost::asio::buffer(atCommand));
  serialPort_.async_read_some(boost::asio::buffer(tmpBuffer_),
    boost::bind(&Sim800Gprs::ReadSomeUntilResultOrTimeout, this, boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred));
  timeout_.async_wait(boost::bind(&Sim800Gprs::OnTimeout, this, boost::asio::placeholders::error));
  timeouted_ = false;
}

bool Sim800Gprs::ContainsError()
{
  return Contains({ {kErrorReply} });
}

bool Sim800Gprs::ContainsExpectedResult()
{
  return Contains(expectedResult_);
}

bool Sim800Gprs::Contains(const std::vector<std::string>& searchWords)
{
  auto it = result.begin();
  for (const auto& word : searchWords)
  {
    it = std::search(it, result.end(), word.begin(), word.end());
    if (it == result.end())
    {
      return false;
    }
    it += word.size();
  }
  return true;
}

void Sim800Gprs::ReadSomeUntilResultOrTimeout(const boost::system::error_code& error, std::size_t readBytes)
{
  if (timeouted_ == true) {
    return;
  }
  if (error) {
    BOOST_LOG_TRIVIAL(error) << "This error ocurred during reading the data " << error.message();
    PostCallbackWithResult(false);
    return;
  }
  result.insert(result.end(), tmpBuffer_.begin(), tmpBuffer_.begin() + readBytes);
  if (!ContainsError() && !ContainsExpectedResult()) {
    serialPort_.async_read_some(boost::asio::buffer(tmpBuffer_),
      boost::bind(&Sim800Gprs::ReadSomeUntilResultOrTimeout, this, boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
    return;
  }
  if (ContainsError()) {
    BOOST_LOG_TRIVIAL(error) << "Response contains ERROR message";
    PostCallbackWithResult(false);
    return;
  }
  PostCallbackWithResult(true);
}

void Sim800Gprs::OnTimeout(const boost::system::error_code& error) {
  if (!error) {
    timeouted_ = true;
    BOOST_LOG_TRIVIAL(error) << "Request timeouted";
    PostCallbackWithResult(false);
  }
}

void Sim800Gprs::PostCallbackWithResult(bool success) {
  timeout_.cancel();
  ioService_.post(std::bind(std::move(cb_), success));
}