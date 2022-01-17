#include "gprs.hpp"

#include <boost/log/trivial.hpp>
#include <boost/bind.hpp>

#include <functional>


namespace
{
  constexpr const char kErrorReply[] = "ERROR";
  std::string ConnectionTypeToString(const Gprs::ConnectionType& ct) {
    switch (ct) {
    case Gprs::ConnectionType::TCP:
      return "TCP";
    case Gprs::ConnectionType::UDP:
      return "UDP";
    }
    return "";
  }

  std::ostream& operator << (std::ostream& os, const Gprs::ConnectionType& obj) {
    os << ConnectionTypeToString(obj);
    return os;
  }
}


Gprs::Gprs(ExtendedSerialPort& serialPort) : Sim800(serialPort) {}

void Gprs::Init(BoolResultCallback cb) {
  auto cfunCb = [=](OptionalString success) {
    if (!success) {
      BOOST_LOG_TRIVIAL(error) << "set cfun failed";
      this->PostCallbackWithArgs(cb, false);
      return;
    }
    CheckSimStatus(cb);
  };
  auto atTestCb = [=](OptionalString success) {
    if (!success) {
      this->PostCallbackWithArgs(cb, false);
      return;
    }
    Execute("AT+CFUN=1\r\n", { {"OK"} }, cfunCb);
  };
  Execute("AT\r\n", { {"OK"} }, atTestCb);
}

void Gprs::Join(const std::string& apnName, BoolResultCallback cb) {

  auto connectGprsCb = [this, cb](OptionalString result) {
    this->PostCallbackWithArgs(cb, bool(result));
  };

  auto setApnCb = [cb, connectGprsCb, this](OptionalString result) {
    if (!result) {
      BOOST_LOG_TRIVIAL(error) << "set apn failed";
      this->PostCallbackWithArgs(cb, false);
      return;
    }
    // Bring up gprs connection
    Execute("AT+CIICR\r\n", { {"OK"} }, connectGprsCb);
  };

  auto checkApnCb = [cb, setApnCb, apnName, this](OptionalString result) {
    if (result) {
      // We have got aps set to the one that we want so we should bring up gprs connection
      setApnCb("apn set previously");
      return;
    }
    // Set the apn
    Execute("AT+CSTT=\"" + apnName + "\",\"\",\"\"\r\n", { {"OK"} }, setApnCb);
  };

  auto shutCb = [cb, setApnCb, checkApnCb, apnName, this](bool result) {
    if (!result) {
      BOOST_LOG_TRIVIAL(error) << "shut gprs failed";
      this->PostCallbackWithArgs(cb, false);
      return;
    }
    // Check if current apn is the one that we want
    // Execute("AT+CSTT?\r\n", { {apnName} }, checkApnCb);
    Execute("AT+CSTT=\"" + apnName + "\",\"\",\"\"\r\n", { {"OK"} }, setApnCb);

  };
  ShutConnection(shutCb);
}

void Gprs::StartConnection(const std::string& address, std::size_t port, ConnectionType connectionType, BoolResultCallback cb) {
  std::ostringstream cmd;
  cmd << "AT+CIPSTART = \"" << connectionType << "\",\"" << address << "\"," << port << "\r\n";
  using namespace std::chrono_literals;
  Execute(cmd.str(), { {"OK"}, {"CONNECT OK"} }, [cb, this](OptionalString result) {
    PostCallbackWithArgs(cb, bool(result));
    }, 6s);
}

void Gprs::SendData(const std::vector<char>& data, BoolResultCallback cb) {
  std::ostringstream cmd;
  auto sendWhenPossible = [cb, data, this](OptionalString result) {
    if (!result) {
      PostCallbackWithArgs(cb, false);
      return;
    }
    Execute(std::string(data.begin(), data.end()), { {"SEND"}, {"OK"} }, [cb, this](OptionalString result) {
      PostCallbackWithArgs(cb, bool(result));
      });
  };
  cmd << "AT+CIPSEND=" << data.size() << "\r\n";
  Execute(cmd.str(), { {">"} }, sendWhenPossible);
}

void Gprs::ReadData(StringResultCallback cb) {
}

void Gprs::CloseTCP() {
}

void Gprs::GetIPAddress(StringResultCallback cb) {
  Execute("AT+CIFSR\r\n", { {"."},{"."},{"."},{"\n"} }, cb);
}

void Gprs::ShutConnection(BoolResultCallback cb) {
  Execute("AT+CIPSHUT\r\n", { {"OK"}, {"SHUT OK"} },
    [cb, this](OptionalString result) {
      PostCallbackWithArgs(cb, bool(result));
    });
}

void Gprs::CheckSimStatusCb(BoolResultCallback cb, OptionalString success) {
  if (success) {
    PostCallbackWithArgs(cb, true);
    return;
  }
  if (retryCount_ < 3) {
    ++retryCount_;
    Execute("AT+CPIN?\r\n", { {"+CPIN: READY"} }, std::bind(&Gprs::CheckSimStatusCb, this, std::move(cb), std::placeholders::_1));
    return;
  }
  BOOST_LOG_TRIVIAL(error) << "Check sim status failed";
  PostCallbackWithArgs(cb, false);
}

void Gprs::CheckSimStatus(BoolResultCallback cb) {
  retryCount_ = 0;
  Execute("AT+CPIN?\r\n", { {"+CPIN: READY"} }, std::bind(&Gprs::CheckSimStatusCb, this, std::move(cb), std::placeholders::_1));
}