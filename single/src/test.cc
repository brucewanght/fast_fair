#include "libslb.h"
#include "btree.h"
#include "debug.h"
#include <random>
using namespace std;

void clear_cache()
{
    // Remove cache
    int size = 1024 * 1024 * 1024;
    char* garbage = new char[size];

    for(int i = 0; i < size; ++i)
        garbage[i] = i;

    for(int i = 100; i < size; ++i)
        garbage[i] += garbage[i - 100];

    delete[] garbage;
}

static bool match_u64(const void* const p1, const void* const p2)
{
    return memcmp(p1, p2, sizeof(u64)) ? false : true;
}

static u64 hash_u64(const void* const p)
{
    return xxhash64(p, sizeof(u64));
}

int main(int argc, char** argv)
{
    //parsing arguments
    uint64_t num_data = 0;
    register entry_key_t i = 0;
	register entry_key_t id = 0;
	entry_key_t* pkey = nullptr;
	entry_key_t* pvalue = nullptr; 

	uint64_t hit = 0;
    int n_threads = 1;
    uint64_t cache_mb = 0;
    float selection_ratio = 0.0f;
    char* input_path = (char*)std::string("../sample_input.txt").data();
    struct rcache* cache = nullptr;
    struct rgen* gen = nullptr;
    int c;

    while((c = getopt(argc, argv, "n:w:c:t:s:i:")) != -1)
    {
        switch(c)
        {
        case 'n':
            num_data = atoll(optarg);
            break;
        case 'w':
            write_latency_in_ns = atol(optarg);
            break;
        case 'c':
            cache_mb = atoll(optarg);
            break;
        case 't':
            n_threads = atoi(optarg);
            break;
        case 's':
            selection_ratio = atof(optarg);
        case 'i':
            input_path = optarg;
        default:
            break;
        }
    }

    btree* bt;
    bt = new btree();

    if(cache_mb > 0)
    {
        cache = (typeof(cache))rcache_create(cache_mb, match_u64, hash_u64, hash_u64);
        debug_assert(cache);
    }
    gen = rgen_new_unizipf(0, num_data - 1, 1024);
    debug_assert(gen);

    struct timespec start, end;

    //reading data
    entry_key_t* keys = new entry_key_t[num_data];
    ifstream ifs;
    ifs.open(input_path);

    if(!ifs)
    {
        cout << "input loading error!" << endl;
        delete[] keys;
        exit(-1);
    }

    for(i = 0; i < num_data; ++i)
        ifs >> keys[i];

    ifs.close();
    {
        clock_gettime(CLOCK_MONOTONIC, &start);
        for(i = 0; i < num_data; ++i)
        {
			//NOTE: we use pointer of data as key and value in btree
            id = rgen_next_wait(gen);
            bt->btree_insert(keys[id], (char*)(keys + id));
        }
        clock_gettime(CLOCK_MONOTONIC, &end);

        long long elapsed_time = (end.tv_sec - start.tv_sec) * 1000000000
                                 + (end.tv_nsec - start.tv_nsec);
        elapsed_time /= 1000;
        printf("INSERT elapsed_time: %lld, Avg: %f\n", elapsed_time,
               (double)elapsed_time / num_data);
    }

    clear_cache();
    {
        clock_gettime(CLOCK_MONOTONIC, &start);
        for(i = 0; i < num_data; ++i)
        {
            id = rgen_next_wait(gen);
            if(cache)
            {
                pkey = &(keys[id]);
                pvalue = (entry_key_t*)rcache_get(cache, pkey);
                if (pvalue)
                {
					++hit;
					//printf_info("HIT key[%lu] = %lu, value = %lu ... count = %lu", id, keys[id], *pvalue, i + 1);
                }
				else
                {
                    pvalue = (entry_key_t*)(bt->btree_search(keys[id]));
                    //printf_info("MISS key[%lu] = %lu, value = %lu ... count = %lu", id, keys[id], *pvalue, i + 1);
					if(pvalue)
						rcache_hint(cache, pvalue);
                }
            }
			else
            {
                pvalue = (entry_key_t*)(bt->btree_search(keys[id]));
				//printf_info("FOUND key[%lu] = %lu, value = %lu ... count = %lu", id, keys[id], *pvalue, i + 1);
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &end);

        long long elapsed_time = (end.tv_sec - start.tv_sec) * 1000000000
                                 + (end.tv_nsec - start.tv_nsec);
        elapsed_time /= 1000;
        printf("SEARCH elapsed_time: %lld, Avg: %f\n", elapsed_time,
               (double)elapsed_time / num_data);
		if (cache)
			printf("SLB get hit %" PRIu64 "/%" PRIu64 ", hit ratio = %lf\n", hit, num_data, (double)hit / num_data);
        //bt->print_tree();
    }

    clear_cache();
    {
        clock_gettime(CLOCK_MONOTONIC, &start);
        for(i = 0; i < num_data; ++i)
        {
			printf_info("DELETE key[%lu] = %lu, ... count = %lu", i, keys[i], i + 1);
            pvalue = (entry_key_t*)(bt->btree_delete(keys[i]));
			if(pvalue)
			{
				printf_info("DELETE key[%lu] = %lu, value = %lu ... count = %lu --------------------- OK", 
						i, keys[i], *pvalue, i + 1);
			}
			else
				printf_info("NOT FOUND key[%lu] = %lu, ... count = %lu", i, keys[i], i + 1);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        long long elapsed_time = (end.tv_sec - start.tv_sec) * 1000000000
                                 + (end.tv_nsec - start.tv_nsec);
        elapsed_time /= 1000;
        printf("DELETE elapsed_time: %lld, Avg: %f\n", elapsed_time,
               (double)elapsed_time / num_data);
        //bt->print_tree();
    }

    delete bt;
    delete[] keys;

    return 0;
}
