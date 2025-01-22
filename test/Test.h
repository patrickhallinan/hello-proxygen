#pragma once

#include "HttpClient.h"

#include <memory>
#include <source_location>


namespace folly {
class EventBase;
}


class Test {
public:
    Test(folly::EventBase& eventBase);

    void run();

private:
    folly::EventBase& eventBase_;

    std::unique_ptr<HttpClient> httpClient_;
};


#ifdef __cpp_lib_source_location
template<typename T, typename U>
void assert_equal(T const& a, U const& b,
                  const std::source_location& loc = std::source_location::current()) {

    if (a != b) {

        std::string msg =  fmt::format("Failed: '{}' != '{}'  -> {}:{}",
            a, b, loc.file_name(), loc.line());

        throw std::runtime_error(msg);
    }
}
#else

#define  assert_equal(a,b) { \
    if ((a) != (b)) { \
        std::string msg =  fmt::format("Failed: '{}' != '{}'  -> {}:{}", \
            (a), (b), __FILE__, __LINE__); \
        throw std::runtime_error(msg); \
    } \
}

#endif
