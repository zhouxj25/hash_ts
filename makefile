hashts: hash_ts.h hash_ts.hpp main.cpp 
	g++ -std=c++11 -lpthread -g2 -o2 *cpp -o hashts
clean:
	rm -rf *.o hashts
