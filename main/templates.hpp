/*
 * singleton.hpp
 *
 *  Created on: Jan 22, 2018
 *      Author: ksu
 */

#ifndef SINGLETON_HPP_
#define SINGLETON_HPP_

#include <memory>

namespace tpl {

template<typename T>
class Singleton {
public:
    static T& instance()
    {
        static const std::unique_ptr<T> instance{new T{}};
        return *instance;
    }

    Singleton(const Singleton&) = delete;
    Singleton& operator= (const Singleton) = delete;

protected:
    struct token {};
    Singleton() {}
};

template< typename T, size_t N >
constexpr size_t countof( const T (&)[N] ) { return N; }


}

#endif /* SINGLETON_HPP_ */
