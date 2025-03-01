#pragma once

#include <folly/futures/Future.h>

#include <string>
#include <vector>


namespace folly {
class EventBase;
}


folly::Future<std::vector<std::string>> performance_test(folly::EventBase*);
