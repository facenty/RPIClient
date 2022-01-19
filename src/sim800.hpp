#ifndef SIM_800_HPP
#define SIM_800_HPP

#include <chrono>
#include <experimental/optional>
#include <utility>

#include <boost/asio.hpp>
#include <boost/asio/high_resolution_timer.hpp>

#include "extendedSerialPort.hpp"

namespace
{
    using namespace std::chrono_literals;
    std::chrono::milliseconds kDefaultTimeout = 2s;
}

class Sim800
{
public:
    using Timeout = boost::asio::high_resolution_timer;
    using OptionalString = std::experimental::optional<std::string>;
    using StringResultCallback = std::function<void(OptionalString)>;

    Sim800(ExtendedSerialPort& serialPort);
    virtual ~Sim800() = default;

protected:
    void Execute(const std::string& atCommand, std::vector<std::string> expectedResult, StringResultCallback cb,
        std::chrono::milliseconds timeout = kDefaultTimeout, bool clearNewLines = true);
    void ReadAmountOfData(std::size_t amountOfCharactersToRead, StringResultCallback cb);
    void ReadSomeUntilContainsOrWord(const std::vector<std::string>& expectedResult, const std::string& word, StringResultCallback cb);

    template<typename... U>
    void PostCallbackWithArgs(std::function<void(U...)> cb, U&&... args) {
        ioService_.post(std::bind(std::move(cb), std::forward<U>(args)...));
    };

private:
    void PreExecute(const std::string& atCommand, StringResultCallback cb, std::chrono::milliseconds timeout, bool clearNewLines);
    bool ContainsError();
    bool ContainsExpectedResult();
    std::pair<bool, std::vector<char>::iterator> Contains(std::vector<char>& buffer, const std::vector<std::string>& searchWords);
    void ReadSomeUntilPredicate(std::function<bool(const std::vector<char>& buffer)> predicate,
        StringResultCallback resultCb,
        const boost::system::error_code& error, std::size_t readBytes);
    void ReadSomeUntilPredicateOrTimeout(std::function<bool(const std::vector<char>& buffer)> predicate,
        const boost::system::error_code& error, std::size_t readBytes);
    void OnTimeout(std::string command, const boost::system::error_code& error);
    void PostCallbackWithResult(StringResultCallback cb, std::experimental::optional<std::string> result);


private:
    ExtendedSerialPort& serialPort_;
    boost::asio::io_service& ioService_;
    std::vector<std::string> expectedResult_;
    std::vector<char> tmpBuffer_ = std::vector<char>(1024);
    std::vector<char> result_;
    std::vector<char> specialResult_;
    Timeout timeout_;
    bool timeouted_;
    StringResultCallback cb_;
    bool clearNewLines_;
};

#endif // SIM_800_HPP