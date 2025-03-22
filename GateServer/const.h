#ifndef CONST_H
#define CONST_H
#include <functional>

enum ERROR_CODES {
    SUCCESS = 0,
    JSON_ERROR = 1001,
    RPC_FAILED = 1002,
};

class Defer {
public:
    Defer(std::function<void()> func) : _func(func) {

    }
    ~Defer() {
        _func();
    }
private:
    std::function<void()> _func;
};

#endif // CONST_H