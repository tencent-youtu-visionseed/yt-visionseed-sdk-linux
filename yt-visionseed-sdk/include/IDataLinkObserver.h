//author: chenliang @ Youtu Lab, Tencent
#ifndef OBSERVER_H
#define OBSERVER_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>

#if defined(ESP32)
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
    #include <freertos/semphr.h>
    #define sem_t SemaphoreHandle_t
    #define sem_timedwait(sem, ts) (xSemaphoreTake(*sem, ((ts)->tv_sec*1000+(ts)->tv_nsec/1000000)/portTICK_PERIOD_MS)==pdTRUE ? 0 : -1)
    #define sem_wait(sem) xSemaphoreTake(*sem, 1000/portTICK_PERIOD_MS)
    #define sem_post(sem) xSemaphoreGive(*sem)
    #define gettid() (long)xTaskGetCurrentTaskHandle()
    #define getpid() 0
#else
    #include <semaphore.h>
#endif

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
