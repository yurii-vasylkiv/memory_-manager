#include <cstdio>
#include "queue.h"

void queue::push(const queue_item &event)
{
    std::unique_lock<std::mutex> lck(mtx);
    while (!ready) cv.wait(lck);
    queue_.push_back(event);
}

bool queue::pop(queue_item &event)
{
    std::unique_lock<std::mutex> lck(mtx);
    for(auto it = queue_.begin(); it != queue_.end(); ++it)
    {
        event = *it;
        queue_.erase(it);
        return true;
    }
    ready = true;
    cv.notify_all();
    return false;
}

void queue::clear()
{
    queue_.clear();
}