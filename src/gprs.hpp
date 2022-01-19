#ifndef GPRS_HPP
#define GPRS_HPP

#include <chrono>
#include <experimental/optional>

#include <boost/asio.hpp>
#include <boost/asio/high_resolution_timer.hpp>

#include "extendedSerialPort.hpp"
#include "sim800.hpp"

class Gprs : public Sim800 {
public:
    using BoolResultCallback = std::function<void(bool)>;
    enum class ConnectionType {
        TCP = 0,
        UDP = 1,
    };
    Gprs(ExtendedSerialPort& serialPort);
    void Init(BoolResultCallback cb);
    void Join(const std::string& apnName, BoolResultCallback cb);
    void StartConnection(const std::string& address, std::size_t port, ConnectionType connectionType, BoolResultCallback cb);
    void SendData(const std::vector<char>& data, BoolResultCallback cb);
    void StartReading(Sim800::StringResultCallback dataPartCb);
    void CloseTCP(BoolResultCallback cb);
    void ShutConnection(BoolResultCallback cb);
    void GetIPAddress(Sim800::StringResultCallback cb);


private:
    void CheckSimStatusCb(BoolResultCallback cb, OptionalString success);
    void CheckSimStatus(BoolResultCallback cb);

private:
    uint retryCount_ = 0;
    std::experimental::optional<BoolResultCallback> stopReadingCb_;
};

#endif // GPRS_HPP