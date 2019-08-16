//author: chenliang @ Youtu Lab, Tencent
#ifndef YT_DATALINKPOSIX_H
#define YT_DATALINKPOSIX_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>

#include "DataLink.h"
#include "IDataLinkObserver.h"
#include <vector>
#include <queue>
#include <memory>
#include <string>

#include "YtMsg.pb.h"

using namespace std;


class YtThread
{
public:
    static void *threadFunc(void *args);
    void exit();
protected:
    volatile bool shouldExit;
    pthread_t thread;
    string mName;
    YtThread(string name);
    virtual ~YtThread();
    void start();
    virtual void *run() = 0;
};
class YtSerialPortPosix : public YtSerialPortBase
{
public:
    YtSerialPortPosix(const char *dev);
    virtual ~YtSerialPortPosix();
    virtual bool isOpen();
    virtual int open();
    virtual int close();
    virtual int read(void *buffer, size_t len);
    virtual int write(void *buffer, size_t len);
};

class YtDataLinkPullPosix : public YtSubject, public YtThread, public YtDataLink
{
public:
    static int createInstance(const char *dev); //TODO: use weak_ptr
    static YtDataLinkPullPosix *getInstance(int instanceId);
    static void AttachAll(IObserver *);  // 注册观察者
    static void DetachAll(IObserver *);  // 注销观察者
    virtual ~YtDataLinkPullPosix();
protected:
    int instanceId;
    static vector<YtDataLinkPullPosix *> mInstance;
    YtDataLinkPullPosix(const char *dev);
    virtual void *run();
};

class YtDataLinkPushPosix : public YtThread
{
public:
    static int createInstance(YtDataLink *dev); //TODO: use weak_ptr
    static YtDataLinkPushPosix *getInstance(int instanceId);
    static void broadcastYtMsgAsync(shared_ptr<YtMsg> msg);
    virtual ~YtDataLinkPushPosix();
    virtual void sendYtMsgAsync(shared_ptr<YtMsg> msg);
    virtual void sendResponseAsync(YtRpcResponse_ReturnCode code, bool has_sequenceId, int32_t sequenceId);
    virtual void sendResponseWithIntDataAsync(YtRpcResponse_ReturnCode code, int intData, bool has_sequenceId, int32_t sequenceId);
protected:
    YtDataLink *mDev;
    static vector<YtDataLinkPushPosix *> mInstance;
    sem_t mUpdateSem;
    std::queue< shared_ptr<YtMsg> > mQueue;

    YtDataLinkPushPosix(YtDataLink *dev);
    virtual void *run();
};

#endif
