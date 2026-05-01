COMPILE=g++ -Wall -std=c++11 -g -DLOGGING
all: babysat
babysat: babysat.cpp config.hpp makefile
	$(COMPILE) -o $@ $<
config.hpp: generate makefile
	./generate > $@
clean:
	rm -f babysat makefile config.hpp cnfs/*.err cnfs/*.log
format:
	clang-format -i babysat.cpp
test: all
	./cnfs/test.sh
.PHONY: all clean format test
