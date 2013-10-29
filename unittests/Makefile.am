CXXFLAGS += -I../packages/googletest/include/ -I../include/ -std=c++0x
LDFLAGS = -pthread
LIBS = ../packages/googletest/libgtest.a

TESTS = ptrtest aptrtest lptrtest laptrtest cstringtest cstdiotest

check_PROGRAMS = ptrtest aptrtest lptrtest laptrtest cstringtest cstdiotest

ptrtest_SOURCES = ptrsuite.cc main.cc

aptrtest_SOURCES = aptrsuite.cc main.cc

lptrtest_SOURCES = lptrsuite.cc main.cc

laptrtest_SOURCES = laptrsuite.cc main.cc

cstringtest_SOURCES = cstringsuite.cc main.cc

cstdiotest_SOURCES = cstdiosuite.cc main.cc
