//author: chenliang @ Youtu Lab, Tencent
#include <pthread.h>

#include "IDataLinkObserver.h"
#include <queue>
#include <memory>

#include "YtMsg.pb.h"
#include "SimplePool.h"

using namespace std;

class YtMsgPool : public SimplePool<YtMsg> {
public:
    static YtMsgPool *getInstance();
    YtMsgPool(int size);
    virtual ~YtMsgPool();
protected:
    virtual void zero(YtMsg *p);
};
