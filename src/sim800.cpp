#include "sim800.hpp"

#include <boost/log/trivial.hpp>
#include <boost/bind.hpp>

#include <functional>


namespace
{
  constexpr const char kErrorReply[] = "ERROR";
}

Sim800::Sim800(ExtendedSerialPort& serialPort) : serialPort_(serialPort),
ioService_(serialPort_.get_io_service()),
timeout_(ioService_) {}

void Sim800::Execute(const std::string& atCommand, std::vector<std::string> expectedResult, StringResultCallback cb, std::chrono::milliseconds timeout, bool clearNewLines)
{
  PreExecute(atCommand, std::move(cb), timeout, clearNewLines);
  expectedResult_ = std::move(expectedResult);
  serialPort_.async_read_some(boost::asio::buffer(tmpBuffer_),
    boost::bind(&Sim800::ReadSomeUntilPredicateOrTimeout, this,
      [this](const std::vector<char>& buffer) {
        return ContainsExpectedResult();
      }, boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred));
}

void Sim800::Execute(const std::string& atCommand, std::size_t amountOfCharactersToReadAtLeast, StringResultCallback cb, std::chrono::milliseconds timeout, bool clearNewLines) {
  PreExecute(atCommand, std::move(cb), timeout, clearNewLines);
  serialPort_.async_read_some(boost::asio::buffer(tmpBuffer_),
    boost::bind(&Sim800::ReadSomeUntilPredicateOrTimeout, this,
      [this, amountOfCharactersToReadAtLeast](const std::vector<char>& buffer) {
        return buffer.size() >= amountOfCharactersToReadAtLeast;
      }, boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred));
}

void Sim800::PreExecute(const std::string& atCommand, StringResultCallback cb, std::chrono::milliseconds timeout, bool clearNewLines) {
  auto commandCopy = std::string(atCommand);
  auto commandToLog = std::string(commandCopy.begin(), std::remove(commandCopy.begin(), std::remove(commandCopy.begin(), commandCopy.end(), '\n'), '\r'));
  BOOST_LOG_TRIVIAL(info) << "Executing command: [ " << commandToLog << " ]";
  result_.clear();
  clearNewLines_ = clearNewLines;
  cb_ = std::move(cb);
  serialPort_.write_some(boost::asio::buffer(atCommand));
  timeout_.expires_from_now(timeout);
  timeout_.async_wait(boost::bind(&Sim800::OnTimeout, this, commandToLog, boost::asio::placeholders::error));
  timeouted_ = false;
}


bool Sim800::ContainsError()
{
  return Contains({ {kErrorReply} });
}

bool Sim800::ContainsExpectedResult()
{
  return Contains(expectedResult_);
}

bool Sim800::Contains(const std::vector<std::string>& searchWords)
{
  auto it = result_.begin();
  for (const auto& word : searchWords)
  {
    it = std::search(it, result_.end(), word.begin(), word.end());
    if (it == result_.end())
    {
      return false;
    }
    it += word.size();
  }
  return true;
}

void Sim800::ReadSomeUntilPredicateOrTimeout(std::function<bool(const std::vector<char>& buffer)> predicate, const boost::system::error_code& error, std::size_t readBytes)
{
  if (timeouted_ == true) {
    return;
  }
  if (error) {
    BOOST_LOG_TRIVIAL(error) << "This error ocurred during reading the data " << error.message();
    PostCallbackWithResult(std::experimental::nullopt);
    return;
  }
  result_.insert(result_.end(), tmpBuffer_.begin(), tmpBuffer_.begin() + readBytes);
  if ((!ContainsError() && !predicate(result_)) || serialPort_.IsDataAvailable()) {
    serialPort_.async_read_some(boost::asio::buffer(tmpBuffer_),
      boost::bind(&Sim800::ReadSomeUntilPredicateOrTimeout, this, predicate, boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
    return;
  }
  if (ContainsError()) {
    BOOST_LOG_TRIVIAL(error) << "Response contains ERROR message";
    PostCallbackWithResult(std::experimental::nullopt);
    return;
  }
  if (clearNewLines_) {
    auto endWithoutNewLines = std::remove(result_.begin(), result_.end(), '\n');
    auto endWithoutCarriageReturn = std::remove(result_.begin(), endWithoutNewLines, '\r');
    PostCallbackWithResult(std::string(result_.begin(), endWithoutCarriageReturn));
    BOOST_LOG_TRIVIAL(info) << "Result [ " << std::string(result_.begin(), endWithoutCarriageReturn) << " ]";
    return;
  }
  BOOST_LOG_TRIVIAL(info) << "Result [ " << std::string(result_.begin(), result_.end()) << " ]";
  PostCallbackWithResult(std::string(result_.begin(), result_.end()));
}

void Sim800::OnTimeout(std::string command, const boost::system::error_code& error) {
  if (!error) {
    timeouted_ = true;
    BOOST_LOG_TRIVIAL(error) << "Request [ " << command << " ]timeouted";
    PostCallbackWithResult(std::experimental::nullopt);
  }
}

void Sim800::PostCallbackWithResult(OptionalString result) {
  timeout_.cancel();
  PostCallbackWithArgs(cb_, std::move(result));
}