//author: chenliang @ Youtu Lab, Tencent
#ifndef OBSERVER_H
#define OBSERVER_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>
#include <semaphore.h>

#include <list>
#include <memory>

#include "YtMsg.pb.h"

using namespace std;

// 抽象观察者
class IObserver
{
public:
    virtual void Update(int instanceId, shared_ptr<YtMsg> msg) = 0;
};

class YtSubject
{
    sem_t mObserversSem;
    list<IObserver *> mObservers;  // 观察者列表
public:
    YtSubject();
    virtual ~YtSubject();
    virtual void Attach(IObserver *);  // 注册观察者
    virtual void Detach(IObserver *);  // 注销观察者
    virtual void Notify(int instanceId, shared_ptr<YtMsg> msg);  // 通知观察者
};

#endif
