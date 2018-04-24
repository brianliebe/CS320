#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string.h>

using namespace std;

struct result 
{
	int hits;
	int misses;
	int total;
	result() 
	{
		hits = 0;
		misses = 0;
		total = 0;
	}
};

result directMappedCache(int size, string infile) 
{
	result res;
	fstream in(infile);
	int array[size/32];
	for (int i = 0; i < size/32; i++) array[i] = 0;

	string a, b;
	while (in >> a >> b) 
	{
		res.total++;
		int index, tag;
		stringstream str;
		str << b;
		str >> hex >> tag;
		index = (tag / 32) % (size/32);
		if (array[index] == 32 * (tag / 32))
		{
			res.hits++;
		}
		else
		{
			res.misses++;
			array[index] = 32 * (tag / 32);
		}
	}
	in.close();
	return res;
}

result setAssociativeCache(int size, string infile) 
{
	result res;
	fstream in(infile);
	int rows = 16*32 / size;
	int array[rows][size];
	int lru[rows][size];
	int time = 0;

	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < size; j++)
		{
			array[i][j] = 0;
			lru[i][j] = 0;
		}
	}

	string a,b;
	while (in >> a >> b)
	{
		res.total++;
		int index, tag;
		stringstream str;
		str << b;
		str >> hex >> tag;
		index = (tag / 32) % rows;

		bool found = false;
		for (int i = 0; i < size; i++) 
		{
			if (array[index][i] == 32 * (tag / 32))
			{
				res.hits++;
				lru[index][i] = ++time;
				found = true;
				break;
			}
		}
		if (!found)
		{
			res.misses++;
			int value = lru[index][0];
			int loc = 0;
			for (int i = 0; i < size; i++)
			{
				if (lru[index][i] < value) 
				{
					value = lru[index][i];
					loc = i;
				}
			}
			lru[index][loc] = ++time;
			array[index][loc] = 32 * (tag / 32);
		}
	}
	in.close();
	return res;
}

result fullyAssociativeCacheLRU (string infile)
{
	result res;
	fstream in(infile);
	int columns = 512;
	int array[columns];
	int lru[columns];
	int time = 0;

	for (int i = 0; i < columns; i++) 
	{
		array[i] = 0;
		lru[i] = 0;
	}
	
	string a,b;
	while (in >> a >> b)
	{
		res.total++;
		int index, tag;
		stringstream str;
		str << b;
		str >> hex >> tag;
		index = (tag / 32);
		tag = 32 * (tag / 32);

		bool found = false;

		for (int i = 0; i < 512; i++) 
		{
			if (array[i] == tag)
			{
				res.hits++;
				lru[i] = ++time;
				found = true;
				break;
			}
		}
		if (!found) 
		{
			res.misses++;
			int ind = 0;
			int value = lru[0];
			for (int i = 0; i < 512; i++) 
			{
				if (lru[i] < value) 
				{
					value = lru[i];
					ind = i;
				}
			}
			lru[ind] = ++time;
			array[ind] = tag;
		}
	}
	in.close();
	return res;
}

int findColdestBit(int * arr, int length, int offset) 
{
	int ret[length/2];
	if (length == 1) 
	{
		if (arr[0] == 1) return offset;
		else return offset + 1;
	}
	if (arr[length/2] == 1)
	{
		memcpy(ret, arr, (length/2) * sizeof(int));
		return findColdestBit(ret, length/2, offset);
	}
	else // 0
	{
		memcpy(ret, arr+(length/2 + 1), (length/2) * sizeof(int));
		return findColdestBit(ret, length/2, offset + (length/2 + 1));
	}
	return 0;
}

int* changeBits(int* arr, int length, int loc)
{
	int level = (length + 1) / 2;
	int value = length / 2;
	while (level > 0) 
	{
		if (level == 1) 
		{
			if (loc == value)
			{
				arr[value] = 0;
			}
			else
			{
				arr[value] = 1;
			}
			break;
		}
		level /= 2;
		if (loc <= value) 
		{
			arr[value] = 0;
			value -= level;
		}
		else
		{
			arr[value] = 1;
			value += level;
		}
	}

	return arr;
}


result fullyAssociativeCacheHotCold (string infile) 
{
	result res;
	fstream in(infile);
	int columns = 512;
	int array[columns];
	int bits[columns - 1];
	int time = 0;

	for (int i = 0; i < columns; i++) 
	{
		array[i] = 0;
		bits[i] = 0;
	}
	
	string a,b;
	while (in >> a >> b)
	{
		res.total++;
		int index, tag;
		bool found = false;
		stringstream str;
		str << b;
		str >> hex >> tag;
		tag = 32 * (tag / 32);
		for (int i = 0; i < 512; i++)
		{
			if (array[i] == tag) 
			{
				res.hits++;
				found = true;
				memcpy(bits, changeBits(bits, 511, i), 511*sizeof(int));
				// change bits
			}
		}
		if (!found) 
		{
			
			res.misses++;
			int loc = findColdestBit(bits, 511, 0);
			memcpy(bits, changeBits(bits, 511, loc), 511*sizeof(int));
			array[loc] = tag;
			
		}
	}
	in.close();
	return res;
}

result setAssociativeCacheNoAllocation(int size, string infile) 
{
	result res;
	fstream in(infile);
	int rows = 16*32 / size;
	int array[rows][size];
	int lru[rows][size];
	int time = 0;

	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < size; j++)
		{
			array[i][j] = 0;
			lru[i][j] = 0;
		}
	}

	string a,b;
	while (in >> a >> b)
	{
		res.total++;
		int index, tag;
		stringstream str;
		str << b;
		str >> hex >> tag;
		index = (tag / 32) % rows;

		bool found = false;
		for (int i = 0; i < size; i++) 
		{
			if (array[index][i] == 32 * (tag / 32))
			{
				res.hits++;
				lru[index][i] = ++time;
				found = true;
				break;
			}
		}
		if (!found && a == "L")
		{
			res.misses++;
			int value = lru[index][0];
			int loc = 0;
			for (int i = 0; i < size; i++)
			{
				if (lru[index][i] < value) 
				{
					value = lru[index][i];
					loc = i;
				}
			}
			lru[index][loc] = ++time;
			array[index][loc] = 32 * (tag / 32);
		}
	}
	in.close();
	return res;
}

result setAssociativeCacheNextLinePrefetching(int size, string infile) 
{
	result res;
	fstream in(infile);
	int rows = 16*32 / size;
	int array[rows][size];
	int lru[rows][size];
	int time = 0;

	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < size; j++)
		{
			array[i][j] = 0;
			lru[i][j] = 0;
		}
	}

	string a,b;
	while (in >> a >> b)
	{
		res.total++;
		int index, tag;
		stringstream str;
		str << b;
		str >> hex >> tag;
		index = (tag / 32) % rows;

		bool found = false;
		for (int i = 0; i < size; i++) 
		{
			if (array[index][i] == 32 * (tag / 32))
			{
				res.hits++;
				lru[index][i] = ++time;
				found = true;
				break;
			}
		}
		if (!found)
		{
			res.misses++;
			int value = lru[index][0];
			int loc = 0;
			for (int i = 0; i < size; i++)
			{
				if (lru[index][i] < value) 
				{
					value = lru[index][i];
					loc = i;
				}
			}
			lru[index][loc] = ++time;
			array[index][loc] = 32 * (tag / 32);
		}

		int newindex = (tag / 32 + 1) % rows;
		found = false;
		for (int i = 0; i < size; i++)
		{
			if (array[newindex][i] == 32 * (tag / 32 + 1))
			{
				found = true;
				lru[newindex][i] = ++time;
				break;
			}
		}
		if (!found) 
		{
			int value = lru[newindex][0];
			int loc = 0;
			for (int i = 0; i < size; i++) 
			{
				if (lru[newindex][i] < value)
				{
					value = lru[newindex][i];
					loc = i;
				}
			}
			lru[newindex][loc] = ++time;
			array[newindex][loc] = 32 * ((tag / 32) + 1);
		}
	}
	in.close();
	return res;
}

result setAssociativeCacheMissPrefetching(int size, string infile) 
{
	result res;
	fstream in(infile);
	int rows = 16*32 / size;
	int array[rows][size];
	int lru[rows][size];
	int time = 0;

	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < size; j++)
		{
			array[i][j] = 0;
			lru[i][j] = 0;
		}
	}

	string a,b;
	while (in >> a >> b)
	{
		res.total++;
		int index, tag;
		stringstream str;
		str << b;
		str >> hex >> tag;
		index = (tag / 32) % rows;

		bool found = false;
		for (int i = 0; i < size; i++) 
		{
			if (array[index][i] == 32 * (tag / 32))
			{
				res.hits++;
				lru[index][i] = ++time;
				found = true;
				break;
			}
		}
		if (!found)
		{
			res.misses++;
			int value = lru[index][0];
			int loc = 0;
			for (int i = 0; i < size; i++)
			{
				if (lru[index][i] < value) 
				{
					value = lru[index][i];
					loc = i;
				}
			}
			lru[index][loc] = ++time;
			array[index][loc] = 32 * (tag / 32);

			// prefetch new line

			int newindex = (tag / 32 + 1) % rows;
			found = false;
			for (int i = 0; i < size; i++)
			{
				if (array[newindex][i] == 32 * (tag / 32 + 1))
				{
					found = true;
					lru[newindex][i] = ++time;
					break;
				}
			}
			if (!found) 
			{
				int value = lru[newindex][0];
				int loc = 0;
				for (int i = 0; i < size; i++) 
				{
					if (lru[newindex][i] < value)
					{
						value = lru[newindex][i];
						loc = i;
					}
				}
				lru[newindex][loc] = ++time;
				array[newindex][loc] = 32 * ((tag / 32) + 1);
			}
		}
	}
	in.close();
	return res;
}

result extraCredit(int size, string infile) 
{
	result res;
	fstream in(infile);
	int rows = 16*32 / size;
	int array[rows][size];

	// vectors to store the tag and the frequencies
	vector<int> tags;
	vector<int> frequencies;

	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < size; j++)
		{
			array[i][j] = 0; // set initially to 0.
		}
	}
	
	string a,b;
	while (in >> a >> b)
	{
		res.total++;
		int index, tag;
		// This just extracts the tag & calculates the index
		stringstream str;
		str << b;
		str >> hex >> tag;
		index = (tag / 32) % rows;

		bool found = false;
		for (int i = 0; i < size; i++) 
		{
			if (array[index][i] == 32 * (tag / 32))
			{
				// if the tag is already present
				res.hits++;
				found = true;
				for (int j = 0; j < tags.size(); j++)
				{
					if (tags[j] == 32 * (tag / 32))
					{
						// we need to increase the tag's frequency in the vector
						frequencies[j]++;
						break;
					}
				}
				break;
			}
		}
		if (!found)
		{
			// if there's a miss, we need to replace the value
			res.misses++;
			int freq[size];
			for (int i = 0; i < size; i++) freq[i] = 0;

			for (int i = 0; i < tags.size(); i++) 
			{
				for (int j = 0; j < size; j++)
				{
					if (tags[i] == array[index][j])
					{
						// fill "freq" with the corresponding values of frequencies of the tags currently in the set
						freq[j] = frequencies[i];
					}
				}
			}
			int max_index = 0;
			int max_value = array[index][0];
			for (int i = 0; i < size; i++) 
			{
				if (freq[i] < max_value)
				{
					// here we're finding the smallest possible frequency 
					max_index = i;
					max_value = freq[i];
				}
			}
			// replace that smallest frequency with the new value
			array[index][max_index] = 32 * (tag / 32);
			bool found = false;
			for (int i = 0; i < tags.size(); i++)
			{
				if (tags[i] == 32 * (tag / 32)) found = true;
			}
			if (!found) 
			{
				// increase the new value's frequency 
				tags.push_back(32 * (tag / 32));
				frequencies.push_back(1);
			}
		}
	}
	in.close();
	return res;
}

int main (int argc, char ** argv)
{
    ofstream out(argv[2]);
	int setAssociativeCacheSizes[] = {2, 4, 8, 16};
	
	// DIRECT MAPPED CACHE 
	cout << "DIRECT MAPPED CACHE... ";
	int directMappedSizes[] = {1*1024, 4*1024, 16*1024, 32*1024};
	for (int i: directMappedSizes)
	{
		result res = directMappedCache(i, argv[1]);
		out << ((i == 1024) ? "" : " ") << res.hits << "," << res.total << ";";
	}
	out << endl;
	cout << "done." << endl;
	
	// SET ASSOCIATIVE CACHE
	cout << "SET ASSOCIATIVE CACHE... ";
	for (int i: setAssociativeCacheSizes)
	{
		result res = setAssociativeCache(i, argv[1]);
		out << ((i == 2) ? "" : " ") << res.hits << "," << res.total << ";";
	}
	out << endl;
	cout << "done." << endl;
	
	// FULLY ASSOCIATIVE CACHE WITH LRU
	cout << "FULLY ASSOCIATIVE CACHE WITH LRU... ";
	result lruRes = fullyAssociativeCacheLRU(argv[1]);
	out << lruRes.hits << "," << lruRes.total << ";" << endl;
	cout << "done." << endl;

	// FULLY ASSOCIATIVE CACHE WITH HOT-COLD BITS
	cout << "FULLY ASSOCIATIVE CACHE WITH LRU (HOT-COLD) APPROXIMATION... ";
	result bitRes = fullyAssociativeCacheHotCold(argv[1]);
	out << bitRes.hits << "," << bitRes.total << ";" << endl;
	cout << "done." << endl;

	// SET ASSOCIATIVE CACHE WITH NO ALLOCATION ON WRITE MISS
	cout << "SET ASSOCIATIVE CACHE WTIH NO ALLOCATION... ";
	for (int i: setAssociativeCacheSizes)
	{
		result res = setAssociativeCacheNoAllocation(i, argv[1]);
		out << ((i == 2) ? "" : " ") << res.hits << "," << res.total << ";";
	}
	out << endl;
	cout << "done." << endl;

	// SET ASSOCIATIVE CACHE WITH NEXT-LINE PREFETCHING
	cout << "SET ASSOCIATIVE CACHE WITH NEXT-LINE PREFETCHING... ";
	for (int i: setAssociativeCacheSizes)
	{
		result res = setAssociativeCacheNextLinePrefetching(i, argv[1]);
		out << ((i == 2) ? "" : " ") << res.hits << "," << res.total << ";";
	}
	out << endl;
	cout << "done." << endl;

	// PREFETCH ON A MISS
	cout << "PREFETCH ON A MISS... ";
	for (int i: setAssociativeCacheSizes)
	{
		result res = setAssociativeCacheMissPrefetching(i, argv[1]);
		out << ((i == 2) ? "" : " ") << res.hits << "," << res.total << ";";
	}
	out << endl;
	cout << "done." << endl;
	
	// EXTRA CREDIT
	cout << endl;
	cout << "EXTRA CREDIT... (it takes ~90 seconds to finish)" << endl;
	cout << "This implements least-frequently-used (LFU) replacement policy, see README for details." << endl;
	for (int i: setAssociativeCacheSizes)
	{
		result res = extraCredit(i, argv[1]);
		cout << ((i == 2) ? "" : " ") << res.hits << "," << res.total << ";";
	}
	cout << endl;
}