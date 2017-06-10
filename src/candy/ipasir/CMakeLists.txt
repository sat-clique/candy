add_library(ipasir STATIC
    ipasircandy.cc
    ipasir.h)
    
target_link_libraries(ipasir simp core sonification utils ${ZLIB_LIBRARIES})


