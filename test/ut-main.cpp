/*
    This is a "generic" main program for unit tests.

    It will run all unit tests linked with it.

*/

#include <iostream>
using std::cerr;
using std::cout;

#define TRACING 0
#define SELFTEST_IMPLEMENTATION
#include "selftest.hpp"

int main( int argc, char *argv[] )
{
    selftest::trace << "Starting test sequence.\n\n\n";
    auto fails = selftest::runUnitTests();

    if ( 0==fails.numFailedTests ) {
        cout << "\n\nUnit testing success. "
             << fails.numTests 
             << " tests completed.\n\n";
        return 0;
    } else {
        cerr << "\n\nUnit testing failure. "
             << fails.numFailedTests 
             << " of "
             << fails.numTests
             << " tests failed.\n\n";
        return 1;
    }
}

