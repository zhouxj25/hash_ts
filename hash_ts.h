/*
 * multithread safe hash table, can lock the specified key avoid be update or delete.
 * @author zhouxj
 * @version 1.0, 2018-11-01
 */
#pragma once
#include <utility>
#include <string>
#include <mutex>
#include <string.h>
using namespace std;

#define HASH_SIZE 1024

typedef char* CHARPTR;
typedef void* VOIDPTR;

inline uint32_t hashSrc(const unsigned char *buf, int len);

template<typename KT>
uint32_t hashFun(const KT &key, int size);

template<>
inline uint32_t hashFun(const VOIDPTR &key, int size);

template<>
inline uint32_t hashFun(const CHARPTR &key, int size);

template<>
inline uint32_t hashFun(const string &key, int size);


template<typename KT, typename VT>
struct Node {
	Node();
	void keepValue(const KT &k, const VT &v);
	VT      value;
	KT      key;
	Node    *next;
	bool	empty;
	int     readNum;
};

template<>
struct Node<CHARPTR, CHARPTR> {
	Node() : next(nullptr), empty(true)
	{   
	}
	void keepValue(const CHARPTR k, const CHARPTR v)
	{
		key = new char[strlen(k)];
		value = new char[strlen(v)];
		memcpy(key, k, strlen(k));
		memcpy(value, v, strlen(v));
	}
	CHARPTR 	value;
	CHARPTR 	key;
	Node<CHARPTR, CHARPTR>    *next;
	bool		empty;
	int     	rLock;
};

template<typename KT>
struct Node<KT, CHARPTR> {
	Node() : next(nullptr), empty(true)
	{   
	}
	void keepValue(const KT k, const CHARPTR v)
	{
		key = k;
		value = new char[strlen(v)];
		memcpy(value, v, strlen(v));
	}
	CHARPTR 	value;
	KT 		key;
	Node<KT, CHARPTR>    *next;
	bool		empty;
};

template<typename VT>
struct Node<CHARPTR, VT> {
	Node() : next(nullptr), empty(true)
	{   
	}
	void keepValue(const CHARPTR k, const VT v)
	{
		key = new char[strlen(k)];
		value = v;
		memcpy(key, k, strlen(k));
	}
	VT 		value;
	CHARPTR 	key;
	Node<CHARPTR, VT>    *next;
	bool		empty;
};

template<typename KT, typename VT>
class IdleNode {
	public:
		IdleNode();
		~IdleNode();

		void add(Node<KT, VT> *node, int size);

		Node<KT, VT> *get();
	private:
		int 		m_size;
		Node<KT, VT> 	*m_head;
		Node<KT, VT> 	*m_tail;
};


template<typename KT, typename VT>
class HashTs {
	public:
		HashTs(); 
		HashTs(int size);
		~HashTs();

		Node<KT, VT>    *find(const KT &key);
		Node<KT, VT>    *findLock(const KT &key);
		void    findUnlock(const KT &key);
		void    add(const KT &key, const VT &value);
		void    del(const KT &key);
		void    print();
	private:
		Node<KT, VT>    *find(const KT &key, int index);
		void    printIndex(int index);
		bool    equal(Node<KT, VT> *node, const KT &key);
	private:
		int        	 	m_indexSize;
		int 			m_totalNum;
		Node<KT, VT> 		*m_table;
		IdleNode<KT, VT> 	m_idle;
};

template<typename VT>
class HashTs<CHARPTR, VT> {
	public:
		HashTs(); 
		HashTs(int size);
		~HashTs();

		Node<CHARPTR, VT>    *find(const CHARPTR key);
		void    add(const CHARPTR key, const VT &value);
		void    del(const CHARPTR key);
		void    print();
	private:
		Node<CHARPTR, VT>    *find(const CHARPTR key, int index);
		Node<CHARPTR, VT>    *findLock(const CHARPTR key);
		void    findUnlock(const CHARPTR key);
		void    printIndex(int index);
		bool    equal(Node<CHARPTR, VT> *node, const CHARPTR key);
	private:
		int         		m_indexSize;
		int 			m_totalNum;
		Node<CHARPTR, VT> 	*m_table;
		IdleNode<CHARPTR, VT> 	m_idle;
};
