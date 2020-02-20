#include "queue.h"

void queue::push(const queue_item &event)
{
    if(queue_.size() > 100)
    {
        return;
    }

    queue_.push_back(event);
}

bool queue::pop(queue_item &event)
{
    for(auto it = queue_.begin(); it != queue_.end(); ++it)
    {
        event = *it;
        queue_.erase(it);
        return true;
    }

    return false;
}

void queue::clear()
{
    queue_.clear();
}