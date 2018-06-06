if(MSVC)
    # candy links against the dynamic MS CRT by default,
    # which can't be mixed with code using the static CRT.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

add_subdirectory(googletest)
add_subdirectory(oscpack)
