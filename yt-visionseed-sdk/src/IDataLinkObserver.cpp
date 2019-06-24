//author: chenliang @ Youtu Lab, Tencent

#include "IDataLinkObserver.h"
#include "YtLog.h"

YtSubject::YtSubject()
{
    if (sem_init(&mObserversSem, 0, 1) != 0)
    {
        LOG_E("[YtSubject] Error: sem_init failed\n");
    }
}
YtSubject::~YtSubject()
{
    sem_wait(&mObserversSem);
    mObservers.clear();
    sem_post(&mObserversSem);
}

void YtSubject::Attach(IObserver *observer) {
    sem_wait(&mObserversSem);
    mObservers.push_back(observer);
    sem_post(&mObserversSem);
}

void YtSubject::Detach(IObserver *observer) {
    sem_wait(&mObserversSem);
    mObservers.remove(observer);
    sem_post(&mObserversSem);
}

void YtSubject::Notify(int instanceId, shared_ptr<YtMsg> msg) {
    sem_wait(&mObserversSem);
    list<IObserver *>::iterator it = mObservers.begin();
    while (it != mObservers.end()) {
        (*it)->Update(instanceId, msg);
        ++it;
    }
    sem_post(&mObserversSem);
}
