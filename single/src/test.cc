#include "libslb.h"
#include "btree.h"
#include <random>

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
    entry_key_t i = 0;
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

    /*
        random_device rd;  //Will be used to obtain a seed for the random number engine
        mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        uniform_int_distribution<uint64_t> randint(0,  ULLONG_MAX - 1);
    */

    if(cache_mb > 0)
    {
        cache = (typeof(cache))rcache_create(cache_mb, match_u64, hash_u64, hash_u64);
        debug_assert(cache);
        gen = rgen_new_unizipf(0, num_data - 1, 1024);
        debug_assert(gen);
    }

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
            //convert entry_key_t numbers array elements to char pointer as values
            bt->btree_insert(keys[i], (char*)(&(keys[i])));
            /*
            printf("debug %s, line %d: ----------------------- insert key = %lu ... count = %d\n",
            		__FUNCTION__, __LINE__, keys[i], i + 1);
            */
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
		register entry_key_t id = 0;
		uint64_t hit = 0;
		entry_key_t* pkey = nullptr;
		entry_key_t* pvalue = nullptr; 

        clock_gettime(CLOCK_MONOTONIC, &start);
        for(i = 0; i < num_data; ++i)
        {
            //j = randint(gen)%num_data;
            id = rgen_next_wait(gen);
            if(cache)
            {
                pkey = &(keys[id]);
                pvalue = (entry_key_t*)rcache_get(cache, pkey);
                debug_assert((pvalue == NULL) || (pvalue == pkey));
                if (pvalue)
                {
                    hit++;
                    /*
                    printf("debug %s, line %d: ----------------------- hit key[%lu] = %lu, value = %lu ... count = %lu\n",
                    		__FUNCTION__, __LINE__, id, keys[id], (entry_key_t)pvalue, i + 1);
                    */
                }
                else
                {
                    pvalue = (entry_key_t*)(bt->btree_search(keys[id]));
                    if(pvalue)
                    {
                        /*
                        printf("debug %s, line %d: ----------------------- miss key[%lu] = %lu, value = %lu ... count = %lu\n",
                        		__FUNCTION__, __LINE__, id, keys[id], *pvalue, i + 1);
                        */
                        rcache_hint(cache, (const void*)pvalue);
                    }
                }
            }
            else
            {
                pvalue = (entry_key_t*)(bt->btree_search(keys[id]));
                if(pvalue)
                {
                    /*
                    printf("debug %s, line %d: ----------------------- miss key[%lu] = %lu, value = %lu ... count = %lu\n",
                    		__FUNCTION__, __LINE__, id, keys[id], *pvalue, i + 1);
                    */
                }
            }
        }
        clock_gettime(CLOCK_MONOTONIC, &end);

        long long elapsed_time = (end.tv_sec - start.tv_sec) * 1000000000
                                 + (end.tv_nsec - start.tv_nsec);
        elapsed_time /= 1000;
        printf("SEARCH elapsed_time: %lld, Avg: %f\n", elapsed_time,
               (double)elapsed_time / num_data);
        printf("SLB get hit %" PRIu64 "/%" PRIu64 ", hit ratio = %lf\n", hit, num_data, (double)hit / num_data);
        //bt->print_tree();
    }

    clear_cache();
    {
        clock_gettime(CLOCK_MONOTONIC, &start);
        for(i = 0; i < num_data; ++i)
        {
            bt->btree_delete(keys[i]);
            /*
            printf("debug %s, line %d: ----------------------- key = %lu, delete OK ... count = %lu\n",
            		__FUNCTION__, __LINE__, keys[i], i + 1);
            */
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
