include ../common/Makefile_common
all: BFHT.so

BFHT.so:BFHT.cpp BFHT.h bloom_filter.hpp
	$(CXX) $(CFLAGS) -fPIC -shared $< -o $@  

clean:
	rm -rf *.o *.so  
