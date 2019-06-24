//author: chenliang @ Youtu Lab, Tencent

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

#include <pb_encode.h>
#include <pb_decode.h>
#include "YtMsg.pb.h"

#include "YtMsgPool.h"

#ifdef __rtems__
#include <rtems.h>
#include <Flic.h>
#endif
#include "YtLog.h"

static YtMsgPool pool(3);
YtMsgPool *YtMsgPool::getInstance()
{
    return &pool;
}
YtMsgPool::YtMsgPool(int size)
{
    if (sem_init(&mSem, 0, 0) != 0)
    {
        LOG_E("[YtMsgPool] Error: sem_init failed\n");
    }
    for (size_t i = 0; i < size; i++)
    {
        mPool.push_back(new YtMsg());
        mUsing.push_back(false);
    }
    sem_post(&mSem);
}
YtMsgPool::~YtMsgPool()
{
    sem_destroy (&mSem);
}
std::shared_ptr<YtMsg> YtMsgPool::receive()
{
    sem_wait(&mSem);
    while (true)
    {
        for (size_t i = 0; i < mUsing.size(); i++)
        {
            if (!mUsing[i])
            {
                mUsing[i] = true;
                YtMsg *p = mPool[i];
                // *mPool[i] = YtMsg_init_zero;
                sem_post(&mSem);
                if (i > 5)
                {
                    LOG_I("[YtMsgPool] get no %d\n", (int)i);
                }
                return std::shared_ptr<YtMsg> (p, [this, i](YtMsg *p) {
                    sem_wait(&mSem);
                    if (i > 5)
                    {
                        LOG_I("[YtMsgPool] release no %d\n", (int)i);
                    }
                    pb_release(YtMsg_fields, p);
                    //TODO: should reset all fields!
                    // p->values.result.has_frameId = false;
                    struct timeval start, end;
                    gettimeofday(&start, NULL);
                    memset(p, 0, sizeof(YtMsg));
                    gettimeofday(&end, NULL);
                    float ts = 1000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec)/1000.f;
                    if (ts > 5)
                    {
                        LOG_W("[YtMsgPool] WARNING: zero %lu bytes in %4.2f ms, TOO SLOW!\n", (unsigned long)sizeof(YtMsg), ts);
                    }
                    mUsing[i] = false;
                    sem_post(&mSem);
                });
            }
        }
        mPool.push_back(new YtMsg());
        mUsing.push_back(false);
        LOG_I("[YtMsgPool] no free object in the pool, enlarge to %d\n", (int)mUsing.size());
    }
    //Never comes here
}
