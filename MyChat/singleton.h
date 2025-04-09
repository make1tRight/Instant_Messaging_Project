#ifndef SINGLETON_H
#define SINGLETON_H
#include "global.h"

template <typename T>
class Singleton {
protected:
    // 单例模式下, 直接初始化和复制初始化的操作都要delete掉
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator=(const Singleton<T>&) = delete;
    
    // 静态的成员变量, 等到程序结束以后才销毁; 如果用完销毁,单例模式就变成一次性的了
    static std::shared_ptr<T> m_instance;

public:
    static std::shared_ptr<T> GetInstance() {
        // 只在第一次调用的时候初始化s_flag
        static std::once_flag s_flag;
        std::call_once(s_flag, [&]() {
            // 这里不能用make_shared, 因为make_shared需要调用构造函数; 而在单例模式中, 构造函数不是公有的
            m_instance = std::shared_ptr<T>(new T);
            });
        return m_instance;
    }

    void PrintAddress() {
        std::cout << m_instance.get() << std::endl;
    }

    ~Singleton() {
        std::cout << "This is singleton destruct" << std::endl;
    }
};

template <typename T>
std::shared_ptr<T> Singleton<T>::m_instance = nullptr; //[3-12:53]这里要在函数体外进行实例化

#endif // SINGLETON_H
