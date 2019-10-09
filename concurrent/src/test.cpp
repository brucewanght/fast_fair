#include "btree.h"

void clear_cache()
{
    // Remove cache
    uint64_t size = 256 * 1024 * 1024;
    char *garbage = new char[size];
    for(uint64_t i = 0; i < size; ++i)
        garbage[i] = i;
    for(uint64_t i = 100; i < size; ++i)
        garbage[i] += garbage[i - 100];
    delete[] garbage;
}

// MAIN
int main(int argc, char** argv)
{
    // Parsing arguments
    uint64_t num_data = 0;
    int n_threads = 1;
    char *input_path = (char *)std::string("../sample_input.txt").data();

    int c;
    while((c = getopt(argc, argv, "n:w:t:i:")) != -1)
    {
        switch(c)
        {
        case 'n':
            num_data = atoi(optarg);
            break;
        case 'w':
            write_latency_in_ns = atol(optarg);
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

    btree *bt;
    bt = new btree();

    struct timespec start, end, tmp;
	long long elapsed_time = 0;

    // Reading data
    entry_key_t* keys = new entry_key_t[num_data];

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

    // Initializing stats
    clflush_cnt = 0;
    search_time_in_insert = 0;
    clflush_time_in_insert = 0;
    gettime_cnt = 0;

    clear_cache();
    // Multithreading
    vector<future<void>> futures(n_threads);

#ifndef MIXED
    uint64_t data_per_thread = num_data / n_threads;
    // Insert
    clock_gettime(CLOCK_MONOTONIC, &start);

    for(int tid = 0; tid < n_threads; tid++)
    {
        uint64_t from = data_per_thread * tid;
        uint64_t to = (tid == n_threads - 1) ? num_data : from + data_per_thread;

        auto f = async(launch::async, [&bt, &keys](uint64_t from, uint64_t to)
        {
            for(uint64_t i = from; i < to; ++i)
			{
                bt->btree_insert(keys[i], (char*) keys[i]);
			}
        }, from, to);
        futures.push_back(move(f));
    }
    for(auto &&f : futures)
        if(f.valid())
            f.get();

    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_time = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
	elapsed_time /= 1000;
	printf("Concurrent inserting with %d threads, elapsed_time (usec): %lld, Avg: %f\n", 
			n_threads, elapsed_time, (double)elapsed_time / num_data);	
    clear_cache();
    futures.clear();

    // Search
    clock_gettime(CLOCK_MONOTONIC, &start);

    for(int tid = 0; tid < n_threads; tid++)
    {
        uint64_t from = data_per_thread * tid;
        uint64_t to = (tid == n_threads - 1) ? num_data : from + data_per_thread;

        auto f = async(launch::async, [&bt, &keys](uint64_t from, uint64_t to)
        {
            for(uint64_t i = from; i < to; ++i)
			{
                bt->btree_search(keys[i]);
			}
        }, from, to);
        futures.push_back(move(f));
    }
    for(auto &&f : futures)
        if(f.valid())
            f.get();

    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_time = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
	elapsed_time /= 1000;
	printf("Concurrent searching with %d threads, elapsed_time (usec): %lld, Avg: %f\n", 
			n_threads, elapsed_time, (double)elapsed_time / num_data);	
	//bt->printAll();
    clear_cache();
    futures.clear();

    // Delete
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(int tid = 0; tid < n_threads; tid++)
    {
        uint64_t from = data_per_thread * tid;
        uint64_t to = (tid == n_threads - 1) ? num_data : from + data_per_thread;

        auto f = async(launch::async, [&bt, &keys](int from, int to)
        {
            for(uint64_t i = from; i < to; ++i)
			{
			    //printf_warn("delete index = %lu, key = %lu ... begin", i, keys[i]);
                bt->btree_delete(keys[i]);
			    //printf_warn("delete index = %lu, key = %lu ... OK", i, keys[i]);
			}
        }, from, to);
        futures.push_back(move(f));
    }
    for(auto &&f : futures)
        if(f.valid())
            f.get();

    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_time = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
	elapsed_time /= 1000;
	printf("Concurrent deleting with %d threads, elapsed_time (usec): %lld, Avg: %f\n",
			n_threads, elapsed_time, (double)elapsed_time / num_data);	
	//bt->printAll();

#else
	long half_num_data = num_data / 2;
    long data_per_thread = num_data / n_threads;
    for(int tid = 0; tid < n_threads; tid++)
    {
        uint64_t from = half_num_data + data_per_thread * tid;
        uint64_t to = (tid == n_threads - 1) ? num_data : from + data_per_thread;

        auto f = async(launch::async, [&bt, &keys, &half_num_data](int from, int to)
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
    for(auto &&f : futures)
        if(f.valid())
            f.get();
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_time = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
	elapsed_time /= 1000;
	printf("Concurrent inserting/searching/deleting with %d threads, elapsed_time (usec): %lld, Avg: %f\n",
			n_threads, elapsed_time, (double)elapsed_time / num_data);	
#endif

    futures.clear();
    delete bt;
    delete[] keys;

    return 0;
}
