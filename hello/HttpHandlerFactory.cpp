#include "HttpHandlerFactory.h"


thread_local folly::EventBase* HttpHandlerFactory::eb_{nullptr};
