#include "scopedFd.hpp"

#include <unistd.h>

ScopedFd::ScopedFd() : value_(-1) {}
ScopedFd::ScopedFd(int value) : value_(value) {}
ScopedFd::~ScopedFd() { clear(); }
ScopedFd::ScopedFd(ScopedFd&& other) : value_(other.release()) {}

ScopedFd& ScopedFd::operator=(ScopedFd&& s) {
    reset(s.release());
    return *this;
}

void ScopedFd::reset(int new_value) {
    if (value_ != -1) {
        close(value_);
    }
    value_ = new_value;
}

void ScopedFd::clear() {
    reset(-1);
}

int ScopedFd::get() const { return value_; }

int ScopedFd::release() {
    int ret = value_;
    value_ = -1;
    return ret;
}