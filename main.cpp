#include <iostream>
#include <unistd.h>
#include "hash_ts.hpp"
using namespace std;


int main()
{
	HashTs<string, string> hashS(2);
	hashS.add("aa", "aaaaaa");
	hashS.add("ab", "bbbbbb");
	hashS.print();
	
	HashTs<int64_t, int64_t> hashI(1);
	hashI.add(33, 23);
	hashI.add(33, 25);
	hashI.print();

	void *p1 = malloc(10);
	void *p2 = malloc(10);
	memcpy(p1, "123456789", 9);
	memcpy(p2, "abcdefghi", 9);
	HashTs<string, void*> hashV(10);
	hashV.add("aa", p1);
	hashV.add("bb", p2);
	Node<string, void*> *node = hashV.findLock("bb");
	hashV.del("bb");
	hashV.findUnlock("bb");
	hashV.del("bb");
    	free(p1);
    	free(p2);
	return 0;
}
