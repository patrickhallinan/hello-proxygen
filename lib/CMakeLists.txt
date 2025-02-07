
add_library(httpclient STATIC
    HttpClient.cpp
    TransactionHandler.cpp
)


target_include_directories(httpclient
PUBLIC
    .
)
