#pragma once

#include <unordered_map>
#include <vector>

static_assert(sizeof(size_t) == 8, "Compiling for 32-bit not supported!");

// Hash function: just digests memory, unless you specialize it
// to do something else (e.g. digest the contents of a string)
size_t HashMemory(void * p, size_t sizeBytes);
template <typename K>
// size_t HashKey(K key) { return HashMemory(&key, sizeof(key)); }
uint32_t HashKey(K key) { return SpookyHash::Hash32(&key, sizeof(key), 0); }

template <typename K, typename V>
class D0HashTable
{
public:
	std::vector<uint32_t> buckets;

	struct KN
	{
		K		key;
		uint32_t next;
	};

	std::vector<KN> keyAndNexts;
	std::vector<V> values;

	uint32_t nextFree;

	D0HashTable();
	
	void Insert(K key, V value);
	
	V * Lookup(K key);
	
	bool Remove(K key);
	
	void Reserve(uint32_t maxSize);
	
	void Reset();
	
	void Rehash(uint32_t bucketCountNew);
};

template <typename K, typename V>
class D1HashTable
{
public:
	enum State
	{
		EMPTY,
		FILLED,
		REMOVED,
	};

	struct KS
	{
		State state;
		K key;
	};

	std::vector<KS> keyAndStates;
	std::vector<V> values;
	uint32_t size_;

	D1HashTable();
	
	void Insert(K key, V value);
	
	V * Lookup(K key);
	
	bool Remove(K key);
	
	void Reserve(uint32_t maxSize);
	
	void Reset();
	
	void Rehash(uint32_t bucketCountNew);
};


// Hash table with separate chaining and no inline elements
template <typename K, typename V>
class C0HashTable
{
public:
	struct Elem
	{
		Elem *	pNext;
		size_t	hash;
		// Note: in a real implementation, instead of K and V this should just
		// be *storage* for K and V, to be constructed/destructed as needed
		K		key;
		V		value;
	};

	struct Bucket
	{
		Elem *	pHead;
	};

	std::vector<Bucket> buckets;
	std::vector<Elem>	elemPool;
	Elem *				pElemFreeHead;
	size_t				size;

	C0HashTable();

	void Insert(K key, V value);
	V * Lookup(K key);
	bool Remove(K key);

	void Reserve(size_t maxSize);
	void Reset();

	void Rehash(size_t bucketCountNew);
};

// Hash table with separate chaining and one inline element
template <typename K, typename V>
class C1HashTable
{
public:
	struct Elem
	{
		Elem *	pNext;
		size_t	hash;
		// Note: in a real implementation, instead of K and V this should just
		// be *storage* for K and V, to be constructed/destructed as needed
		K		key;
		V		value;
	};

	struct Bucket
	{
		Elem *	pHead;
		// Steal a bit from the hash value to say whether the bucket is filled
		size_t	hash:63;
		size_t	filled:1;
		// Note: in a real implementation, instead of K and V this should just
		// be *storage* for K and V, to be constructed/destructed as needed
		K		key;
		V		value;
	};

	std::vector<Bucket> buckets;
	std::vector<Elem>	elemPool;
	Elem *				pElemFreeHead;
	size_t				size;

	C1HashTable();

	void Insert(K key, V value);
	V * Lookup(K key);
	bool Remove(K key);

	void Reserve(size_t maxSize);
	void Reset();

	void Rehash(size_t bucketCountNew);
};

// Hash table with open addressing and linear probing
template <typename K, typename V>
class OLHashTable
{
public:
	enum BSTATE
	{
		BSTATE_Empty,
		BSTATE_Filled,
		BSTATE_Removed,
	};

	struct Bucket
	{
		// Steal a couple bits from the hash value to say whether the bucket
		// is empty, filled, or removed (different from empty)
		size_t	hash:62;
		size_t	state:2;
		// Note: in a real implementation, instead of K and V this should just
		// be *storage* for K and V, to be constructed/destructed as needed
		K		key;
		V		value;
	};

	std::vector<Bucket> buckets;
	size_t				size;

	OLHashTable();

	void Insert(K key, V value);
	V * Lookup(K key);
	bool Remove(K key);

	void Reserve(size_t maxSize);
	void Reset();

	void Rehash(size_t bucketCountNew);
};

// Hash table with open addressing and quadratic probing
template <typename K, typename V>
class OQHashTable
{
public:
	enum BSTATE
	{
		BSTATE_Empty,
		BSTATE_Filled,
		BSTATE_Removed,
	};

	struct Bucket
	{
		// Steal a couple bits from the hash value to say whether the bucket
		// is empty, filled, or removed (different from empty)
		size_t	hash:63;
		size_t	state:2;
		// Note: in a real implementation, instead of K and V this should just
		// be *storage* for K and V, to be constructed/destructed as needed
		K		key;
		V		value;
	};

	std::vector<Bucket> buckets;
	size_t				size;

	OQHashTable();

	void Insert(K key, V value);
	V * Lookup(K key);
	bool Remove(K key);

	void Reserve(size_t maxSize);
	void Reset();

	void Rehash(size_t bucketCountNew);
};

// "Data-oriented" hash table: open addressing, linear probing, but
// stores the hashes in a separate array from the keys & values
template <typename K, typename V>
class DO1HashTable
{
public:
	enum BSTATE
	{
		BSTATE_Empty,
		BSTATE_Filled,
		BSTATE_Removed,
	};

	struct Bucket
	{
		// Steal a couple bits from the hash value to say whether the bucket
		// is empty, filled, or removed (different from empty)
		size_t	hash:62;
		size_t	state:2;
	};

	struct KV
	{
		// Note: in a real implementation, instead of K and V this should just
		// be *storage* for K and V, to be constructed/destructed as needed
		K		key;
		V		value;
	};

	std::vector<Bucket>	buckets;
	std::vector<KV>		keyvals;
	size_t				size;

	DO1HashTable();

	void Insert(K key, V value);
	V * Lookup(K key);
	bool Remove(K key);

	void Reserve(size_t maxSize);
	void Reset();

	void Rehash(size_t bucketCountNew);
};

// "Data-oriented" hash table: open addressing, linear probing, but
// stores the hashes/keys/values all in separate arrays
template <typename K, typename V>
class DO2HashTable
{
public:
	enum BSTATE
	{
		BSTATE_Empty,
		BSTATE_Filled,
		BSTATE_Removed,
	};

	struct Bucket
	{
		// Steal a couple bits from the hash value to say whether the bucket
		// is empty, filled, or removed (different from empty)
		size_t	hash:62;
		size_t	state:2;
	};

	std::vector<Bucket>	buckets;
	// Note: in a real implementation, instead of K and V this should just
	// be *storage* for K and V, to be constructed/destructed as needed
	std::vector<K>		keys;
	std::vector<V>		values;
	size_t				size;

	DO2HashTable();

	void Insert(K key, V value);
	V * Lookup(K key);
	bool Remove(K key);

	void Reserve(size_t maxSize);
	void Reset();

	void Rehash(size_t bucketCountNew);
};

// Wrapper around unordered_map with the same interface as the others,
// and using the same hash function (instead of whatever std::hash is)
template <typename K, typename V>
class UMHashTable
{
public:
	struct Hasher
	{
		size_t operator() (K key) const
		{
			return HashKey(key);
		}
	};

	std::unordered_map<K, V, Hasher> map;

	void Insert(K key, V value)
	{
		map.insert(std::make_pair(key, value));
	}

	V * Lookup(K key)
	{
		auto it = map.find(key);
		if (it == map.end())
			return nullptr;
		return &it->second;
	}

	bool Remove(K key)
	{
		return (map.erase(key) > 0);
	}

	void Reserve(size_t bucketCountNew)
	{
		map.reserve(bucketCountNew);
	}

	void Reset()
	{
		map.clear();
	}
};

#include "hash-tables-impl.h"
