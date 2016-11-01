//
// Created by chris on 9/26/16.
//

#include "XMessageQueue.h"
#include "XLog.h"

#define TAG "UTIL_XMessageQueue"

XMessageQueue::XMessageQueue()
{
    // call init function
    init();
}


XMessageQueue::XMessageQueue(char *name)
{
    this->msg_queue_name = name;    // set message queue name .

    // call init function
    init();
}

XMessageQueue::~XMessageQueue()
{
    this->msg_queue.clear();
}

int XMessageQueue::init()
{

    this->node_count_current = 0;
    this->node_count_max = 0xFFFF;

    pthread_mutex_init(&this->mutexLock ,NULL);
    pthread_cond_init(&has_node, NULL);
    pthread_cond_init(&has_space, NULL);
    return 0;
}

int XMessageQueue::push(player_event_e evt_type)
{
    pthread_mutex_lock(&this->mutexLock);

    if(this->msg_queue.size() >= this->node_count_max){ // full
        pthread_cond_wait(&this->has_space, &this->mutexLock);
    }

    // add message at the tail.
    this->msg_queue.push_back(evt_type);
    // alter node_count
    this->node_count_current ++;

    pthread_mutex_unlock(&this->mutexLock);
    pthread_cond_signal(&this->has_node);

    return 0;
}


int XMessageQueue::push_front(player_event_e evt_type)
{
    pthread_mutex_lock(&this->mutexLock);

    if(this->msg_queue.size() >= this->node_count_max){ // full
        pthread_cond_wait(&this->has_space, &this->mutexLock);
    }

    // add message at the header.
    this->msg_queue.push_front(evt_type);
    // alter node_count
    this->node_count_current ++;

    pthread_mutex_unlock(&this->mutexLock);
    pthread_cond_signal(&this->has_node);

    return 0;
}

#if 0
int XMessageQueue::push(ipc_msg_node_t *msg_node)
{
    pthread_mutex_lock(this->mutexLock);

    if(this->msg_queue.size() >= this->node_count_max){ // full
        pthread_cond_wait(&this->has_space, this->mutexLock);
    }

    // add message at the tail.
    this->msg_queue.push_back(msg_node);
    // alter node_count
    this->node_count_current ++;

    pthread_mutex_unlock(this->mutexLock);
    pthread_cond_signal(&this->has_node);

    return 0;
}
#endif


player_event_e XMessageQueue::pop()
{
    pthread_mutex_lock(&this->mutexLock);

    if(this->msg_queue.size() == 0){ // empty
        pthread_cond_wait(&this->has_node, &this->mutexLock);
    }

    // pop a message node
    player_event_e evt_type = this->msg_queue.front();
    this->msg_queue.pop_front();
    // alter node count
    this->node_count_current --;

    pthread_mutex_unlock(&this->mutexLock);
    pthread_cond_signal(&this->has_space);
    return evt_type;
}


bool XMessageQueue::empty()
{
    return (this->node_count_current == 0);
}


int XMessageQueue::size()
{
    return this->node_count_current;
}