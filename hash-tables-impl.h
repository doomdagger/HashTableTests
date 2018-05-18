#pragma once

#include <algorithm>
#include <cassert>

// Master hash function: Bob Jenkins' SpookyHash
#include "SpookyHash/SpookyV2.h"
inline size_t HashMemory(void * p, size_t sizeBytes)
{
	return size_t(SpookyHash::Hash64(p, sizeBytes, 0));
}

static const int s_hashTableInitialSize = 16;

template <typename K, typename V>
D0HashTable<K, V>::D0HashTable()
{
	buckets.resize(16, static_cast<uint32_t>(-1));

	keyAndNexts.resize(16);
	values.resize(16);

	nextFree = 0;

	for (uint32_t idx = 0; idx < 15; ++idx)
	{
		keyAndNexts[idx].next = idx + 1;
	}

	keyAndNexts[15].next = static_cast<uint32_t>(-1);
}
template <typename K, typename V>
void D0HashTable<K, V>::Insert(K key, V value)
{
	if (nextFree == -1)
	{
		Rehash(static_cast<uint32_t>(buckets.size() * 2));
	}

	const auto index = nextFree;

	const auto hash = HashKey(key);

	auto& currentIndex = buckets[hash & (buckets.size() - 1)];
	// auto& currentIndex = buckets[hash % buckets.size()];

	auto& kn = keyAndNexts[index];

	nextFree = kn.next;

	kn.key = key;
	kn.next = currentIndex;

	currentIndex = index;


	values[index] = value;
}

template <typename K, typename V>
V* D0HashTable<K, V>::Lookup(K key)
{
	const auto hash = HashKey(key);

	auto index = buckets[hash & (buckets.size() - 1)];
	// auto index = buckets[hash % buckets.size()];

	while (index != -1)
	{
		auto& kn = keyAndNexts[index];

		if (kn.key == key)
		{
			return &values[index];
		}

		index = kn.next;
	};

	return nullptr;
}

template <typename K, typename V>
bool D0HashTable<K, V>::Remove(K key)
{
	const auto hash = HashKey(key);

	auto hashIndex = hash & (buckets.size() - 1);
	// auto hashIndex = hash % buckets.size();

	auto index = buckets[hashIndex];

	if (index == -1)
	{
		return false;
	}

	auto* prevKn = &keyAndNexts[index];

	{
		if (prevKn->key == key)
		{
			buckets[hashIndex] = prevKn->next;

			prevKn->next = nextFree;
			nextFree = index;

			return true;
		}
		else
		{
			index = prevKn->next;
		}
	}

	while (index != -1)
	{
		auto& kn = keyAndNexts[index];

		if (kn.key == key)
		{
			prevKn->next = kn.next;

			kn.next = nextFree;
			nextFree = index;

			return true;
		}

		index = kn.next;
		prevKn = &kn;
	};

	return false;
}

template <typename K, typename V>
void D0HashTable<K, V>::Reserve(uint32_t maxSize)
{
    maxSize |= maxSize >> 1;
    maxSize |= maxSize >> 2;
    maxSize |= maxSize >> 4;
    maxSize |= maxSize >> 8;
    maxSize |= maxSize >> 16;

	Rehash(maxSize);
}

template <typename K, typename V>
void D0HashTable<K, V>::Reset()
{
	buckets.clear();
	keyAndNexts.clear();
	values.clear();

	buckets.resize(16);

	keyAndNexts.resize(16);
	values.resize(16);

	nextFree = 0;

	for (uint32_t idx = 0; idx < 15; ++idx)
	{
		keyAndNexts[idx].next = idx + 1;
	}

	keyAndNexts[15].next = -1;
}

template <typename K, typename V>
void D0HashTable<K, V>::Rehash(uint32_t bucketCountNew)
{
	uint32_t size = static_cast<uint32_t>(buckets.size());

	if (bucketCountNew <= size)
	{
		return;
	}

	keyAndNexts.resize(bucketCountNew);
	values.resize(bucketCountNew);

	std::vector<uint32_t> bucketsNew(bucketCountNew, static_cast<uint32_t>(-1));

	auto newSize = bucketsNew.size();

	for (uint32_t idx = 0; idx < size; ++idx)
	{
		auto index = buckets[idx];

		while (index != -1)
		{
			auto& kn = keyAndNexts[index];

			const auto hash = HashKey(kn.key);

			auto newHashIndex = hash & (newSize - 1);
			// auto newHashIndex = hash % newSize;

			auto& newIndex = bucketsNew[newHashIndex];

			if (newIndex == -1)
			{
				newIndex = index;

				index = kn.next;
				kn.next = static_cast<uint32_t>(-1);
			}
			else
			{
				auto currentNewIndex = newIndex;

				newIndex = index;
				index = kn.next;
				kn.next = currentNewIndex;
			}
		}
	}

	{
		for (uint32_t idx = size; idx < bucketCountNew; ++idx)
		{
			keyAndNexts[idx].next = idx + 1;
		}

		keyAndNexts[bucketCountNew - 1].next = nextFree;
		nextFree = size;
	}

	buckets.swap(bucketsNew);
}

// D1HashTable open address
template <typename K, typename V>
D1HashTable<K, V>::D1HashTable()
{
	keyAndStates.resize(16);
	values.resize(16);
	size_ = 0;
}

template <typename K, typename V>
void D1HashTable<K, V>::Insert(K key, V value)
{
	if (size_ * 3 > keyAndStates.size() * 2)
	{
		Rehash(static_cast<uint32_t>(keyAndStates.size() * 2));
	}

	const auto hash = HashKey(key);

	const uint32_t keyEnd = static_cast<uint32_t>(keyAndStates.size());
	const uint32_t keyStart = hash & (keyEnd - 1);

	++size_;

	for (uint32_t idx = keyStart; idx < keyEnd; ++idx)
	{
		auto& ks = keyAndStates[idx];

		if (ks.state != FILLED)
		{
			ks.state = FILLED;
			ks.key = key;
			values[idx] = value;

			return;
		}
	}

	for (uint32_t idx = 0; idx < keyStart; ++idx)
	{
		auto& ks = keyAndStates[idx];

		if (ks.state != FILLED)
		{
			ks.state = FILLED;
			ks.key = key;
			values[idx] = value;

			return;
		}
	}
}

template <typename K, typename V>
V* D1HashTable<K, V>::Lookup(K key)
{
	const auto hash = HashKey(key);

	const uint32_t keyEnd = static_cast<uint32_t>(keyAndStates.size());
	const uint32_t keyStart = hash & (keyEnd - 1);

	for (uint32_t idx = keyStart; idx < keyEnd; ++idx)
	{
		auto& ks = keyAndStates[idx];

		switch (ks.state)
		{
		case EMPTY:
			return nullptr;
		case FILLED:
			if (ks.state == FILLED && ks.key == key)
			{
				return &values[idx];
			}
			break;
		default:
			break;
		}
	}

	for (uint32_t idx = 0; idx < keyStart; ++idx)
	{
		auto& ks = keyAndStates[idx];

		switch (ks.state)
		{
		case EMPTY:
			return nullptr;
		case FILLED:
			if (ks.state == FILLED && ks.key == key)
			{
				return &values[idx];
			}
			break;
		default:
			break;
		}
	}

	return nullptr;
}

template <typename K, typename V>
bool D1HashTable<K, V>::Remove(K key)
{
	const auto hash = HashKey(key);

	const uint32_t keyEnd = static_cast<uint32_t>(keyAndStates.size());
	const uint32_t keyStart = hash & (keyEnd - 1);

	for (uint32_t idx = keyStart; idx < keyEnd; ++idx)
	{
		auto& ks = keyAndStates[idx];

		switch (ks.state)
		{
		case EMPTY:
			return false;
		case FILLED:
			if (ks.state == FILLED && ks.key == key)
			{
				ks.state = REMOVED;
				--size_;
				return true;
			}
			break;
		default:
			break;
		}
	}

	for (uint32_t idx = 0; idx < keyStart; ++idx)
	{
		auto& ks = keyAndStates[idx];

		switch (ks.state)
		{
		case EMPTY:
			return false;
		case FILLED:
			if (ks.state == FILLED && ks.key == key)
			{
				ks.state = REMOVED;
				--size_;
				return true;
			}
			break;
		default:
			break;
		}
	}

	return false;
}

template <typename K, typename V>
void D1HashTable<K, V>::Reserve(uint32_t maxSize)
{
	maxSize = maxSize * 3 / 2;

    maxSize |= maxSize >> 1;
    maxSize |= maxSize >> 2;
    maxSize |= maxSize >> 4;
    maxSize |= maxSize >> 8;
    maxSize |= maxSize >> 16;

	Rehash(maxSize);
}

template <typename K, typename V>
void D1HashTable<K, V>::Reset()
{
	keyAndStates.clear();
	values.clear();

	keyAndStates.resize(16);
	buckets.resize(16);

	size_ = 0;
}

template <typename K, typename V>
void D1HashTable<K, V>::Rehash(uint32_t bucketCountNew)
{
    const uint32_t oldSize = static_cast<uint32_t>(keyAndStates.size());

	if (bucketCountNew <= oldSize)
	{
		return;
	}

	std::vector<KS> newKeyAndStates(bucketCountNew);
	std::vector<V> newValues(bucketCountNew);

    const uint32_t keyEnd = static_cast<uint32_t>(bucketCountNew);
    const uint32_t keyEndS1 = keyEnd - 1;
	

	for (uint32_t i = 0; i < oldSize; ++i)
	{
		const auto&	ks = keyAndStates[i];

		if (ks.state != FILLED)
		{
			continue;
		}

		const auto hash = HashKey(ks.key);

        uint32_t keyStart = hash & keyEndS1;

		for (uint32_t idx = keyStart; idx < keyEnd; ++idx)
		{
			auto& newKs = newKeyAndStates[idx];

			if (newKs.state != FILLED)
			{
                newKs.state = FILLED;
                newKs.key = ks.key;
				newValues[idx] = values[i];

				goto L1;
			}
		}

		for (uint32_t idx = 0; idx < keyStart; ++idx)
		{
			auto& newKs = newKeyAndStates[idx];

			if (newKs.state != FILLED)
			{
                newKs.state = FILLED;
                newKs.key = ks.key;
				newValues[idx] = values[i];

				goto L1;
			}
		}

		L1:;
	}

	keyAndStates.swap(newKeyAndStates);
	values.swap(newValues);
}


// C0HashTable implementation

template <typename K, typename V>
C0HashTable<K, V>::C0HashTable()
:	size(0)
{
	// Start off with a small initial size
	buckets.resize(s_hashTableInitialSize);
	elemPool.resize(s_hashTableInitialSize);

	// Build the initial free list
	pElemFreeHead = &elemPool[0];
	for (int i = 0; i < s_hashTableInitialSize - 1; ++i)
		elemPool[i].pNext = &elemPool[i+1];
}

template <typename K, typename V>
void C0HashTable<K, V>::Insert(K key, V value)
{
	// Resize larger if we're out of elements
	if (!pElemFreeHead)
	{
		Rehash(buckets.size() * 2);
	}

	// Grab the next element off the free list
	assert(pElemFreeHead);
	Elem * e = pElemFreeHead;
	pElemFreeHead = e->pNext;

	// Hash the key and look up the appropriate bucket
	const auto hash = HashKey(key);
	// Bucket * b = &buckets[hash % buckets.size()];
	Bucket * b = &buckets[hash & (buckets.size() - 1)];

	// Insert the new element into the bucket
	e->pNext = b->pHead;
	b->pHead = e;

	// Store the hash, key, and value in the element
	e->hash = hash;
	e->key = key;
	e->value = value;

	++size;
}

template <typename K, typename V>
V * C0HashTable<K, V>::Lookup(K key)
{
	// Hash the key and look up the appropriate bucket
	const auto hash = HashKey(key);
	// Bucket * b = &buckets[hash % buckets.size()];
	Bucket * b = &buckets[hash & (buckets.size() - 1)];

	// Walk the chain looking for a matching element
	for (Elem * e = b->pHead; e; e = e->pNext)
	{
		if (e->hash == hash && e->key == key)
			return &e->value;
	}

	return nullptr;
}

template <typename K, typename V>
bool C0HashTable<K, V>::Remove(K key)
{
	// Hash the key and look up the appropriate bucket
	const auto hash = HashKey(key);
	// Bucket * b = &buckets[hash % buckets.size()];
	Bucket * b = &buckets[hash & (buckets.size() - 1)];

	Elem * pHead = b->pHead;
	if (!pHead)
		return false;

	Elem * eRemoved = nullptr;

	// Check the first element
	if (pHead->hash == hash && pHead->key == key)
	{
		eRemoved = pHead;
		b->pHead = pHead->pNext;
	}
	else
	{
		// Walk the chain looking for a matching element
		for (Elem * e = pHead->pNext, * ePrev = pHead;
			 e;
			 ePrev = e, e = e->pNext)
		{
			if (e->hash == hash && e->key == key)
			{
				eRemoved = e;
				ePrev->pNext = e->pNext;
				break;
			}
		}
	}

	if (eRemoved)
	{
		// Put eRemoved back on the free list
		eRemoved->hash = 0;
		eRemoved->pNext = pElemFreeHead;
		pElemFreeHead = eRemoved;
		--size;
	}

	return (eRemoved != nullptr);
}

template <typename K, typename V>
void C0HashTable<K, V>::Reserve(size_t maxSize)
{
    maxSize |= maxSize >> 1;
    maxSize |= maxSize >> 2;
    maxSize |= maxSize >> 4;
    maxSize |= maxSize >> 8;
    maxSize |= maxSize >> 16;

	Rehash(maxSize);
}

template <typename K, typename V>
void C0HashTable<K, V>::Rehash(size_t bucketCountNew)
{
	// Can't rehash down to smaller than current size or initial size
	bucketCountNew = std::max(std::max(bucketCountNew, size),
						   size_t(s_hashTableInitialSize));

	// Build a new set of buckets and elements
	std::vector<Bucket> bucketsNew(bucketCountNew);
	std::vector<Elem> elemPoolNew(bucketCountNew);

	// Walk through all the current elements, move them into the new
	// element pool and insert them into the new buckets
	size_t iElemNext = 0;
	for (size_t i = 0, iEnd = buckets.size(); i < iEnd; ++i)
	{
		Bucket * b = &buckets[i];
		for (Elem * e = b->pHead; e; e = e->pNext)
		{
			Elem * eNew = &elemPoolNew[iElemNext];
			++iElemNext;

			const auto hash = e->hash;
			// Bucket * bNew = &bucketsNew[hash % bucketCountNew];
			Bucket * bNew = &bucketsNew[hash & (bucketCountNew - 1)];

			// Insert the new element into the bucket
			eNew->pNext = bNew->pHead;
			bNew->pHead = eNew;

			// Store the hash, key, and value in the element
			eNew->hash = hash;
			eNew->key = std::move(e->key);
			eNew->value = std::move(e->value);
		}
	}

	// Swap the new buckets and elements into place
	buckets.swap(bucketsNew);
	elemPool.swap(elemPoolNew);

	// Build a free list of the remaining elements in the pool
	pElemFreeHead = (iElemNext < elemPool.size()) ? &elemPool[iElemNext] : nullptr;
	for (size_t i = iElemNext, iEnd = elemPool.size() - 1; i < iEnd; ++i)
		elemPool[i].pNext = &elemPool[i+1];
}

template <typename K, typename V>
void C0HashTable<K, V>::Reset()
{
	// Blow away the current table and reset to small initial size
	buckets.clear();
	buckets.resize(s_hashTableInitialSize);
	elemPool.clear();
	elemPool.resize(s_hashTableInitialSize);

	// Build the initial free list
	pElemFreeHead = &elemPool[0];
	for (int i = 0; i < s_hashTableInitialSize - 1; ++i)
		elemPool[i].pNext = &elemPool[i+1];

	size = 0;
}



// C1HashTable implementation

static const size_t s_63Bits = 0x7fffffffffffffffULL;

template <typename K, typename V>
C1HashTable<K, V>::C1HashTable()
:	size(0)
{
	// Start off with a small initial size.  Since we have space for an
	// element in the bucket itself, start with only half as many elements
	// in the element pool.
	buckets.resize(s_hashTableInitialSize);
	elemPool.resize(s_hashTableInitialSize / 2);

	// Build the initial free list
	pElemFreeHead = &elemPool[0];
	for (size_t i = 0, iEnd = elemPool.size() - 1; i < iEnd; ++i)
		elemPool[i].pNext = &elemPool[i+1];
}

template <typename K, typename V>
void C1HashTable<K, V>::Insert(K key, V value)
{
	// Hash the key and look up the appropriate bucket
	const auto hash = HashKey(key) & s_63Bits;
	// Bucket * b = &buckets[hash % buckets.size()];
	Bucket * b = &buckets[hash & (buckets.size() - 1)];

	// Is the bucket empty?
	if (!b->filled)
	{
		// Store it in the bucket. Done.
		b->filled = true;
		b->hash = hash;
		b->key = key;
		b->value = value;
		++size;
		return;
	}

	// Resize larger if we're out of elements
	if (!pElemFreeHead)
	{
		Rehash(buckets.size() * 2);

		// Need to re-lookup the bucket since we resized
		// b = &buckets[hash % buckets.size()];
		b = &buckets[hash & (buckets.size() - 1)];

		// Is the bucket empty?
		if (!b->filled)
		{
			// Store it in the bucket. Done.
			b->filled = true;
			b->hash = hash;
			b->key = key;
			b->value = value;
			++size;
			return;
		}
	}

	// Grab the next element off the free list
	assert(pElemFreeHead);
	Elem * e = pElemFreeHead;
	pElemFreeHead = e->pNext;

	// Insert the new element into the bucket
	e->pNext = b->pHead;
	b->pHead = e;

	// Store the hash, key, and value in the element
	e->hash = hash;
	e->key = key;
	e->value = value;

	++size;
}

template <typename K, typename V>
V * C1HashTable<K, V>::Lookup(K key)
{
	// Hash the key and look up the appropriate bucket
	const auto hash = HashKey(key) & s_63Bits;
	// Bucket * b = &buckets[hash % buckets.size()];
	Bucket * b = &buckets[hash & (buckets.size() - 1)];

	if (!b->filled)
		return nullptr;

	// Check if it's in the bucket itself
	if (b->hash == hash && b->key == key)
	{
		return &b->value;
	}

	// Walk the chain looking for a matching element
	for (Elem * e = b->pHead; e; e = e->pNext)
	{
		if (e->hash == hash && e->key == key)
			return &e->value;
	}

	return nullptr;
}

template <typename K, typename V>
bool C1HashTable<K, V>::Remove(K key)
{
	// Hash the key and look up the appropriate bucket
	const auto hash = HashKey(key) & s_63Bits;
	// Bucket * b = &buckets[hash % buckets.size()];
	Bucket * b = &buckets[hash & (buckets.size() - 1)];

	if (!b->filled)
		return nullptr;

	// Check if it's in the bucket itself
	if (b->hash == hash && b->key == key)
	{
		// If the bucket has a chain, move the first element of the chain
		// into the bucket
		if (Elem * pHead = b->pHead)
		{
			b->pHead = pHead->pNext;
			b->hash = pHead->hash;
			b->key = std::move(pHead->key);
			b->value = std::move(pHead->value);

			// Put the removed element back on the free list
			pHead->hash = 0;
			pHead->pNext = pElemFreeHead;
			pElemFreeHead = pHead;
		}
		else
		{
			// The bucket is now empty
			b->hash = 0;
			b->filled = false;
		}

		--size;
		return true;
	}

	Elem * pHead = b->pHead;
	if (!pHead)
		return false;

	Elem * eRemoved = nullptr;

	// Check the first element
	if (pHead->hash == hash && pHead->key == key)
	{
		eRemoved = pHead;
		b->pHead = pHead->pNext;
	}
	else
	{
		// Walk the chain looking for a matching element
		for (Elem * e = pHead->pNext, * ePrev = pHead;
			 e;
			 ePrev = e, e = e->pNext)
		{
			if (e->hash == hash && e->key == key)
			{
				eRemoved = e;
				ePrev->pNext = e->pNext;
				break;
			}
		}
	}

	if (eRemoved)
	{
		// Put eRemoved back on the free list
		eRemoved->hash = 0;
		eRemoved->pNext = pElemFreeHead;
		pElemFreeHead = eRemoved;
		--size;
	}

	return (eRemoved != nullptr);
}

template <typename K, typename V>
void C1HashTable<K, V>::Reserve(size_t maxSize)
{
    maxSize |= maxSize >> 1;
    maxSize |= maxSize >> 2;
    maxSize |= maxSize >> 4;
    maxSize |= maxSize >> 8;
    maxSize |= maxSize >> 16;

	Rehash(maxSize);
}

template <typename K, typename V>
void C1HashTable<K, V>::Rehash(size_t bucketCountNew)
{
	// Can't rehash down to smaller than current size or initial size
	bucketCountNew = std::max(std::max(bucketCountNew, size),
						   size_t(s_hashTableInitialSize));

	// Build a new set of buckets and elements
	std::vector<Bucket> bucketsNew(bucketCountNew);
	std::vector<Elem> elemPoolNew(bucketCountNew / 2);

	// Walk through all the current elements, move them into the new
	// element pool and insert them into the new buckets
	size_t iElemNext = 0;
	for (size_t i = 0, iEnd = buckets.size(); i < iEnd; ++i)
	{
		Bucket * b = &buckets[i];

		if (!b->filled)
			continue;

		// Handle the element in the bucket
		const auto hash = b->hash;
		// Bucket * bNew = &bucketsNew[hash % bucketCountNew];
		Bucket * bNew = &bucketsNew[hash & (bucketCountNew - 1)];
		if (!bNew->filled)
		{
			// Store it in the bucket. Done.
			bNew->filled = true;
			bNew->hash = hash;
			// Note: can't move from old key and value because we might
			// have to use the escape hatch (rehash even bigger) later.
			bNew->key = b->key;
			bNew->value = b->value;
		}
		else
		{
			if (iElemNext == elemPoolNew.size())
			{
				// Escape hatch: rehash even bigger!
				bucketsNew.clear();
				elemPoolNew.clear();
				Rehash(bucketCountNew * 2);
				return;
			}
			Elem * eNew = &elemPoolNew[iElemNext];
			++iElemNext;

			// Insert the new element into the bucket
			eNew->pNext = bNew->pHead;
			bNew->pHead = eNew;

			// Store the hash, key, and value in the element
			eNew->hash = hash;
			// Note: can't move from old key and value because we might
			// have to use the escape hatch (rehash even bigger) later.
			eNew->key = b->key;
			eNew->value = b->value;
		}

		// Handle the elements in the chain
		for (Elem * e = b->pHead; e; e = e->pNext)
		{
			const auto hash = e->hash;
			// Bucket * bNew = &bucketsNew[hash % bucketCountNew];
			Bucket * bNew = &bucketsNew[hash & (bucketCountNew - 1)];

			if (!bNew->filled)
			{
				// Store it in the bucket. Done.
				bNew->filled = true;
				bNew->hash = hash;
				// Note: can't move from old key and value because we might
				// have to use the escape hatch (rehash even bigger) later.
				bNew->key = e->key;
				bNew->value = e->value;
			}
			else
			{
				if (iElemNext == elemPoolNew.size())
				{
					// Escape hatch: rehash even bigger!
					bucketsNew.clear();
					elemPoolNew.clear();
					Rehash(bucketCountNew * 2);
					return;
				}
				Elem * eNew = &elemPoolNew[iElemNext];
				++iElemNext;

				// Insert the new element into the bucket
				eNew->pNext = bNew->pHead;
				bNew->pHead = eNew;

				// Store the hash, key, and value in the element
				eNew->hash = hash;
				// Note: can't move from old key and value because we might
				// have to use the escape hatch (rehash even bigger) later.
				eNew->key = e->key;
				eNew->value = e->value;
			}
		}
	}

	// Swap the new buckets and elements into place
	buckets.swap(bucketsNew);
	elemPool.swap(elemPoolNew);

	// Build a free list of the remaining elements in the pool
	pElemFreeHead = (iElemNext < elemPool.size()) ? &elemPool[iElemNext] : nullptr;
	for (size_t i = iElemNext, iEnd = elemPool.size() - 1; i < iEnd; ++i)
		elemPool[i].pNext = &elemPool[i+1];
}

template <typename K, typename V>
void C1HashTable<K, V>::Reset()
{
	// Blow away the current table and reset to small initial size
	buckets.clear();
	buckets.resize(s_hashTableInitialSize);
	elemPool.clear();
	elemPool.resize(s_hashTableInitialSize / 2);

	// Build the initial free list
	pElemFreeHead = &elemPool[0];
	for (size_t i = 0, iEnd = elemPool.size() - 1; i < iEnd; ++i)
		elemPool[i].pNext = &elemPool[i+1];

	size = 0;
}



// OLHashTable implementation

static const size_t s_62Bits = 0x3fffffffffffffffULL;

template <typename K, typename V>
OLHashTable<K, V>::OLHashTable()
:	size(0)
{
	// Start off with a small initial size
	buckets.resize(s_hashTableInitialSize);
}

template <typename K, typename V>
void OLHashTable<K, V>::Insert(K key, V value)
{
	// Resize larger if the load factor goes over 2/3
	if (size * 3 > buckets.size() * 2)
	{
		Rehash(buckets.size() * 2);
	}

	// Hash the key and find the starting bucket
	const auto hash = HashKey(key) & s_62Bits;
	// size_t iBucketStart = hash % buckets.size();
	size_t iBucketStart = hash & (buckets.size() - 1);

	// Search for an unused bucket
	Bucket * bTarget = nullptr;
	for (size_t i = iBucketStart, iEnd = buckets.size(); i < iEnd; ++i) 
	{
		Bucket * b = &buckets[i];
		if (b->state != BSTATE_Filled)
		{
			bTarget = b;
			break;
		}
	}
	if (!bTarget)
	{
		for (size_t i = 0; i < iBucketStart; ++i)
		{
			Bucket * b = &buckets[i];
			if (b->state != BSTATE_Filled)
			{
				bTarget = b;
				break;
			}
		}
	}

	assert(bTarget);

	// Store the hash, key, and value in the bucket
	bTarget->hash = hash;
	bTarget->state = BSTATE_Filled;
	bTarget->key = key;
	bTarget->value = value;

	++size;
}

template <typename K, typename V>
V * OLHashTable<K, V>::Lookup(K key)
{
	// Hash the key and find the starting bucket
	const auto hash = HashKey(key) & s_62Bits;
	// size_t iBucketStart = hash % buckets.size();
	size_t iBucketStart = hash & (buckets.size() - 1);

	// Search the buckets until we hit an empty one
	for (size_t i = iBucketStart, iEnd = buckets.size(); i < iEnd; ++i)
	{
		Bucket * b = &buckets[i];
		switch (b->state)
		{
		case BSTATE_Empty:
			return nullptr;
		case BSTATE_Filled:
			if (b->hash == hash && b->key == key)
				return &b->value;
			break;
		default:
			break;
		}
	}
	for (size_t i = 0; i < iBucketStart; ++i)
	{
		Bucket * b = &buckets[i];
		switch (b->state)
		{
		case BSTATE_Empty:
			return nullptr;
		case BSTATE_Filled:
			if (b->hash == hash && b->key == key)
				return &b->value;
			break;
		default:
			break;
		}
	}

	return nullptr;
}

template <typename K, typename V>
bool OLHashTable<K, V>::Remove(K key)
{
	// Hash the key and find the starting bucket
	const auto hash = HashKey(key) & s_62Bits;
	// size_t iBucketStart = hash % buckets.size();
	size_t iBucketStart = hash & (buckets.size() - 1);

	// Search the buckets until we hit an empty one
	for (size_t i = iBucketStart, iEnd = buckets.size(); i < iEnd; ++i)
	{
		Bucket * b = &buckets[i];
		switch (b->state)
		{
		case BSTATE_Empty:
			return false;
		case BSTATE_Filled:
			if (b->hash == hash && b->key == key)
			{
				b->hash = 0;
				b->state = BSTATE_Removed;
				--size;
				return true;
			}
			break;
		default:
			break;
		}
	}
	for (size_t i = 0; i < iBucketStart; ++i)
	{
		Bucket * b = &buckets[i];
		switch (b->state)
		{
		case BSTATE_Empty:
			return false;
		case BSTATE_Filled:
			if (b->hash == hash && b->key == key)
			{
				b->hash = 0;
				b->state = BSTATE_Removed;
				--size;
				return true;
			}
			break;
		default:
			break;
		}
	}

	return false;
}

template <typename K, typename V>
void OLHashTable<K, V>::Reserve(size_t maxSize)
{
	maxSize = maxSize * 3 / 2;

    maxSize |= maxSize >> 1;
    maxSize |= maxSize >> 2;
    maxSize |= maxSize >> 4;
    maxSize |= maxSize >> 8;
    maxSize |= maxSize >> 16;

	Rehash(maxSize);
}

template <typename K, typename V>
void OLHashTable<K, V>::Rehash(size_t bucketCountNew)
{
	// Can't rehash down to smaller than current size or initial size
	bucketCountNew = std::max(std::max(bucketCountNew, size),
						   size_t(s_hashTableInitialSize));

	// Build a new set of buckets
	std::vector<Bucket> bucketsNew(bucketCountNew);

	// Walk through all the current elements and insert them into the new buckets
	for (size_t i = 0, iEnd = buckets.size(); i < iEnd; ++i)
	{
		Bucket * b = &buckets[i];
		if (b->state != BSTATE_Filled)
			continue;

		// Hash the key and find the starting bucket
		const auto hash = b->hash;
		// size_t iBucketStart = hash % bucketCountNew;
		size_t iBucketStart = hash & (bucketCountNew - 1);

		// Search for an unused bucket
		Bucket * bTarget = nullptr;
		for (size_t j = iBucketStart; j < bucketCountNew; ++j) 
		{
			Bucket * bNew = &bucketsNew[j];
			if (bNew->state != BSTATE_Filled)
			{
				bTarget = bNew;
				break;
			}
		}
		if (!bTarget)
		{
			for (size_t j = 0; j < iBucketStart; ++j)
			{
				Bucket * bNew = &bucketsNew[j];
				if (bNew->state != BSTATE_Filled)
				{
					bTarget = bNew;
					break;
				}
			}
		}

		assert(bTarget);

		// Store the hash, key, and value in the bucket
		bTarget->hash = hash;
		bTarget->state = BSTATE_Filled;
		bTarget->key = std::move(b->key);
		bTarget->value = std::move(b->value);
	}

	// Swap the new buckets into place
	buckets.swap(bucketsNew);
}

template <typename K, typename V>
void OLHashTable<K, V>::Reset()
{
	// Blow away the current table and reset to small initial size
	buckets.clear();
	buckets.resize(s_hashTableInitialSize);

	size = 0;
}



// OQHashTable implementation

template <typename K, typename V>
OQHashTable<K, V>::OQHashTable()
:	size(0)
{
	// Start off with a small initial size
	buckets.resize(s_hashTableInitialSize);
}

template <typename K, typename V>
void OQHashTable<K, V>::Insert(K key, V value)
{
	// Resize larger if the load factor goes over 2/3
	if (size * 3 > buckets.size() * 2)
	{
		Rehash(buckets.size() * 2);
	}

	// Hash the key and find the starting bucket
	const auto hash = HashKey(key) & s_62Bits;
	// size_t iBucketStart = hash % buckets.size();
	size_t iBucketStart = hash & (buckets.size() - 1);

	// Search for an unused bucket
	Bucket * bTarget = nullptr;
	for (size_t i = 0, iEnd = buckets.size(); i < iEnd; ++i)
	{
		// Evaluate the probing sequence.  This one is guaranteed to hit all
		// elements if the table is power-of-two-sized; see:
		//   http://en.wikipedia.org/wiki/Quadratic_probing
		size_t probe = (iBucketStart + (i + i*i) / 2) % iEnd;

		Bucket * b = &buckets[probe];
		if (b->state != BSTATE_Filled)
		{
			bTarget = b;
			break;
		}
	}

	assert(bTarget);

	// Store the hash, key, and value in the bucket
	bTarget->hash = hash;
	bTarget->state = BSTATE_Filled;
	bTarget->key = key;
	bTarget->value = value;

	++size;
}

template <typename K, typename V>
V * OQHashTable<K, V>::Lookup(K key)
{
	// Hash the key and find the starting bucket
	const auto hash = HashKey(key) & s_62Bits;
	// size_t iBucketStart = hash % buckets.size();
	size_t iBucketStart = hash & (buckets.size() - 1);

	// Search the buckets until we hit an empty one
	for (size_t i = 0, iEnd = buckets.size(); i < iEnd; ++i)
	{
		// Evaluate the probing sequence
		size_t probe = (iBucketStart + (i + i*i) / 2) % iEnd;

		Bucket * b = &buckets[probe];
		switch (b->state)
		{
		case BSTATE_Empty:
			return nullptr;
		case BSTATE_Filled:
			if (b->hash == hash && b->key == key)
				return &b->value;
			break;
		default:
			break;
		}
	}

	return nullptr;
}

template <typename K, typename V>
bool OQHashTable<K, V>::Remove(K key)
{
	// Hash the key and find the starting bucket
	const auto hash = HashKey(key) & s_62Bits;
	// size_t iBucketStart = hash % buckets.size();
	size_t iBucketStart = hash & (buckets.size() - 1);

	// Search the buckets until we hit an empty one
	for (size_t i = 0, iEnd = buckets.size(); i < iEnd; ++i)
	{
		// Evaluate the probing sequence
		size_t probe = (iBucketStart + (i + i*i) / 2) % iEnd;

		Bucket * b = &buckets[probe];
		switch (b->state)
		{
		case BSTATE_Empty:
			return false;
		case BSTATE_Filled:
			if (b->hash == hash && b->key == key)
			{
				b->hash = 0;
				b->state = BSTATE_Removed;
				--size;
				return true;
			}
			break;
		default:
			break;
		}
	}

	return false;
}

template <typename K, typename V>
void OQHashTable<K, V>::Reserve(size_t maxSize)
{
	maxSize = maxSize * 3 / 2;
    maxSize |= maxSize >> 1;
    maxSize |= maxSize >> 2;
    maxSize |= maxSize >> 4;
    maxSize |= maxSize >> 8;
    maxSize |= maxSize >> 16;

	Rehash(maxSize);
}

template <typename K, typename V>
void OQHashTable<K, V>::Rehash(size_t bucketCountNew)
{
	// Can't rehash down to smaller than current size or initial size
	bucketCountNew = std::max(std::max(bucketCountNew, size),
						   size_t(s_hashTableInitialSize));

	// Build a new set of buckets
	std::vector<Bucket> bucketsNew(bucketCountNew);

	// Walk through all the current elements and insert them into the new buckets
	for (size_t i = 0, iEnd = buckets.size(); i < iEnd; ++i)
	{
		Bucket * b = &buckets[i];
		if (b->state != BSTATE_Filled)
			continue;

		// Hash the key and find the starting bucket
		const auto hash = b->hash;
		// size_t iBucketStart = hash % bucketCountNew;
		size_t iBucketStart = hash & (bucketCountNew - 1);

		// Search for an unused bucket
		Bucket * bTarget = nullptr;
		for (size_t j = 0; j < bucketCountNew; ++j) 
		{
			// Evaluate the probing sequence
			size_t probe = (iBucketStart + (j + j*j) / 2) % bucketCountNew;

			Bucket * bNew = &bucketsNew[probe];
			if (bNew->state != BSTATE_Filled)
			{
				bTarget = bNew;
				break;
			}
		}

		assert(bTarget);

		// Store the hash, key, and value in the bucket
		bTarget->hash = hash;
		bTarget->state = BSTATE_Filled;
		bTarget->key = std::move(b->key);
		bTarget->value = std::move(b->value);
	}

	// Swap the new buckets into place
	buckets.swap(bucketsNew);
}

template <typename K, typename V>
void OQHashTable<K, V>::Reset()
{
	// Blow away the current table and reset to small initial size
	buckets.clear();
	buckets.resize(s_hashTableInitialSize);

	size = 0;
}



// DO1HashTable implementation

template <typename K, typename V>
DO1HashTable<K, V>::DO1HashTable()
:	size(0)
{
	// Start off with a small initial size
	buckets.resize(s_hashTableInitialSize);
	keyvals.resize(s_hashTableInitialSize);
}

template <typename K, typename V>
void DO1HashTable<K, V>::Insert(K key, V value)
{
	// Resize larger if the load factor goes over 2/3
	if (size * 3 > buckets.size() * 2)
	{
		Rehash(buckets.size() * 2);
	}

	// Hash the key and find the starting bucket
	const auto hash = HashKey(key) & s_62Bits;
	// size_t iBucketStart = hash % buckets.size();
	size_t iBucketStart = hash & (buckets.size() - 1);

	// Search for an unused bucket
	Bucket * bTarget = nullptr;
	KV * kvTarget = nullptr;
	for (size_t i = iBucketStart, iEnd = buckets.size(); i < iEnd; ++i) 
	{
		Bucket * b = &buckets[i];
		if (b->state != BSTATE_Filled)
		{
			bTarget = b;
			kvTarget = &keyvals[i];
			break;
		}
	}
	if (!bTarget)
	{
		for (size_t i = 0; i < iBucketStart; ++i)
		{
			Bucket * b = &buckets[i];
			if (b->state != BSTATE_Filled)
			{
				bTarget = b;
				kvTarget = &keyvals[i];
				break;
			}
		}
	}

	assert(bTarget);
	assert(kvTarget);

	// Store the hash, key, and value in the bucket
	bTarget->hash = hash;
	bTarget->state = BSTATE_Filled;
	kvTarget->key = key;
	kvTarget->value = value;

	++size;
}

template <typename K, typename V>
V * DO1HashTable<K, V>::Lookup(K key)
{
	// Hash the key and find the starting bucket
	const auto hash = HashKey(key) & s_62Bits;
	// size_t iBucketStart = hash % buckets.size();
	size_t iBucketStart = hash & (buckets.size() - 1);

	// Search the buckets until we hit an empty one
	for (size_t i = iBucketStart, iEnd = buckets.size(); i < iEnd; ++i)
	{
		Bucket * b = &buckets[i];
		switch (b->state)
		{
		case BSTATE_Empty:
			return nullptr;
		case BSTATE_Filled:
			if (b->hash == hash)
			{
				KV * kv = &keyvals[i];
				if (kv->key == key)
					return &kv->value;
			}
			break;
		default:
			break;
		}
	}
	for (size_t i = 0; i < iBucketStart; ++i)
	{
		Bucket * b = &buckets[i];
		switch (b->state)
		{
		case BSTATE_Empty:
			return nullptr;
		case BSTATE_Filled:
			if (b->hash == hash)
			{
				KV * kv = &keyvals[i];
				if (kv->key == key)
					return &kv->value;
			}
			break;
		default:
			break;
		}
	}

	return nullptr;
}

template <typename K, typename V>
bool DO1HashTable<K, V>::Remove(K key)
{
	// Hash the key and find the starting bucket
	const auto hash = HashKey(key) & s_62Bits;
	// size_t iBucketStart = hash % buckets.size();
	size_t iBucketStart = hash & (buckets.size() - 1);

	// Search the buckets until we hit an empty one
	for (size_t i = iBucketStart, iEnd = buckets.size(); i < iEnd; ++i)
	{
		Bucket * b = &buckets[i];
		switch (b->state)
		{
		case BSTATE_Empty:
			return false;
		case BSTATE_Filled:
			if (b->hash == hash)
			{
				KV * kv = &keyvals[i];
				if (kv->key == key)
				{
					b->hash = 0;
					b->state = BSTATE_Removed;
					--size;
					return true;
				}
			}
			break;
		default:
			break;
		}
	}
	for (size_t i = 0; i < iBucketStart; ++i)
	{
		Bucket * b = &buckets[i];
		switch (b->state)
		{
		case BSTATE_Empty:
			return false;
		case BSTATE_Filled:
			if (b->hash == hash)
			{
				KV * kv = &keyvals[i];
				if (kv->key == key)
				{
					b->hash = 0;
					b->state = BSTATE_Removed;
					--size;
					return true;
				}
			}
			break;
		default:
			break;
		}
	}

	return false;
}

template <typename K, typename V>
void DO1HashTable<K, V>::Reserve(size_t maxSize)
{
	maxSize = maxSize * 3 / 2;
    maxSize |= maxSize >> 1;
    maxSize |= maxSize >> 2;
    maxSize |= maxSize >> 4;
    maxSize |= maxSize >> 8;
    maxSize |= maxSize >> 16;

	Rehash(maxSize);
}

template <typename K, typename V>
void DO1HashTable<K, V>::Rehash(size_t bucketCountNew)
{
	// Can't rehash down to smaller than current size or initial size
	bucketCountNew = std::max(std::max(bucketCountNew, size),
						   size_t(s_hashTableInitialSize));

	// Build a new set of buckets and keyvals
	std::vector<Bucket> bucketsNew(bucketCountNew);
	std::vector<KV> keyvalsNew(bucketCountNew);

	// Walk through all the current elements and insert them into the new buckets
	for (size_t i = 0, iEnd = buckets.size(); i < iEnd; ++i)
	{
		Bucket * b = &buckets[i];
		if (b->state != BSTATE_Filled)
			continue;

		// Hash the key and find the starting bucket
		const auto hash = b->hash;
		// size_t iBucketStart = hash % bucketCountNew;
		size_t iBucketStart = hash & (bucketCountNew - 1);

		// Search for an unused bucket
		Bucket * bTarget = nullptr;
		KV * kvTarget = nullptr;
		for (size_t j = iBucketStart; j < bucketCountNew; ++j) 
		{
			Bucket * bNew = &bucketsNew[j];
			if (bNew->state != BSTATE_Filled)
			{
				bTarget = bNew;
				kvTarget = &keyvalsNew[j];
				break;
			}
		}
		if (!bTarget)
		{
			for (size_t j = 0; j < iBucketStart; ++j)
			{
				Bucket * bNew = &bucketsNew[j];
				if (bNew->state != BSTATE_Filled)
				{
					bTarget = bNew;
					kvTarget = &keyvalsNew[j];
					break;
				}
			}
		}

		assert(bTarget);
		assert(kvTarget);

		// Store the hash, key, and value in the bucket
		bTarget->hash = hash;
		bTarget->state = BSTATE_Filled;
		KV * kv = &keyvals[i];
		kvTarget->key = std::move(kv->key);
		kvTarget->value = std::move(kv->value);
	}

	// Swap the new buckets and keyvals into place
	buckets.swap(bucketsNew);
	keyvals.swap(keyvalsNew);
}

template <typename K, typename V>
void DO1HashTable<K, V>::Reset()
{
	// Blow away the current table and reset to small initial size
	buckets.clear();
	buckets.resize(s_hashTableInitialSize);
	keyvals.clear();
	keyvals.resize(s_hashTableInitialSize);

	size = 0;
}



// DO2HashTable implementation

template <typename K, typename V>
DO2HashTable<K, V>::DO2HashTable()
:	size(0)
{
	// Start off with a small initial size
	buckets.resize(s_hashTableInitialSize);
	keys.resize(s_hashTableInitialSize);
	values.resize(s_hashTableInitialSize);
}

template <typename K, typename V>
void DO2HashTable<K, V>::Insert(K key, V value)
{
	// Resize larger if the load factor goes over 2/3
	if (size * 3 > buckets.size() * 2)
	{
		Rehash(buckets.size() * 2);
	}

	// Hash the key and find the starting bucket
	const auto hash = HashKey(key) & s_62Bits;
	// size_t iBucketStart = hash % buckets.size();
	size_t iBucketStart = hash & (buckets.size() - 1);

	// Search for an unused bucket
	Bucket * bTarget = nullptr;
	size_t iBucketTarget = 0;
	for (size_t i = iBucketStart, iEnd = buckets.size(); i < iEnd; ++i) 
	{
		Bucket * b = &buckets[i];
		if (b->state != BSTATE_Filled)
		{
			bTarget = b;
			iBucketTarget = i;
			break;
		}
	}
	if (!bTarget)
	{
		for (size_t i = 0; i < iBucketStart; ++i)
		{
			Bucket * b = &buckets[i];
			if (b->state != BSTATE_Filled)
			{
				bTarget = b;
				iBucketTarget = i;
				break;
			}
		}
	}

	assert(bTarget);

	// Store the hash, key, and value in the bucket
	bTarget->hash = hash;
	bTarget->state = BSTATE_Filled;
	keys[iBucketTarget] = key;
	values[iBucketTarget] = value;

	++size;
}

template <typename K, typename V>
V * DO2HashTable<K, V>::Lookup(K key)
{
	// Hash the key and find the starting bucket
	const auto hash = HashKey(key) & s_62Bits;
	// size_t iBucketStart = hash % buckets.size();
	size_t iBucketStart = hash & (buckets.size() - 1);

	// Search the buckets until we hit an empty one
	for (size_t i = iBucketStart, iEnd = buckets.size(); i < iEnd; ++i)
	{
		Bucket * b = &buckets[i];
		switch (b->state)
		{
		case BSTATE_Empty:
			return nullptr;
		case BSTATE_Filled:
			if (b->hash == hash && keys[i] == key)
				return &values[i];
			break;
		default:
			break;
		}
	}
	for (size_t i = 0; i < iBucketStart; ++i)
	{
		Bucket * b = &buckets[i];
		switch (b->state)
		{
		case BSTATE_Empty:
			return nullptr;
		case BSTATE_Filled:
			if (b->hash == hash && keys[i] == key)
				return &values[i];
			break;
		default:
			break;
		}
	}

	return nullptr;
}

template <typename K, typename V>
bool DO2HashTable<K, V>::Remove(K key)
{
	// Hash the key and find the starting bucket
	const auto hash = HashKey(key) & s_62Bits;
	// size_t iBucketStart = hash % buckets.size();
	size_t iBucketStart = hash & (buckets.size() - 1);

	// Search the buckets until we hit an empty one
	for (size_t i = iBucketStart, iEnd = buckets.size(); i < iEnd; ++i)
	{
		Bucket * b = &buckets[i];
		switch (b->state)
		{
		case BSTATE_Empty:
			return false;
		case BSTATE_Filled:
			if (b->hash == hash && keys[i] == key)
			{
				b->hash = 0;
				b->state = BSTATE_Removed;
				--size;
				return true;
			}
			break;
		default:
			break;
		}
	}
	for (size_t i = 0; i < iBucketStart; ++i)
	{
		Bucket * b = &buckets[i];
		switch (b->state)
		{
		case BSTATE_Empty:
			return false;
		case BSTATE_Filled:
			if (b->hash == hash && keys[i] == key)
			{
				b->hash = 0;
				b->state = BSTATE_Removed;
				--size;
				return true;
			}
			break;
		default:
			break;
		}
	}

	return false;
}

template <typename K, typename V>
void DO2HashTable<K, V>::Reserve(size_t maxSize)
{
	maxSize = maxSize * 3 / 2;
    maxSize |= maxSize >> 1;
    maxSize |= maxSize >> 2;
    maxSize |= maxSize >> 4;
    maxSize |= maxSize >> 8;
    maxSize |= maxSize >> 16;

	Rehash(maxSize);
}

template <typename K, typename V>
void DO2HashTable<K, V>::Rehash(size_t bucketCountNew)
{
	// Can't rehash down to smaller than current size or initial size
	bucketCountNew = std::max(std::max(bucketCountNew, size),
						   size_t(s_hashTableInitialSize));

	// Build a new set of buckets, keys, and values
	std::vector<Bucket> bucketsNew(bucketCountNew);
	std::vector<K> keysNew(bucketCountNew);
	std::vector<V> valuesNew(bucketCountNew);

	// Walk through all the current elements and insert them into the new buckets
	for (size_t i = 0, iEnd = buckets.size(); i < iEnd; ++i)
	{
		Bucket * b = &buckets[i];
		if (b->state != BSTATE_Filled)
			continue;

		// Hash the key and find the starting bucket
		const auto hash = b->hash;
		// size_t iBucketStart = hash % bucketCountNew;
		size_t iBucketStart = hash & (bucketCountNew - 1);

		// Search for an unused bucket
		Bucket * bTarget = nullptr;
		size_t iBucketTarget = 0;
		for (size_t j = iBucketStart; j < bucketCountNew; ++j) 
		{
			Bucket * bNew = &bucketsNew[j];
			if (bNew->state != BSTATE_Filled)
			{
				bTarget = bNew;
				iBucketTarget = j;
				break;
			}
		}
		if (!bTarget)
		{
			for (size_t j = 0; j < iBucketStart; ++j)
			{
				Bucket * bNew = &bucketsNew[j];
				if (bNew->state != BSTATE_Filled)
				{
					bTarget = bNew;
					iBucketTarget = j;
					break;
				}
			}
		}

		assert(bTarget);

		// Store the hash, key, and value in the bucket
		bTarget->hash = hash;
		bTarget->state = BSTATE_Filled;
		keysNew[iBucketTarget] = std::move(keys[i]);
		valuesNew[iBucketTarget] = std::move(values[i]);
	}

	// Swap the new buckets, keys, and values into place
	buckets.swap(bucketsNew);
	keys.swap(keysNew);
	values.swap(valuesNew);
}

template <typename K, typename V>
void DO2HashTable<K, V>::Reset()
{
	// Blow away the current table and reset to small initial size
	buckets.clear();
	buckets.resize(s_hashTableInitialSize);
	keys.clear();
	keys.resize(s_hashTableInitialSize);
	values.clear();
	values.resize(s_hashTableInitialSize);

	size = 0;
}
