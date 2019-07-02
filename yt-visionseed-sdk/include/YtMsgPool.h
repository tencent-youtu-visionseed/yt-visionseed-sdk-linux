//author: chenliang @ Youtu Lab, Tencent
#include <pthread.h>

#include "IDataLinkObserver.h"
#include <queue>
#include <memory>

#include "YtMsg.pb.h"

using namespace std;

class YtMsgPool {
public:
    static YtMsgPool *getInstance();
    YtMsgPool(int size);
    virtual ~YtMsgPool();
    std::shared_ptr<YtMsg> receive();
protected:
    sem_t mSem;
    vector<bool> mUsing;
    vector<YtMsg*> mPool;
};
