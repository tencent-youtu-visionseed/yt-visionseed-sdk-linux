//author: chenliang @ Youtu Lab, Tencent

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <memory>
#include <pthread.h>
#include <sys/time.h>

#include "SimplePool.h"
#include "YtLog.h"

#include "YtMsg.pb.h"
template class SimplePool<YtMsg>;

#ifdef __rtems__
#include "cv.h"
template class SimplePool<HwImageExt>;
#endif

template <class T>
SimplePool<T> *SimplePool<T>::instance = NULL;

template <class T>
SimplePool<T>::SimplePool(int size, int _limit) : limit(_limit)
{
#ifdef INC_FREERTOS_H
    mSem = xSemaphoreCreateCounting(10, 1);
#else
    if (sem_init(&mSem, 0, 1) != 0)
    {
        LOG_E("[SimplePool] Error: sem_init failed\n");
    }
#endif
    for (size_t i = 0; i < size; i++)
    {
        mPool.push_back(new T());
        mUsing.push_back(false);
    }
}

template <class T>
SimplePool<T>::~SimplePool()
{
#ifdef INC_FREERTOS_H
#else
    sem_destroy (&mSem);
#endif
}

template <class T>
void SimplePool<T>::zero(T *p)
{
}

template <class T>
int SimplePool<T>::getTotal()
{
    int ret = 0;
    sem_wait(&mSem);
    for (size_t i = 0; i < mUsing.size(); i++)
    {
        if (mUsing[i])
        {
            ret ++;
        }
    }
    sem_post(&mSem);
    return ret;
}

template <class T>
std::shared_ptr<T> SimplePool<T>::receive()
{
    sem_wait(&mSem);
    while (true)
    {
        for (size_t i = 0; i < mUsing.size(); i++)
        {
            if (!mUsing[i])
            {
                mUsing[i] = true;
                T *p = mPool[i];
                // *mPool[i] = YtMsg_init_zero;
                sem_post(&mSem);
                if (i > 5)
                {
                    LOG_I("[SimplePool] get no %d\n", (int)i);
                }
                return std::shared_ptr<T> (p, [this, i](T *p) {
                    sem_wait(&mSem);
                    if (i > 5)
                    {
                        LOG_I("[SimplePool] release no %d\n", (int)i);
                    }
                    zero(p);
                    mUsing[i] = false;
                    sem_post(&mSem);
                });
            }
        }
        if (mUsing.size() >= limit)
        {
            LOG_E("[SimplePool] meet limit(%d): %d!\n", limit, (int)mUsing.size());
            sem_post(&mSem);
            return std::shared_ptr<T>();
        }
        else
        {
            mPool.push_back(new T());
            mUsing.push_back(false);
            LOG_W("[SimplePool] " COLOR_RED "no free object in the pool" COLOR_NO ", enlarge to %d\n", (int)mUsing.size());
        }
    }
    //Never comes here
}
