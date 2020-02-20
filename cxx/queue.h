#pragma once

#include <inttypes.h>
#include <list>

struct queue_item
{
    int8_t type;
    uint8_t data [256];
};

class queue
{
public:
    queue() = default;
    virtual ~queue() = default;
    void push(const queue_item& event);
    bool pop(queue_item& event);
    void clear();
private:
    std::list<queue_item>queue_;
};