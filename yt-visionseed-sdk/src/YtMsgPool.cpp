//author: chenliang @ Youtu Lab, Tencent

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <memory>
#include <pthread.h>
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

YtMsgPool *YtMsgPool::getInstance()
{
    if (instance == NULL)
    {
        instance = new YtMsgPool(1);
    }
    return (YtMsgPool*)instance;
}

YtMsgPool::YtMsgPool(int size) : SimplePool(size, 16)
{
}
YtMsgPool::~YtMsgPool()
{
}

void YtMsgPool::zero(YtMsg *p)
{
    pb_release(YtMsg_fields, p);
    //TODO: should reset all fields!
    // p->values.result.has_frameId = false;
#ifdef __rtems__
    struct timeval start, end;
    gettimeofday(&start, NULL);
#endif
    memset(p, 0, sizeof(YtMsg));
#ifdef __rtems__
    gettimeofday(&end, NULL);
    float ts = 1000 * ( end.tv_sec - start.tv_sec ) + (end.tv_usec - start.tv_usec)/1000.f;
    if (ts > 5)
    {
        LOG_W("[YtMsgPool] WARNING: zero %lu bytes in %4.2f ms, TOO SLOW!\n", (unsigned long)sizeof(YtMsg), ts);
    }
#endif
}
