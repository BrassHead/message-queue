# Simple Makefile
#

test: ../Build/ut-channel
	../Build/ut-channel

../Build/ut-channel: ut-channel.cpp ../channel.hpp ../External/selftest/selftest.hpp 
	mkdir -p ../Build
	c++ -std=c++11 -I../External/selftest -I.. -Wall -Werror -g -DDEBUG -o ../Build/ut-channel ut-channel.cpp

../External/selftest/selftest.hpp:
	mkdir -p ../External
	git clone https://github.com/BrassHead/selftest.git ../External/selftest 

clean:
	rm -rf ../Build

