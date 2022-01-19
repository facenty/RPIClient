#include "sim800.hpp"

#include <boost/log/trivial.hpp>
#include <boost/bind.hpp>

#include <functional>


namespace
{
  constexpr const char kErrorReply[] = "ERROR";
  std::string RemoveWhitespaces(std::string txt) {
    txt.erase(std::remove(txt.begin(), std::remove(txt.begin(), txt.end(), '\n'), '\r'), txt.end());
    return txt;
  }
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

void Sim800::ReadAmountOfData(std::size_t amountOfCharactersToRead, StringResultCallback cb) {

  auto readCb = [this, amountOfCharactersToRead, cb](OptionalString result) {
    if (!result) {
      PostCallbackWithResult(cb, std::experimental::nullopt);
      return;
    }
    PostCallbackWithResult(cb, std::string(specialResult_.begin(), specialResult_.begin() + amountOfCharactersToRead));
    specialResult_.erase(specialResult_.begin(), specialResult_.begin() + amountOfCharactersToRead);
  };

  if (specialResult_.size() >= amountOfCharactersToRead) {
    readCb(std::string(specialResult_.begin(), specialResult_.end()));
    return;
  }

  serialPort_.async_read_some(boost::asio::buffer(tmpBuffer_),
    boost::bind(&Sim800::ReadSomeUntilPredicate, this, [this, amountOfCharactersToRead](const std::vector<char>& buffer) {
      return buffer.size() >= amountOfCharactersToRead;
      }, readCb, boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred));

}

void Sim800::ReadSomeUntilContainsOrWord(const std::vector<std::string>& expectedResult, const std::string& word, StringResultCallback cb) {
  auto readCb = [this, expectedResult, cb](OptionalString result) {
    if (!result) {
      PostCallbackWithResult(std::move(cb), std::experimental::nullopt);
      return;
    }
    auto expectedResultInfo = Contains(specialResult_, expectedResult);
    if (expectedResultInfo.first) {
      PostCallbackWithResult(cb, std::string(specialResult_.begin(), expectedResultInfo.second));
      specialResult_.erase(specialResult_.begin(), expectedResultInfo.second);
      return;
    }
    PostCallbackWithResult(cb, std::experimental::nullopt);
  };
  serialPort_.async_read_some(boost::asio::buffer(tmpBuffer_),
    boost::bind(&Sim800::ReadSomeUntilPredicate, this, [this, expectedResult, word](const std::vector<char>& buffer) {
      return Contains(specialResult_, expectedResult).first || Contains(specialResult_, { word }).first;
      }, readCb, boost::asio::placeholders::error,
      boost::asio::placeholders::bytes_transferred));
}

void Sim800::PreExecute(const std::string& atCommand, StringResultCallback cb, std::chrono::milliseconds timeout, bool clearNewLines) {
  auto commandToLog = RemoveWhitespaces(atCommand);
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
  return Contains(result_, { {kErrorReply} }).first;
}

bool Sim800::ContainsExpectedResult()
{
  return Contains(result_, expectedResult_).first;
}

std::pair<bool, std::vector<char>::iterator> Sim800::Contains(std::vector<char>& buffer, const std::vector<std::string>& searchWords)
{
  auto it = buffer.begin();
  for (const auto& word : searchWords)
  {
    it = std::search(it, buffer.end(), word.begin(), word.end());
    if (it == buffer.end())
    {
      return std::make_pair(false, it);
    }
    it += word.size();
  }
  return std::make_pair(true, it);
}

void Sim800::ReadSomeUntilPredicate(std::function<bool(const std::vector<char>& buffer)> predicate,
  StringResultCallback resultCb,
  const boost::system::error_code& error, std::size_t readBytes) {

  if (error) {
    BOOST_LOG_TRIVIAL(error) << "This error ocurred during reading the data " << error.message();
    PostCallbackWithResult(resultCb, std::experimental::nullopt);
    return;
  }
  specialResult_.insert(specialResult_.end(), tmpBuffer_.begin(), tmpBuffer_.begin() + readBytes);
  if (!predicate(specialResult_)) {
    serialPort_.async_read_some(boost::asio::buffer(tmpBuffer_),
      boost::bind(&Sim800::ReadSomeUntilPredicate, this, predicate, resultCb, boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
    return;
  }
  PostCallbackWithResult(resultCb, std::string(specialResult_.begin(), specialResult_.end()));
}

void Sim800::ReadSomeUntilPredicateOrTimeout(std::function<bool(const std::vector<char>& buffer)> predicate, const boost::system::error_code& error, std::size_t readBytes)
{
  if (timeouted_ == true) {
    return;
  }
  if (error) {
    BOOST_LOG_TRIVIAL(error) << "This error ocurred during reading the data " << error.message();
    PostCallbackWithResult(cb_, std::experimental::nullopt);
    return;
  }
  result_.insert(result_.end(), tmpBuffer_.begin(), tmpBuffer_.begin() + readBytes);
  if (!ContainsError() && !predicate(result_)) {
    serialPort_.async_read_some(boost::asio::buffer(tmpBuffer_),
      boost::bind(&Sim800::ReadSomeUntilPredicateOrTimeout, this, predicate, boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
    return;
  }
  auto withoutWhitespaces = RemoveWhitespaces(std::string(result_.begin(), result_.end()));
  BOOST_LOG_TRIVIAL(info) << "Result [ " << withoutWhitespaces << " ]";
  if (ContainsError()) {
    BOOST_LOG_TRIVIAL(error) << "Response contains ERROR message";
    BOOST_LOG_TRIVIAL(error) << "Response ";
    PostCallbackWithResult(cb_, std::experimental::nullopt);
    return;
  }
  if (clearNewLines_) {
    // auto endWithoutNewLines = std::remove(result_.begin(), result_.end(), '\n');
    // auto endWithoutCarriageReturn = std::remove(result_.begin(), endWithoutNewLines, '\r');
    PostCallbackWithResult(cb_, withoutWhitespaces);
    return;
  }
  PostCallbackWithResult(cb_, std::string(result_.begin(), result_.end()));
}

void Sim800::OnTimeout(std::string command, const boost::system::error_code& error) {
  if (!error) {
    timeouted_ = true;
    BOOST_LOG_TRIVIAL(error) << "Request [ " << command << " ]timeouted";
    PostCallbackWithResult(cb_, std::experimental::nullopt);
  }
}

void Sim800::PostCallbackWithResult(StringResultCallback cb, OptionalString result) {
  timeout_.cancel();
  PostCallbackWithArgs(cb, std::move(result));
}