#ifndef __SCOPED_FD_HPP__
#define __SCOPED_FD_HPP__

class ScopedFd {
public:
    ScopedFd();
    explicit ScopedFd(int value);
    ~ScopedFd();
    ScopedFd(ScopedFd&& other);
    ScopedFd& operator=(ScopedFd&& s);
    void reset(int new_value);
    void clear();
    int get() const;
    int release();
private:
    ScopedFd(const ScopedFd&) = delete;
    void operator=(const ScopedFd&) = delete;
    int value_;
};

#endif // __UTILS_H__shutdown -r 0