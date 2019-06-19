#include "btree.h"

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

int main(int argc, char** argv)
{
    //parsing arguments
    int num_data = 0;
    int n_threads = 1;
    float selection_ratio = 0.0f;
    char* input_path = (char*)std::string("../sample_input.txt").data();
    int c;

    while((c = getopt(argc, argv, "n:w:t:s:i:")) != -1)
    {
        switch(c)
        {
        case 'n':
            num_data = atoll(optarg);
            break;
        case 'w':
            write_latency_in_ns = atol(optarg);
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
	entry_key_t i = 0;

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
            bt->btree_insert(keys[i], (char*)keys[i]);
			/*
            printf("debug %s, line %d: ----------------------- insert key = %lu ... count = %lu\n",
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
        clock_gettime(CLOCK_MONOTONIC, &start);
        for(i = 0; i < num_data; ++i)
        {
            bt->btree_search(keys[i]);
			/*
            printf("debug %s, line %d: ----------------------- search key = %lu ... count = %lu\n",
            		__FUNCTION__, __LINE__, keys[i], i + 1);
			*/
        }
        clock_gettime(CLOCK_MONOTONIC, &end);

        long long elapsed_time = (end.tv_sec - start.tv_sec) * 1000000000
                                 + (end.tv_nsec - start.tv_nsec);
        elapsed_time /= 1000;
        printf("SEARCH elapsed_time: %lld, Avg: %f\n", elapsed_time,
               (double)elapsed_time / num_data);
        //bt->printAll();
    }

    clear_cache();
    {
        clock_gettime(CLOCK_MONOTONIC, &start);
        for(i = 0; i < num_data; ++i)
        {
			/*
            printf("debug %s, line %d: ----------------------- key = %lu, delete begin ... count = %lu\n",
            		__FUNCTION__, __LINE__, keys[i], i + 1);
			*/
            bt->btree_delete(keys[i]);
			/*
            printf("debug %s, line %d: ----------------------- key = %lu, delete OK ...... count = %lu\n",
            		__FUNCTION__, __LINE__, keys[i], i + 1);
			*/
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        long long elapsed_time = (end.tv_sec - start.tv_sec) * 1000000000
                                 + (end.tv_nsec - start.tv_nsec);
        elapsed_time /= 1000;
        printf("DELETE elapsed_time: %lld, Avg: %f\n", elapsed_time,
               (double)elapsed_time / num_data);
        //bt->printAll();
    }

    delete bt;
    delete[] keys;

    return 0;
}
