//author: chenliang @ Youtu Lab, Tencent
#include <pthread.h>

#include "IDataLinkObserver.h"
#include <queue>
#include <memory>

using namespace std;

template <class T>
class SimplePool {
public:
    SimplePool(int size, int _limit);
    virtual ~SimplePool();
    std::shared_ptr<T> receive();
    int getTotal();
    int limit;
protected:
    static SimplePool *instance;
    virtual void zero(T *p);
    sem_t mSem;
    vector<bool> mUsing;
    vector<T*> mPool;
};
