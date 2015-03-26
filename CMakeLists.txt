cmake_minimum_required(VERSION 2.8.3)
project(ironclad)


find_package( Threads )


## System dependencies are found with CMake's conventions
# find_package(Boost REQUIRED COMPONENTS system)


###########
## Build ##
###########


## Specify additional locations of header files
## Your package locations should be listed before other locations
 include_directories(include)

################################
# GTest
################################
ADD_SUBDIRECTORY (packages/google-test)
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})


################################
# Unit Tests
################################
# Add test cpp file
add_executable( ptrtest unittests/ptrsuite.cc unittests/main.cc )
target_link_libraries(ptrtest gtest gtest_main ${CMAKE_THREAD_LIBS_INIT})
add_test( ptrtest ptrtest )

add_executable( aptrtest unittests/aptrsuite.cc unittests/main.cc )
target_link_libraries(aptrtest gtest gtest_main ${CMAKE_THREAD_LIBS_INIT})
add_test( aptrtest aptrtest )

add_executable( lptrtest unittests/lptrsuite.cc unittests/main.cc)
target_link_libraries(lptrtest gtest gtest_main ${CMAKE_THREAD_LIBS_INIT})
add_test( lptrtest lptrtest )

add_executable( laptrtest unittests/laptrsuite.cc unittests/main.cc)
target_link_libraries(laptrtest gtest gtest_main ${CMAKE_THREAD_LIBS_INIT})
add_test( laptrtest laptrtest )

add_executable( cstringtest unittests/cstringsuite.cc unittests/main.cc)
target_link_libraries(cstringtest gtest gtest_main ${CMAKE_THREAD_LIBS_INIT})
add_test( cstringtest cstringtest )

add_executable( cstdiotest unittests/cstdiosuite.cc unittests/main.cc)
target_link_libraries(cstdiotest gtest gtest_main ${CMAKE_THREAD_LIBS_INIT})
add_test( cstdiotest cstdiotest )