#include "classes.h"

using namespace std;

mutex mt_tasks; 
mutex mt_logs;
mutex mt_workers;
condition_variable cv;
bool broker_has_tasks = false;

Thread::Thread() {}
Thread::~Thread() { m_terminate=true; }

Queue::Queue() {}
Queue::~Queue() {}

task_t Queue::pop_task()
{
    lock_guard<mutex> lk(mt_tasks);
    auto item = m_tasks.front();
    m_tasks.pop_front();
    return item;
}

void Queue::submit_task(task_t task_obj)
{
    lock_guard<mutex> lk(mt_tasks);
    m_tasks.push_back(task_obj);
}

Logger::Logger() {}
Logger::~Logger() {}

return_t Logger::pop_log()
{
    lock_guard<mutex> lk(mt_logs);
    auto item = m_logs.front();
    m_logs.pop_front();
    return item;
}

void Logger::submit_log(return_t data)
{
    lock_guard<mutex> lk(mt_logs);
    m_logs.push_back(data);
}

Worker::Worker() : m_limit(10)
{
    auto tr = thread(&Worker::exec, this);
    m_thread = &(tr);
    m_thread->detach();
    m_terminate = false;
}

Worker::~Worker() { broker=nullptr; }

void Worker::exec()
{
    while (true) 
    {
        if (m_terminate) return;
        unique_lock lk(mt_workers);
        worker_has_tasks = m_tasks.size() > 0;
        worker_has_slots = m_tasks.size() < m_limit;
        cv.wait(lk, [&]() {
            return (broker_has_tasks || worker_has_tasks || m_terminate);
        });
        if (worker_has_tasks) do_task();
        get_tasks();
        lk.unlock();
    }
}

void Worker::get_tasks()
{
    while (broker_has_tasks && worker_has_slots) 
    {
        if (broker == nullptr) return;
        if (broker->m_tasks.size() == 0) 
        {
            broker_has_tasks = false;
            cv.notify_all();
            break;
        }
        auto item = broker->pop_task();
        submit_task(item);
        worker_has_slots = m_tasks.size() < m_limit;
    }
}

void Worker::do_task()
{
    if (broker == nullptr) return;
    auto item = pop_task(); 
    lambda_t func; input_t data; offset_t offset;
    tie(func, data, offset) = item;
    return_t res = func(data, offset);
    broker->submit_log(res);
    worker_has_slots = m_tasks.size() < m_limit;
}

void Worker::_link_broker(Broker *br) 
{
    broker = br; 
}

Broker::Broker()
{
    auto tr = thread(&Broker::exec, this);
    m_thread = &(tr);
    m_thread->detach();
    m_terminate = false;
}

Broker::~Broker() {}

void Broker::exec()
{
    while (true) 
    {
        if (m_terminate) return;
        if (m_logs.size() > 0) report();
        lock_guard<mutex> lk(mt_workers);
        broker_has_tasks = (m_tasks.size() > 0);
        cv.notify_all();
    }
}

void Broker::report()
{
    int type; input_t data; offset_t offset;
    tie(type, data, offset) = pop_log();
    if (type == LambdaType::second) 
    {
        for (int i=0; i<data.size(); i++) 
        {
            if (data[i] == 0) printf("Compare with Buffer1 at %p :: OK\n", (void*)(offset+i));
            if (data[i] == 1) printf("Compare with Buffer1 at %p :: FAIL\n", (void*)(offset+i));
        }
    }
    if (type == LambdaType::first) 
    {
        for (int i=0; i<data.size(); i++) 
        {
            printf("Write to Buffer2 at %p\n", (void*)(offset+i));
        }
    }
    m_tasks_completed++;
    printf("Tasks completed: %d / %d\n", 
        m_tasks_completed, m_tasks_expected);
}

void Broker::link_worker(Worker *worker)
{
    workers.push_back(worker);
    worker->_link_broker(this);
}

void Broker::reset()
{
    m_tasks_completed = 0;
    m_tasks.clear();
    m_logs.clear();
}
