#include "libslb.h"
#include "btree.h"
#include <random>
#include <cassert>
using namespace std;

void clear_cache(uint64_t size)
{
    // Remove cache
    char* garbage = new char[size];
    for(uint64_t i = 0; i < size; ++i)
        garbage[i] = i;
    for(uint64_t i = 100; i < size; ++i)
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

// MAIN
int main(int argc, char** argv)
{
    // Parsing arguments
    uint64_t num_data = 0;
    int n_threads = 1;
    uint64_t cache_mb = 0;

    float selection_ratio = 0.0f;
    uint64_t hit = 0;
    bool found = false;
    bool has_bf = false;
    struct rcache* cache = nullptr;
    struct rgen* gen = nullptr;
    struct bloomfilter* bf = nullptr;

    char* input_path = (char*)std::string("../sample_input.txt").data();

    int c;
    while((c = getopt(argc, argv, "n:w:c:bt:i:")) != -1)
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
        case 'b':
            bf = bf_create(4, num_data);
            break;
        case 't':
            n_threads = atoi(optarg);
            break;
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
        cache = (struct rcache*)rcache_create(cache_mb, match_u64, hash_u64, hash_u64);
        debug_assert(cache);
    }
    gen = rgen_new_unizipf(0, num_data - 1, 1024);
    debug_assert(gen);

    struct timespec start, end, tmp;
    long long elapsed_time = 0;

    // Reading data
    static entry_key_t* keys = new entry_key_t[num_data];

    ifstream ifs;
    ifs.open(input_path);

    if(!ifs)
    {
        cout << "input loading error!" << endl;
    }

    for(uint64_t i = 0; i < num_data; ++i)
    {
        ifs >> keys[i];
    }
    ifs.close();

    uint64_t data_per_thread = num_data / n_threads;

    // Multithreading
    vector<future<void>> futures(n_threads);

#ifndef MIXED
    clear_cache(num_data * 16);
    {
        // Insert
        clock_gettime(CLOCK_MONOTONIC, &start);

        for(int tid = 0; tid < n_threads; tid++)
        {
            uint64_t from = data_per_thread * tid;
            uint64_t to = (tid == n_threads - 1) ? num_data : from + data_per_thread;

            auto f = async(launch::async, [&bt, bf](uint64_t from, uint64_t to)
            {
                entry_key_t* pvalue = nullptr;
                for(uint64_t i = from; i < to; ++i)
                {
                    //printf_info("thread %lx, insert index = %lu, key = %lu ... begin", pthread_self(), i, keys[i]);
                    pvalue = &(keys[i]);
                    bt->btree_insert(keys[i], (char*)pvalue);
                    if(bf)
                        bf_mark(bf, keys[i]);
                    //printf_info("thread %lx, insert index = %lu, key = %lu ... OK", pthread_self(), i, keys[i]);
                }
            }, from, to);
            futures.push_back(move(f));
        }
        for(auto&& f : futures)
            if(f.valid())
                f.get();

        clock_gettime(CLOCK_MONOTONIC, &end);
        elapsed_time = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
        elapsed_time /= 1000;
        printf("Concurrent inserting with %d threads, elapsed_time (usec): %lld, Avg: %f\n",
               n_threads, elapsed_time, (double)elapsed_time / num_data);
        futures.clear();
    }

    clear_cache(num_data * 16);
    {
        // Search
        clock_gettime(CLOCK_MONOTONIC, &start);

        for(int tid = 0; tid < n_threads; tid++)
        {
            uint64_t from = data_per_thread * tid;
            uint64_t to = (tid == n_threads - 1) ? num_data : from + data_per_thread;

            auto f = async(launch::async, [&bt, bf, cache, &hit](uint64_t from, uint64_t to)
            {
                entry_key_t* pkey = nullptr;
                entry_key_t* pvalue = nullptr;
                for(uint64_t i = from; i < to; ++i)
                {
                    //printf_info("Thread %lx, SEARCH: key[%lu] = %lu ... count = %lu", pthread_self(), i, keys[i], i + 1);
                    if(bf && !bf_test(bf, keys[i]))
                    {

                        /*
                        printf_info("Thread %lx, SEARCH: Bloom Filter NOT FOUND key[%lu] = %lu ... count = %lu",
                        		pthread_self(), i, keys[i], i + 1);
                        */
                        continue;
                    }

                    if(cache)
                    {
                        pkey = &(keys[i]);
                        pvalue = (entry_key_t*)rcache_get(cache, pkey);
                        if (pvalue)
                        {
                            ++hit;
                            /*
                            printf_info("Thread %lx, SEARCH: HIT key[%lu] = %lu, value = %p ... count = %lu",
                            		pthread_self(), i, keys[i], pvalue, i + 1);
                            */
                        }
                        else
                        {
                            pvalue = (entry_key_t*)(bt->btree_search(keys[i]));
                            if(pvalue)
                            {
                                /*
                                printf_info("Thread %lx, SEARCH: MISS key[%lu] = %lu, value = %p ... count = %lu",
                                		pthread_self(), i, keys[i], pvalue, i + 1);
                                */
                                rcache_hint(cache, pvalue);
                            }
                            /*
                            else
                            {
                            	printf_info("Thread %lx, SEARCH: NOT FOUND key[%lu] = %lu ... count = %lu",
                            			pthread_self(), i, keys[i], i + 1);
                            }
                            */
                        }
                    }
                    else
                    {
                        pvalue = (entry_key_t*)bt->btree_search(keys[i]);
                        //printf_info("Thread %lx, key %lu, value %p, ori_value %p", pthread_self(), keys[i], pvalue, (char*)keys[i]);
                        //assert(pvalue == (entry_key_t*)keys[i]);
                    }
                }
            }, from, to);
            futures.push_back(move(f));
        }
        for(auto&& f : futures)
            if(f.valid())
                f.get();

        clock_gettime(CLOCK_MONOTONIC, &end);
        elapsed_time = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
        elapsed_time /= 1000;
        printf("Concurrent searching with %d threads, elapsed_time (usec): %lld, Avg: %f\n",
               n_threads, elapsed_time, (double)elapsed_time / num_data);
        if (cache)
            printf("SLB get hit %" PRIu64 "/%" PRIu64 ", hit ratio = %lf\n", hit, num_data, (double)hit / num_data);
        //bt->print_tree();
        futures.clear();
    }

    clear_cache(num_data * 16);
    {
        // Delete
        clock_gettime(CLOCK_MONOTONIC, &start);
        for(int tid = 0; tid < n_threads; tid++)
        {
            uint64_t from = data_per_thread * tid;
            uint64_t to = (tid == n_threads - 1) ? num_data : from + data_per_thread;

            auto f = async(launch::async, [&bt, &bf](int from, int to)
            {
                for(uint64_t i = from; i < to; ++i)
                {
                    //printf_info("Thread %lx, delete index = %lu, key = %lu ... begin", pthread_self(), i, keys[i]);
                    if(bf && !bf_test(bf, keys[i]))
                    {
                        //printf_info("DELETE: Bloom Filter DELETE key[%lu] = %lu, NOT FOUND ... count = %lu", i, keys[i], i + 1);
                        continue;
                    }
                    bt->btree_delete(keys[i]);
                    //printf_info("Thread %lx, delete index = %lu, key = %lu ... OK", pthread_self(), i, keys[i]);
                }
            }, from, to);
            futures.push_back(move(f));
        }
        for(auto&& f : futures)
            if(f.valid())
                f.get();

        clock_gettime(CLOCK_MONOTONIC, &end);
        elapsed_time = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
        elapsed_time /= 1000;
        printf("Concurrent deleting with %d threads, elapsed_time (usec): %lld, Avg: %f\n",
               n_threads, elapsed_time, (double)elapsed_time / num_data);
        //bt->print_tree();
        bt->print_stat();
        futures.clear();
    }

#else
    long half_num_data = num_data / 2;
    for(int tid = 0; tid < n_threads; tid++)
    {
        uint64_t from = half_num_data + data_per_thread * tid;
        uint64_t to = (tid == n_threads - 1) ? num_data : from + data_per_thread;

        auto f = async(launch::async, [&bt, &half_num_data](int from, int to)
        {
            for(uint64_t i = from; i < to; ++i)
            {
                uint64_t sidx = i - half_num_data;

                uint64_t jid = i % 4;
                switch(jid)
                {
                case 0:
                    bt->btree_insert(keys[i], (char*) keys[i]);
                    for(uint64_t j = 0; j < 4; j++)
                        bt->btree_search(keys[(sidx + j + jid * 8) % half_num_data]);
                    bt->btree_delete(keys[i]);
                    break;
                case 1:
                    for(uint64_t j = 0; j < 3; j++)
                        bt->btree_search(keys[(sidx + j + jid * 8) % half_num_data]);
                    bt->btree_insert(keys[i], (char*) keys[i]);
                    bt->btree_search(keys[(sidx + 3 + jid * 8) % half_num_data]);
                    break;
                case 2:
                    for(uint64_t j = 0; j < 2; j++)
                        bt->btree_search(keys[(sidx + j + jid * 8) % half_num_data]);
                    bt->btree_insert(keys[i], (char*) keys[i]);
                    for(uint64_t j = 2; j < 4; j++)
                        bt->btree_search(keys[(sidx + j + jid * 8) % half_num_data]);
                    break;
                case 3:
                    for(uint64_t j = 0; j < 4; j++)
                        bt->btree_search(keys[(sidx + j + jid * 8) % half_num_data]);
                    bt->btree_insert(keys[i], (char*) keys[i]);
                    break;
                default:
                    break;
                }
            }
        }, from, to);
        futures.push_back(move(f));

    }
    for(auto&& f : futures)
        if(f.valid())
            f.get();
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_time = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
    elapsed_time /= 1000;
    printf("Concurrent inserting/searching/deleting with %d threads, elapsed_time (usec): %lld, Avg: %f\n",
           n_threads, elapsed_time, (double)elapsed_time / num_data);
    futures.clear();
#endif

    if(bf)
        bf_destroy(bf);
    delete bt;
    delete[] keys;

    return 0;
}
