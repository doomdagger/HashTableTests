// Hash table performance tests

#include <cfloat>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <algorithm>
#include "hash-tables.h"
#include "timer.h"

static_assert(sizeof(int) == 4, "Int should be 4 bytes, what kind of weird architecture are you on?!");
static_assert(sizeof(size_t) == 8, "Compiling for 32-bit not supported!");

typedef unsigned int uint;

// Various payload sizes: payload is defined as sizeof(key) + sizeof(value),
// and keys here are 8-byte so e.g. data32 --> 8-byte key + 24-byte value
struct data32  { size_t data[(32   / sizeof(size_t)) - 1]; };
struct data128 { size_t data[(128  / sizeof(size_t)) - 1]; };
struct data1K  { size_t data[(1024 / sizeof(size_t)) - 1]; };
struct data4K  { size_t data[(4096 / sizeof(size_t)) - 1]; };

static_assert(sizeof(size_t) + sizeof(data32 ) == 32  , "data32 has wrong size!" );
static_assert(sizeof(size_t) + sizeof(data128) == 128 , "data128 has wrong size!");
static_assert(sizeof(size_t) + sizeof(data1K ) == 1024, "data1K has wrong size!" );
static_assert(sizeof(size_t) + sizeof(data4K ) == 4096, "data4K has wrong size!" );

void UnitTests();
template<typename K, typename V> void FillTiming(int numKeys, bool presize);
template<typename K, typename V> void LookupTiming(int numKeys, bool fail);
template<typename K, typename V> void RemoveTiming(int numKeys);
template<typename K, typename V> void DestructTiming(int numKeys);

FILE * g_pFileOut = nullptr;
void Log(const char * fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_start(args, fmt);
	vfprintf(g_pFileOut, fmt, args);
}

int g_reps = 5;
int g_allocs = 0;
int g_deallocs = 0;

#define COUNT_ALLOCS 0
#if COUNT_ALLOCS
#pragma warning(disable: 4290)	// C++ exception specification ignored
void * operator new(size_t size) throw(std::bad_alloc)
{
	++g_allocs;
	void * p = malloc(size);
	if (!p) throw std::bad_alloc();
	return p;
}
void * operator new[](size_t size) throw(std::bad_alloc)
{
	++g_allocs;
	void * p = malloc(size);
	if (!p) throw std::bad_alloc();
	return p;
}
void * operator new[](size_t size, const std::nothrow_t&) throw()	{ ++g_allocs; return malloc(size); }
void * operator new   (size_t size, const std::nothrow_t&) throw()	{ ++g_allocs; return malloc(size); }
void operator delete(void * ptr) throw()							{ ++g_deallocs; free(ptr); }
void operator delete(void * ptr, const std::nothrow_t &) throw()	{ ++g_deallocs; free(ptr); }
void operator delete[](void * ptr) throw()							{ ++g_deallocs; free(ptr); }
void operator delete[](void * ptr, const std::nothrow_t &) throw()	{ ++g_deallocs; free(ptr); }
#endif // COUNT_ALLOCS

int main (int argc, const char ** argv)
{
	(void)argc;
	(void)argv;

	UnitTests();

	bool timeMediumPayloads = true;
	bool timeLargePayloads	= false;		// Note: this makes it take quite a bit longer
	bool timeFill			= true;
	bool timePresizedFill	= true;
	bool timeLookup			= true;
	bool timeFailedLookup	= true;
	bool timeRemove			= true;
	bool timeDestruct		= true;

	clock_t clockStart = clock();

	int numKeysMax = 10000;
	int stepSize = numKeysMax / 10;

#ifdef _MSC_VER
	fopen_s(&g_pFileOut, "results.txt", "wt");
#else
	g_pFileOut = fopen("results.txt", "wt");
#endif

	Log(
		"Key:\tUM = unordered_map\n"
		"\tCh = separate chaining with an element pool and free-list\n"
		"\tOL = open addressing with linear probing\n"
		"\tDO1 = \"data-oriented\": OA, linear, with hashes stored separately from keys and values\n"
		"\tDO2 = \"data-oriented\": OA, linear, with hashes, keys, and values all separate\n"
		);

	if (timeFill)
	{
		Log(
			"\n"
			"Fill time (ms)\t8 bytes\t\t\t\t\t\t32 bytes\t\t\t\t\t\t128 bytes\t\t\t\t\t\t1K bytes\t\t\t\t\t\t4K bytes\n"
			"Elem count\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\n"
			);
		for (int numKeys = stepSize; numKeys <= numKeysMax; numKeys += stepSize)
		{
			Log("%d", numKeys);
			FillTiming<uint, uint>(numKeys, false);
			if (timeMediumPayloads)
			{
				Log("\t");
				FillTiming<uint, data32> (numKeys, false); Log("\t");
				FillTiming<uint, data128>(numKeys, false);
			}
			if (timeLargePayloads)
			{
				Log("\t");
				FillTiming<uint, data1K> (numKeys, false); Log("\t");
				FillTiming<uint, data4K> (numKeys, false);
			}
			Log("\n");
		}
	}

	if (timePresizedFill)
	{
		Log(
			"\n"
			"Presized fill time (ms)\t8 bytes\t\t\t\t\t\t32 bytes\t\t\t\t\t\t128 bytes\t\t\t\t\t\t1K bytes\t\t\t\t\t\t4K bytes\n"
			"Elem count\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\n"
			);
		for (int numKeys = stepSize; numKeys <= numKeysMax; numKeys += stepSize)
		{
			Log("%d", numKeys);
			FillTiming<uint, uint>(numKeys, true);
			if (timeMediumPayloads)
			{
				Log("\t");
				FillTiming<uint, data32> (numKeys, true); Log("\t");
				FillTiming<uint, data128>(numKeys, true);
			}
			if (timeLargePayloads)
			{
				Log("\t");
				FillTiming<uint, data1K> (numKeys, true); Log("\t");
				FillTiming<uint, data4K> (numKeys, true);
			}
			Log("\n");
		}
	}

	if (timeLookup)
	{
		Log(
			"\n"
			"Time for 100K lookups (ms)\t8 bytes\t\t\t\t\t\t32 bytes\t\t\t\t\t\t128 bytes\t\t\t\t\t\t1K bytes\t\t\t\t\t\t4K bytes\n"
			"Elem count\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\n"
			);
		for (int numKeys = stepSize; numKeys <= numKeysMax; numKeys += stepSize)
		{
			Log("%d", numKeys);
			LookupTiming<uint, uint>(numKeys, false);
			if (timeMediumPayloads)
			{
				Log("\t");
				LookupTiming<uint, data32> (numKeys, false); Log("\t");
				LookupTiming<uint, data128>(numKeys, false);
			}
			if (timeLargePayloads)
			{
				Log("\t");
				LookupTiming<uint, data1K> (numKeys, false); Log("\t");
				LookupTiming<uint, data4K> (numKeys, false);
			}
			Log("\n");
		}
	}

	if (timeFailedLookup)
	{
		Log(
			"\n"
			"Time for 100K failed lookups (ms)\t8 bytes\t\t\t\t\t\t32 bytes\t\t\t\t\t\t128 bytes\t\t\t\t\t\t1K bytes\t\t\t\t\t\t4K bytes\n"
			"Elem count\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\n"
			);
		for (int numKeys = stepSize; numKeys <= numKeysMax; numKeys += stepSize)
		{
			Log("%d", numKeys);
			LookupTiming<uint, uint>(numKeys, true);
			if (timeMediumPayloads)
			{
				Log("\t");
				LookupTiming<uint, data32> (numKeys, true); Log("\t");
				LookupTiming<uint, data128>(numKeys, true);
			}
			if (timeLargePayloads)
			{
				Log("\t");
				LookupTiming<uint, data1K> (numKeys, true); Log("\t");
				LookupTiming<uint, data4K> (numKeys, true);
			}
			Log("\n");
		}
	}

	if (timeRemove)
	{
		Log(
			"\n"
			"Time to remove half the elements (ms)\t8 bytes\t\t\t\t\t\t32 bytes\t\t\t\t\t\t128 bytes\t\t\t\t\t\t1K bytes\t\t\t\t\t\t4K bytes\n"
			"Elem count\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\t\tUM\tCh\tOL\tDO1\tDO2\tD0\tD1\n"
			);
		for (int numKeys = stepSize; numKeys <= numKeysMax; numKeys += stepSize)
		{
			Log("%d", numKeys);
			RemoveTiming<uint, uint>(numKeys);
			if (timeMediumPayloads)
			{
				Log("\t");
				RemoveTiming<uint, data32> (numKeys); Log("\t");
				RemoveTiming<uint, data128>(numKeys);
			}
			if (timeLargePayloads)
			{
				Log("\t");
				RemoveTiming<uint, data1K> (numKeys); Log("\t");
				RemoveTiming<uint, data4K> (numKeys);
			}
			Log("\n");
		}
	}

	if (timeDestruct)
	{
		Log(
			"\n"
			"Destruction time (ms)\t8 bytes\t\t\t\t\t\t32 bytes\t\t\t\t\t\t128 bytes\t\t\t\t\t\t1K bytes\t\t\t\t\t\t4K bytes\n"
			"Elem count\tUM\tCh\tOL\tDO1\tDO2\t\tUM\tCh\tOL\tDO1\tDO2\t\tUM\tCh\tOL\tDO1\tDO2\t\tUM\tCh\tOL\tDO1\tDO2\t\tUM\tCh\tOL\tDO1\tDO2\n"
			);
		for (int numKeys = stepSize; numKeys <= numKeysMax; numKeys += stepSize)
		{
			Log("%d", numKeys);
			DestructTiming<uint, uint>(numKeys);
			if (timeMediumPayloads)
			{
				Log("\t");
				DestructTiming<uint, data32> (numKeys); Log("\t");
				DestructTiming<uint, data128>(numKeys);
			}
			if (timeLargePayloads)
			{
				Log("\t");
				DestructTiming<uint, data1K> (numKeys); Log("\t");
				DestructTiming<uint, data4K> (numKeys);
			}
			Log("\n");
		}
	}

	fclose(g_pFileOut);
	printf("Results written to results.txt\n");

	clock_t clockEnd = clock();
	printf("Done in %.0f seconds\n", float(clockEnd - clockStart) / float(CLOCKS_PER_SEC));

	return 0;
}



// Xorshift RNG implemented as a C++11 UniformRandomNumberGenerator concept
class XorshiftRNG
{
public:
	typedef uint result_type;
	result_type state;

	static result_type min() { return 0; }
	static result_type max() { return result_type(-1); }
	result_type operator() ()
	{
		// Xorshift algorithm from George Marsaglia's paper
		state ^= (state << 13);
		state ^= (state >> 17);
		state ^= (state << 5);
		return state;
	}
};



// Unit tests, to make sure hash tables are functioning correctly

// Helper functions to run tests
template<typename HT>
void UnitTests(
	int numKeys,
	const std::vector<uint> & keys,
	const std::vector<uint> & values,
	const char * name)
{
	// First test: insertion and lookup
	{
		HT ht;
		// First insert some keys
		for (int i = 0; i < numKeys; ++i)
			ht.Insert(keys[i], values[i]);
		// Then test they can all be looked-up
		for (int i = 0; i < numKeys; ++i)
		{
			uint * pValue = ht.Lookup(keys[i]);
			if (!pValue)
			{
				printf("%s: failed to lookup previously-inserted key\n", name);
				return;
			}
			if (*pValue != values[i])
			{
				printf("%s: lookup returned wrong value\n", name);
				return;
			}
		}
	}

	// Second test: repeated insertion and removal
	{
		HT ht;
		int numRounds = 10;
		int keysPerRound = numKeys / numRounds;
		// First insert two rounds' worth of keys
		for (int i = 0, iEnd = keysPerRound * 2; i < iEnd; ++i)
			ht.Insert(keys[i], values[i]);
		// Each round removes a bunch of keys and inserts some more
		for (int i = 2; i < numRounds; ++i)
		{
			for (int j = keysPerRound * (i - 2), jEnd = j + keysPerRound; j < jEnd; ++j)
			{
				if (!ht.Remove(keys[j]))
				{
					printf("%s: failed to remove previously-inserted key\n", name);
					return;
				}
			}
			for (int j = keysPerRound * i, jEnd = j + keysPerRound; j < jEnd; ++j)
				ht.Insert(keys[j], values[j]);
		}
		// Test that all the removed ones are no longer present
		for (int i = 0, iEnd = keysPerRound * (numRounds - 2); i < iEnd; ++i)
		{
			uint * pValue = ht.Lookup(keys[i]);
			if (pValue)
			{
				printf("%s: key still findable after being removed\n", name);
				return;
			}
		}
		// Then test all the non-removed ones are still present
		for (int i = keysPerRound * (numRounds - 2), iEnd = i + keysPerRound * 2; i < iEnd; ++i)
		{
			uint * pValue = ht.Lookup(keys[i]);
			if (!pValue)
			{
				printf("%s: failed to lookup previously-inserted key after removes\n", name);
				return;
			}
			if (*pValue != values[i])
			{
				printf("%s: lookup returned wrong value after removes\n", name);
				return;
			}
		}
	}
	
	printf("%s: all tests passed\n", name);
}

void UnitTests()
{
	static const int numKeys = 1000;

	// Create a list of guaranteed unique keys by random shuffling
	std::vector<uint> keys(numKeys);
	for (int i = 0; i < numKeys; ++i)
		keys[i] = i;
	XorshiftRNG rng = { 0xbeeff00d };
	std::shuffle(keys.begin(), keys.end(), rng);

	// Create a list of random values
	std::vector<uint> values(numKeys);
	for (int i = 0; i < numKeys; ++i)
		values[i] = rng();

	// Run tests
	UnitTests<UMHashTable<uint, uint>>(numKeys, keys, values, "unordered_map");
	UnitTests<C0HashTable<uint, uint>>(numKeys, keys, values, "C0HashTable");
	UnitTests<C1HashTable<uint, uint>>(numKeys, keys, values, "C1HashTable");
	UnitTests<OLHashTable<uint, uint>>(numKeys, keys, values, "OLHashTable");
	UnitTests<OQHashTable<uint, uint>>(numKeys, keys, values, "OQHashTable");
	UnitTests<DO1HashTable<uint, uint>>(numKeys, keys, values, "DO1HashTable");
	UnitTests<DO2HashTable<uint, uint>>(numKeys, keys, values, "DO2HashTable");

	UnitTests<D0HashTable<uint, uint>>(numKeys, keys, values, "D0HashTable");
	UnitTests<D1HashTable<uint, uint>>(numKeys, keys, values, "D1HashTable");
}



// Timing tests

template<typename K, typename V>
void FillTiming(int numKeys, bool presize)
{
	// Create a list of guaranteed unique keys by random shuffling
	std::vector<uint> keys(numKeys);
	for (int i = 0; i < numKeys; ++i)
		keys[i] = i;
	XorshiftRNG rng = { 0xf002beef };
	std::shuffle(keys.begin(), keys.end(), rng);

	// Run tests and measure timing
	float timeMin;
	timeMin = FLT_MAX;
	g_allocs = 0;
	for (int i = 0; i < g_reps; ++i)
	{
		UMHashTable<K, V> ht;
		Timer timer;
		timer.Start();
		if (presize)
			ht.Reserve(numKeys);
		for (int i = 0; i < numKeys; ++i)
		{
			ht.Insert(keys[i], V());
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_allocs);
#else
	Log("\t%0.2f", timeMin);
#endif

	timeMin = FLT_MAX;
	g_allocs = 0;
	for (int i = 0; i < g_reps; ++i)
	{
		C0HashTable<K, V> ht;
		Timer timer;
		timer.Start();
		if (presize)
			ht.Reserve(numKeys);
		for (int i = 0; i < numKeys; ++i)
		{
			ht.Insert(keys[i], V());
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_allocs);
#else
	Log("\t%0.2f", timeMin);
#endif

	timeMin = FLT_MAX;
	g_allocs = 0;
	for (int i = 0; i < g_reps; ++i)
	{
		OLHashTable<K, V> ht;
		Timer timer;
		timer.Start();
		if (presize)
			ht.Reserve(numKeys);
		for (int i = 0; i < numKeys; ++i)
		{
			ht.Insert(keys[i], V());
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_allocs);
#else
	Log("\t%0.2f", timeMin);
#endif

	timeMin = FLT_MAX;
	g_allocs = 0;
	for (int i = 0; i < g_reps; ++i)
	{
		DO1HashTable<K, V> ht;
		Timer timer;
		timer.Start();
		if (presize)
			ht.Reserve(numKeys);
		for (int i = 0; i < numKeys; ++i)
		{
			ht.Insert(keys[i], V());
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_allocs);
#else
	Log("\t%0.2f", timeMin);
#endif

	timeMin = FLT_MAX;
	g_allocs = 0;
	for (int i = 0; i < g_reps; ++i)
	{
		DO2HashTable<K, V> ht;
		Timer timer;
		timer.Start();
		if (presize)
			ht.Reserve(numKeys);
		for (int i = 0; i < numKeys; ++i)
		{
			ht.Insert(keys[i], V());
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_allocs);
#else
	Log("\t%0.2f", timeMin);
#endif

	timeMin = FLT_MAX;
	g_allocs = 0;
	for (int i = 0; i < g_reps; ++i)
	{
		D0HashTable<K, V> ht;
		Timer timer;
		timer.Start();
		if (presize)
			ht.Reserve(numKeys);
		for (int i = 0; i < numKeys; ++i)
		{
			ht.Insert(keys[i], V());
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}

#if COUNT_ALLOCS
	Log("\t%d", g_allocs);
#else
	Log("\t%0.2f", timeMin);
#endif


	timeMin = FLT_MAX;
	g_allocs = 0;
	for (int i = 0; i < g_reps; ++i)
	{
		D1HashTable<K, V> ht;
		Timer timer;
		timer.Start();
		if (presize)
			ht.Reserve(numKeys);
		for (int i = 0; i < numKeys; ++i)
		{
			ht.Insert(keys[i], V());
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}

#if COUNT_ALLOCS
	Log("\t%d", g_allocs);
#else
	Log("\t%0.2f", timeMin);
#endif

}

// Helper functions to fill a hash table with some keys
template<typename K, typename V, template<typename, typename> class HT>
void Fill(HT<K, V> & ht, int numKeys)
{
	// Create a list of guaranteed unique keys by random shuffling
	std::vector<uint> keys(numKeys);
	for (int i = 0; i < numKeys; ++i)
		keys[i] = i;
	XorshiftRNG rng = { 0xf002beef };
	std::shuffle(keys.begin(), keys.end(), rng);

	ht.Reserve(numKeys);
	for (int i = 0; i < numKeys; ++i)
	{
		ht.Insert(keys[i], V());
	}
}

// Write lookup results to a dummy volatile to prevent compiler from optimizing them away
volatile size_t dummy;

template<typename K, typename V>
void LookupTiming(int numKeys, bool fail)
{
	static const int numLookups = 100000;
	XorshiftRNG rng = { 0xfaf4f00d };

	// Create a list of random keys to lookup
	std::vector<uint> keys(numLookups);
	if (fail)
	{
		for (int i = 0; i < numLookups; ++i)
			keys[i] = numKeys + (rng() % numKeys);
	}
	else
	{
		for (int i = 0; i < numLookups; ++i)
			keys[i] = rng() % numKeys;
	}

	// Run tests and measure timing
	float timeMin;
	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		UMHashTable<K, V> ht;
		Fill(ht, numKeys);
		Timer timer;
		timer.Start();
		for (int i = 0; i < numLookups; ++i)
		{
			V * pValue = ht.Lookup(keys[i]);
			if (pValue)
				dummy = *(size_t *)pValue;
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
	Log("\t%0.2f", timeMin);

	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		C0HashTable<K, V> ht;
		Fill(ht, numKeys);
		Timer timer;
		timer.Start();
		for (int i = 0; i < numLookups; ++i)
		{
			V * pValue = ht.Lookup(keys[i]);
			if (pValue)
				dummy = *(size_t *)pValue;
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
	Log("\t%0.2f", timeMin);

	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		OLHashTable<K, V> ht;
		Fill(ht, numKeys);
		Timer timer;
		timer.Start();
		for (int i = 0; i < numLookups; ++i)
		{
			V * pValue = ht.Lookup(keys[i]);
			if (pValue)
				dummy = *(size_t *)pValue;
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
	Log("\t%0.2f", timeMin);

	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		DO1HashTable<K, V> ht;
		Fill(ht, numKeys);
		Timer timer;
		timer.Start();
		for (int i = 0; i < numLookups; ++i)
		{
			V * pValue = ht.Lookup(keys[i]);
			if (pValue)
				dummy = *(size_t *)pValue;
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
	Log("\t%0.2f", timeMin);

	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		DO2HashTable<K, V> ht;
		Fill(ht, numKeys);
		Timer timer;
		timer.Start();
		for (int i = 0; i < numLookups; ++i)
		{
			V * pValue = ht.Lookup(keys[i]);
			if (pValue)
				dummy = *(size_t *)pValue;
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
	Log("\t%0.2f", timeMin);


	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		D0HashTable<K, V> ht;
		Fill(ht, numKeys);
		Timer timer;
		timer.Start();
		for (int i = 0; i < numLookups; ++i)
		{
			V * pValue = ht.Lookup(keys[i]);
			if (pValue)
				dummy = *(size_t *)pValue;
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
	Log("\t%0.2f", timeMin);

	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		D1HashTable<K, V> ht;
		Fill(ht, numKeys);
		Timer timer;
		timer.Start();
		for (int i = 0; i < numLookups; ++i)
		{
			V * pValue = ht.Lookup(keys[i]);
			if (pValue)
				dummy = *(size_t *)pValue;
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
	Log("\t%0.2f", timeMin);
}

template<typename K, typename V>
void RemoveTiming(int numKeys)
{
	int numRemoves = numKeys / 2;
	XorshiftRNG rng = { 0xba28beef };

	// Create a list of random keys to remove
	std::vector<uint> keys(numRemoves);
	for (int i = 0; i < numRemoves; ++i)
		keys[i] = rng() % numKeys;

	// Run tests and measure timing
	float timeMin;
	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		UMHashTable<K, V> ht;
		Fill(ht, numKeys);
		g_deallocs = 0;
		Timer timer;
		timer.Start();
		for (int i = 0; i < numRemoves; ++i)
		{
			ht.Remove(keys[i]);
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_deallocs);
#else
	Log("\t%0.2f", timeMin);
#endif

	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		C0HashTable<K, V> ht;
		Fill(ht, numKeys);
		g_deallocs = 0;
		Timer timer;
		timer.Start();
		for (int i = 0; i < numRemoves; ++i)
		{
			ht.Remove(keys[i]);
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_deallocs);
#else
	Log("\t%0.2f", timeMin);
#endif

	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		OLHashTable<K, V> ht;
		Fill(ht, numKeys);
		g_deallocs = 0;
		Timer timer;
		timer.Start();
		for (int i = 0; i < numRemoves; ++i)
		{
			ht.Remove(keys[i]);
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_deallocs);
#else
	Log("\t%0.2f", timeMin);
#endif

	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		DO1HashTable<K, V> ht;
		Fill(ht, numKeys);
		g_deallocs = 0;
		Timer timer;
		timer.Start();
		for (int i = 0; i < numRemoves; ++i)
		{
			ht.Remove(keys[i]);
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_deallocs);
#else
	Log("\t%0.2f", timeMin);
#endif

	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		DO2HashTable<K, V> ht;
		Fill(ht, numKeys);
		g_deallocs = 0;
		Timer timer;
		timer.Start();
		for (int i = 0; i < numRemoves; ++i)
		{
			ht.Remove(keys[i]);
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_deallocs);
#else
	Log("\t%0.2f", timeMin);
#endif


	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		D0HashTable<K, V> ht;
		Fill(ht, numKeys);
		g_deallocs = 0;
		Timer timer;
		timer.Start();
		for (int i = 0; i < numRemoves; ++i)
		{
			ht.Remove(keys[i]);
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_deallocs);
#else
	Log("\t%0.2f", timeMin);
#endif



	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		D1HashTable<K, V> ht;
		Fill(ht, numKeys);
		g_deallocs = 0;
		Timer timer;
		timer.Start();
		for (int i = 0; i < numRemoves; ++i)
		{
			ht.Remove(keys[i]);
		}
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_deallocs);
#else
	Log("\t%0.2f", timeMin);
#endif

}

template<typename K, typename V>
void DestructTiming(int numKeys)
{
	// Run tests and measure timing
	float timeMin;
	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		UMHashTable<K, V> * ht = new UMHashTable<K, V>;
		Fill(*ht, numKeys);
		g_deallocs = 0;
		Timer timer;
		timer.Start();
		delete ht;
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_deallocs);
#else
	Log("\t%0.2f", timeMin);
#endif

	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		C0HashTable<K, V> * ht = new C0HashTable<K, V>;
		Fill(*ht, numKeys);
		g_deallocs = 0;
		Timer timer;
		timer.Start();
		delete ht;
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_deallocs);
#else
	Log("\t%0.2f", timeMin);
#endif

	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		OLHashTable<K, V> * ht = new OLHashTable<K, V>;
		Fill(*ht, numKeys);
		g_deallocs = 0;
		Timer timer;
		timer.Start();
		delete ht;
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_deallocs);
#else
	Log("\t%0.2f", timeMin);
#endif

	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		DO1HashTable<K, V> * ht = new DO1HashTable<K, V>;
		Fill(*ht, numKeys);
		g_deallocs = 0;
		Timer timer;
		timer.Start();
		delete ht;
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_deallocs);
#else
	Log("\t%0.2f", timeMin);
#endif

	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		DO2HashTable<K, V> * ht = new DO2HashTable<K, V>;
		Fill(*ht, numKeys);
		g_deallocs = 0;
		Timer timer;
		timer.Start();
		delete ht;
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_deallocs);
#else
	Log("\t%0.2f", timeMin);
#endif

	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		D0HashTable<K, V> * ht = new D0HashTable<K, V>;
		Fill(*ht, numKeys);
		g_deallocs = 0;
		Timer timer;
		timer.Start();
		delete ht;
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_deallocs);
#else
	Log("\t%0.2f", timeMin);
#endif


	timeMin = FLT_MAX;
	for (int i = 0; i < g_reps; ++i)
	{
		D1HashTable<K, V> * ht = new D1HashTable<K, V>;
		Fill(*ht, numKeys);
		g_deallocs = 0;
		Timer timer;
		timer.Start();
		delete ht;
		timer.Stop();
		timeMin = std::min(timeMin, timer.msAccumulated);
	}
#if COUNT_ALLOCS
	Log("\t%d", g_deallocs);
#else
	Log("\t%0.2f", timeMin);
#endif

}
