#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <condition_variable>

using namespace std;

typedef uint16_t* offset_t;
typedef vector<uint16_t> input_t;
typedef tuple<int,input_t,offset_t> return_t;
typedef function<return_t(input_t,offset_t)> lambda_t;
typedef tuple<lambda_t,input_t,offset_t> task_t;

enum BrokerState {first_pass, second_pass, completed};
enum LambdaType {first, second};

class Thread;
class Queue;
class Worker;
class Broker;
class Logger;

class Thread
{
public:
    thread *m_thread;
    bool m_terminate;
    Thread();
    ~Thread();
};

class Queue
{
public:
    deque<task_t> m_tasks;
    Queue();
    ~Queue();
    task_t pop_task();
    void submit_task(task_t task_obj);

};

class Logger
{
public:
    deque<return_t> m_logs;
    Logger();
    ~Logger();
    return_t pop_log();
    void submit_log(return_t log_obj);
};

class Worker : public Thread, public Queue
{
public:
    Broker* broker;
    bool worker_has_tasks;
    bool worker_has_slots;
    int m_limit;
    Worker();
    ~Worker();
    void exec();
    void do_task();
    void get_tasks();
    void _link_broker(Broker* broker);
};

class Broker : public Thread, public Queue, public Logger
{
public:
    vector<Worker*> workers;
    int m_tasks_completed;
    int m_tasks_expected;
    Broker();
    ~Broker();
    void exec();
    void report();
    void reset();
    void link_worker(Worker* worker);
};
