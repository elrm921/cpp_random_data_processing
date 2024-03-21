#include <iostream>
#include <cstring>
#include <thread>
#include <vector>
#include <queue>
#include <random>
#include <bitset>
#include "classes.h"

using namespace std;

lambda_t lambda_1 = [](vector<uint16_t> data, offset_t offset) 
{
    vector<uint16_t> res;
    for (int i=0; i<data.size(); i++) {
        uint16_t value = data[i]^0xF;
        *(offset+i) = value;
        res.push_back(value);
    }
    return tuple(LambdaType::first,res,offset);
};

lambda_t lambda_2 = [](vector<uint16_t> data, offset_t offset) 
{
    vector<uint16_t> res;
    for (int i=0; i<data.size(); i++) {
        uint16_t value = data[i]^0xF;
        if (value == *(offset+i))
            res.push_back(0);
        else 
            res.push_back(1);
    }
    return tuple(LambdaType::second,res,offset);
};

int main(int argc, char **argv)
{
    int treads = 4;
    int buffer_size_b = 102400;
    int chunk_size_b = 256;

    for (int i = 0; i < argc; i++) 
    {
        if (argc % 2 != 1) return 1;
        if (strcmp(argv[i], "-t") == 0) treads = stoi(argv[i+1]);
        if (strcmp(argv[i], "-s") == 0) buffer_size_b = stoi(argv[i+1]);
    } 

    Broker broker;
    if (treads < 1) treads = 1;
    vector<Worker> pool(treads);
    fill(pool.begin(), pool.end(), Worker());
    for_each(pool.begin(), pool.end(), [&](Worker &wk) {broker.link_worker(&wk);});

    if (buffer_size_b > 104857600) buffer_size_b = 104857600;
    if (buffer_size_b < 1024) buffer_size_b = 1024;

    int buffer_size = buffer_size_b / 16;  
    int chunk_size = chunk_size_b / 16; 
    broker.m_tasks_expected = buffer_size / chunk_size;

    printf("Treads: %d\n", treads);
    printf("Buffer: %d\n", buffer_size);
    printf("Chunks: %d\n", buffer_size / chunk_size);

    vector<uint16_t> buffer1(buffer_size);
    vector<uint16_t> buffer2(buffer_size);

    default_random_engine generator;
    uniform_int_distribution<uint16_t> distribution(0,65535);
    for_each(buffer1.begin(), buffer1.end(), [&](uint16_t &n) {n=distribution(generator);});

    vector<uint16_t> chunk;
    uint16_t* offset;

    broker.reset();
    printf("Reading data from buffer1... \n");
    auto it1 = buffer1.begin();
    auto it2 = buffer2.begin();
    while (it1 != buffer1.end()) 
    {
        if (chunk.size() == 0) 
        {
            offset = &(*it2);
        }
        chunk.push_back(*it1);
        if (chunk.size() == 16) 
        {
            broker.submit_task(tuple(lambda_1, chunk, offset));
            chunk.clear();
        }
        it1++; it2++;
    }

    while (broker.m_tasks_completed < broker.m_tasks_expected) 
    {
        this_thread::sleep_for(200ms);
    }
    printf("First pass completed! \n");

    broker.reset();
    printf("Reading data from buffer2... \n");
    it1 = buffer1.begin();
    it2 = buffer2.begin();
    while (it2 != buffer2.end()) 
    {
        if (chunk.size() == 0) 
        {
            offset = &(*it1);
        }
        chunk.push_back(*it2);
        if (chunk.size() == 16) 
        {
            broker.submit_task(tuple(lambda_2, chunk, offset));
            chunk.clear();
        }
        it1++; it2++;
    }

    while (broker.m_tasks_completed < broker.m_tasks_expected) 
    {
        this_thread::sleep_for(200ms);
    }
    printf("Second pass completed! \n");

    return 0;
}
