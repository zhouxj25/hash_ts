#include "hash_ts.h"
#include <iostream>
using namespace std;

static std::mutex RWLOCK;

uint32_t hashSrc(const unsigned char *buf, int len)
{
    unsigned int hash = 5381;

    while (len--)
        hash = ((hash << 5) + hash) + (*buf++); // hash * 33 + c
    return hash;
}

template<typename KT>
uint32_t hashFun(const KT &key, int size)
{
    return hashSrc((unsigned char*)&key, sizeof(key)) % size;
}

template<>
uint32_t hashFun(const VOIDPTR &key, int size)
{
    return hashSrc((unsigned char*)key, strlen((const char *)key)) % size;
}

template<>
uint32_t hashFun(const CHARPTR &key, int size)
{
    return hashSrc((unsigned char*)key, strlen((const char *)key)) % size;
}

template<>
uint32_t hashFun(const string &key, int size)
{
    return hashSrc((unsigned char*)key.c_str(), key.size()) % size;
}

template<typename KT, typename VT>
Node<KT, VT>::Node() : next(nullptr), empty(true), readNum(0)
{   
}

template<typename KT, typename VT>
void Node<KT, VT>::keepValue(const KT &k, const VT &v)
{
    key = k;
    value = v;    
}

template<typename KT, typename VT>
IdleNode<KT, VT>::IdleNode() : m_size(0), m_head(nullptr), m_tail(nullptr)
{
}
template<typename KT, typename VT>
IdleNode<KT, VT>::~IdleNode()
{
    while (m_head)
    {
        Node<KT, VT> *node = m_head->next;
        delete m_head;
        m_head = nullptr;
        m_head = node;    
    }
}

template<typename KT, typename VT>
void IdleNode<KT, VT>::add(Node<KT, VT> *node, int size)
{
    if (m_size == size)
    {
        delete node;
        node = nullptr;
        return;
    }
    
	node->empty = true;
	node->next = nullptr;
	if (0 == m_size)
	{
		m_head = node;
		m_tail = node;
	}
	else
	{
		m_tail->next = node;
		m_tail = node;
	}
	++m_size;
}

template<typename KT, typename VT>
Node<KT, VT> *IdleNode<KT, VT>::get()
{
	Node<KT, VT> *node(nullptr);
	if (0 == m_size)
	{
		node = new Node<KT, VT>;
	}
	else if (1 == m_size)
	{
		node = m_head;
		m_head = nullptr;
		m_tail = nullptr;
		--m_size;
	}
	else
	{
		node = m_head;
		m_head = node->next;
		--m_size;
	}
	node->empty = false;
	node->next = nullptr;
	
	return node;		
}

template<typename KT, typename VT>
HashTs<KT, VT>::HashTs() : m_indexSize(HASH_SIZE), m_totalNum(0)
{
	m_table = new Node<KT, VT>[m_indexSize];
}

template<typename KT, typename VT>
HashTs<KT, VT>::HashTs(int size) : m_indexSize(size), m_totalNum(0)
{
	m_table = new Node<KT, VT>[m_indexSize];
}

template<typename KT, typename VT>
HashTs<KT, VT>::~HashTs()
{
	if (m_table)
	{
	    for (int i = 0; i < m_indexSize; ++i)
	    {
	        Node<KT, VT> *node = (m_table + i)->next;
	        while (node)
	        {
	            Node<KT, VT> *next = node->next;
	            delete node;
	            node = nullptr;
	            node = next;    
	        }
	    }
	    
		delete[] m_table;
		m_table = nullptr;
	}
}

template<typename KT, typename VT>
void HashTs<KT, VT>::add(const KT &key, const VT &value)
{
    int index = hashFun(key, m_indexSize);
    std::lock_guard<std::mutex> lck(RWLOCK); 
    Node<KT, VT> *node(nullptr);
    if (node = find(key, index))
    {
        if (node->readNum > 0)
	    {
	        cout << "node[" << node->key << "], is read locked, can't modify!" << endl;
	        return;    
	    }
        node->value = value;
        return;
    }
    
    node = m_table + index;
    if (node->empty)
	{
		node->keepValue(key, value);
		node->empty = false;
	}
	else
	{
		node = m_idle.get();
		node->keepValue(key, value);
		Node<KT, VT> *tmp = m_table + index;
		while (tmp->next)
			tmp = tmp->next;
		tmp->next = node;
	}
	++m_totalNum;
}

template<typename KT, typename VT>
Node<KT, VT> *HashTs<KT, VT>::find(const KT &key)
{
	int index = hashFun(key, m_indexSize);
	std::lock_guard<std::mutex> lck(RWLOCK); 
	Node<KT, VT> *node = m_table + index;
	while (node)
	{
		if (equal(node, key))
	    {
			return node;
	    }
		else
			node = node->next;
	}

	return nullptr;
}

template<typename KT, typename VT>
Node<KT, VT>    *HashTs<KT, VT>::findLock(const KT &key)
{
    int index = hashFun(key, m_indexSize);
	std::lock_guard<std::mutex> lck(RWLOCK); 
	Node<KT, VT> *node = m_table + index;
	while (node)
	{
		if (equal(node, key))
	    {
	        node->readNum += 1;
			return node;
	    }
		else
			node = node->next;
	}

	return nullptr;
}

template<typename KT, typename VT>
void HashTs<KT, VT>::findUnlock(const KT &key)
{
    int index = hashFun(key, m_indexSize);
	std::lock_guard<std::mutex> lck(RWLOCK); 
	Node<KT, VT> *node = m_table + index;
	while (node)
	{
		if (equal(node, key))
	    {
	        node->readNum -= 1;
	        break;
	    }
		else
			node = node->next;
	}
}

template<typename KT, typename VT>
Node<KT, VT> *HashTs<KT, VT>::find(const KT &key, int index)
{ 
	Node<KT, VT> *node = m_table + index;
	while (node)
	{
		if (equal(node, key))
	    {
			return node;
	    }
		else
			node = node->next;
	}

	return nullptr;
}

template<typename KT, typename VT>
void HashTs<KT, VT>::HashTs::del(const KT &key)
{
	int index = hashFun(key, m_indexSize);
	std::lock_guard<std::mutex> lck(RWLOCK); 
	Node<KT, VT> *node = m_table + index;
	if (equal(node, key))
	{
	    if (node->readNum > 0)
	    {
	        cout << "node[" << node->key << "], is read locked, can't delete!" << endl;
	        return;    
	    }
		node->empty = true;
		--m_totalNum;
		return;
	}
	Node<KT, VT> *next = node->next;
	while (next)
	{
		if (equal(next, key))
		{
		    if (next->readNum > 0)
    	    {
    	        cout << "node[" << next->key << "], is read locked, can't delete!" << endl;
    	        return;    
    	    }
			node->next = next->next;
			m_idle.add(next, m_indexSize);
		    --m_totalNum;
			break;
		}
		node = next;
		next = next->next;
	}
}

template<typename KT, typename VT>
void HashTs<KT, VT>::print()
{
    for (int i = 0; i < m_indexSize; ++i)
        printIndex(i);
}

template<typename KT, typename VT>
void HashTs<KT, VT>::printIndex(int index)
{
    Node<KT, VT> *node = m_table + index;
    while (node)
    {
        if (node == m_table + index)
            cout << index << ":";
        if (!node->empty)
            cout << node->key;
        if (node->next)
            cout << "->";
        
        node = node->next;
    }
    cout << endl;
}

template<typename KT, typename VT>
bool HashTs<KT, VT>::equal(Node<KT, VT> *node, const KT &key)
{
    if (node->empty)
        return false;    
    
    if (node->key != key)
        return false;
    
    return true;
}

template<typename VT>
HashTs<CHARPTR, VT>::HashTs() : m_indexSize(HASH_SIZE), m_totalNum(0)
{
	m_table = new Node<CHARPTR, VT>[m_indexSize];
}

template<typename VT>
HashTs<CHARPTR, VT>::HashTs(int size) : m_indexSize(size), m_totalNum(0)
{
	m_table = new Node<CHARPTR, VT>[m_indexSize];
}

template<typename VT>
HashTs<CHARPTR, VT>::~HashTs()
{
	if (m_table)
	{
	    for (int i = 0; i < m_indexSize; ++i)
	    {
	        Node<CHARPTR, VT> *node = (m_table + i)->next;
	        while (node)
	        {
	            Node<CHARPTR, VT> *next = node->next;
	            delete node;
	            node = nullptr;
	            node = next;    
	        }
	    }
	    
		delete[] m_table;
		m_table = nullptr;
	}
}

template<typename VT>
void HashTs<CHARPTR, VT>::add(const CHARPTR key, const VT &value)
{
    int index = hashFun(key, m_indexSize);
    std::lock_guard<std::mutex> lck(RWLOCK); 
    Node<CHARPTR, VT> *node(nullptr);
    if (node = find(key, index))
    {
        if (node->readNum > 0)
	    {
	        cout << "node[" << node->key << "], is read locked, can't modify!" << endl;
	        return;    
	    }
        node->value = value;
        return;
    }
    node = m_table + index;
    if (node->empty)
	{
		node->keepValue(key, value);
		node->empty = false;
	}
	else
	{
		node = m_idle.get();
		node->keepValue(key, value);
		Node<CHARPTR, VT> *tmp = m_table + index;
		while (tmp->next)
			tmp = tmp->next;
		tmp->next = node;
	}
	++m_totalNum;
}

template<typename VT>
Node<CHARPTR, VT> *HashTs<CHARPTR, VT>::find(const CHARPTR key)
{
	int index = hashFun(key, m_indexSize);
	std::lock_guard<std::mutex> lck(RWLOCK); 
	Node<CHARPTR, VT> *node = m_table + index;
	while (node)
	{
		if (equal(node, key))
	    {
	        return node;
	    }
		else
			node = node->next;
	}

	return nullptr;
}

template<typename VT>
Node<CHARPTR, VT>   *HashTs<CHARPTR, VT>::findLock(const CHARPTR key)
{
    int index = hashFun(key, m_indexSize);
	std::lock_guard<std::mutex> lck(RWLOCK); 
	Node<CHARPTR, VT> *node = m_table + index;
	while (node)
	{
		if (equal(node, key))
	    {
	        node->readNum += 1;
			return node;
	    }
		else
			node = node->next;
	}

	return nullptr;
}

template<typename VT>
void HashTs<CHARPTR, VT>::findUnlock(const CHARPTR key)
{
    int index = hashFun(key, m_indexSize);
	std::lock_guard<std::mutex> lck(RWLOCK); 
	Node<CHARPTR, VT> *node = m_table + index;
	while (node)
	{
		if (equal(node, key))
	    {
	        node->readNum -= 1;
	        break;
	    }
		else
			node = node->next;
	}
}

template<typename VT>
Node<CHARPTR, VT> *HashTs<CHARPTR, VT>::find(const CHARPTR key, int index)
{
	Node<CHARPTR, VT> *node = m_table + index;
	while (node)
	{
		if (equal(node, key))
	    {
	        return node;
	    }
		else
			node = node->next;
	}

	return nullptr;
}

template<typename VT>
void HashTs<CHARPTR, VT>::HashTs::del(const CHARPTR key)
{
	int index = hashFun(key, m_indexSize);
	std::lock_guard<std::mutex> lck(RWLOCK); 
	Node<CHARPTR, VT> *node = m_table + index;
	if (equal(node, key))
	{
	    if (node->readNum > 0)
	    {
	        cout << "node[" << node->key << "], is read locked, can't delete!" << endl;
	        return;    
	    }
		node->empty = true;
		--m_totalNum;
		return;
	}
	Node<CHARPTR, VT> *next = node->next;
	while (next)
	{
		if (equal(next, key))
		{
		    if (next->readNum > 0)
    	    {
    	        cout << "node[" << next->key << "], is read locked, can't delete!" << endl;
    	        return;    
    	    }
			node->next = next->next;
			m_idle.add(next, m_indexSize);
		    --m_totalNum;
			break;
		}
		node = next;
		next = next->next;
	}
}

template<typename VT>
void HashTs<CHARPTR, VT>::print()
{
    for (int i = 0; i < m_indexSize; ++i)
        printIndex(i);
}

template<typename VT>
void HashTs<CHARPTR, VT>::printIndex(int index)
{
    Node<CHARPTR, VT> *node = m_table + index;
    while (node)
    {
        if (node == m_table + index)
            cout << index << ":";
        if (!node->empty)
            cout << node->key;
        if (node->next)
            cout << "->";
        
        node = node->next;
    }
    cout << endl;
}

template<typename VT>
bool HashTs<CHARPTR, VT>::equal(Node<CHARPTR, VT> *node, const CHARPTR key)
{
    if (node->empty)
        return false;    
    
    if (strcmp(node->key, key))
        return false;
    
    return true;
}