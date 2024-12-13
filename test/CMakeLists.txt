
add_executable(test
    HttpClient.cpp
    TransactionHandler.cpp

    main.cpp
)


target_link_libraries(test
PUBLIC
    proxygenhttpserver
    proxygen                                                                          
    wangle                                                                            
    fizz                                                                              
    folly                                                                             
                                                                                      
    unwind                                                                            
    sodium                                                                            

    dl                                                                                
    gflags                                                                            
    glog                                                                              
    pthread                                                                           
    fmt
    ssl                                                                               
    lzma                                                                              
    lz4                                                                               
    snappy                                                                            
    crypto                                                                            
    z                                                                                 
    zstd                                                                              
    iberty                                                                            
    event                                                                             
    double-conversion                                                                 
    bz2                                                                               
                                                                                      
    boost_context                                                                     
)

