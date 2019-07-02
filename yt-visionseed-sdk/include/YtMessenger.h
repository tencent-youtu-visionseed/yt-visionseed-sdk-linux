#ifndef __VS_MESSENGER_H__
#define __VS_MESSENGER_H__

#include <map>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "pb_decode.h"
#include "YtMsg.pb.h"
#include "IDataLinkObserver.h"
#include "DataLinkPosix.h"

#if defined(ESP32)
#else
    #include <sys/syscall.h>
    #define gettid() syscall(__NR_gettid)
#endif

#define TIMEOUT 10

#define VSRPC_POST(vs_message)  YtDataLinkPushPosix::getInstance()->sendYtMsgAsync(vs_message); \
    usleep(10000); \
    LOG_D("[YtMessenger] msg posted.\n"); \
    LOG_D("[YtMessenger] Pid: %d, tid: %ld.\n", getpid(), gettid()); \


// RPC Call
#define VSRPC(vs_request, vs_func, vs_params, vs_response)   \
    std::shared_ptr<YtMsg> vs_request(new YtMsg(), [](YtMsg *p) { LOG_D("[YtMsg rpc] release %p\n", p); pb_release(YtMsg_fields, p); delete p; });   \
    vs_request->which_values = YtMsg_rpc_tag; \
    vs_request->values.rpc.func = VSPRC_FUNC_TYPE(vs_func); \
    vs_request->values.rpc.which_params = VSPRC_PARAM_TAG(vs_params); \
    std::shared_ptr<YtMsg> vs_response;

#define VSRPC_CALL(vs_request, vs_response)       \
    ( vs_response = messenger->SendMsg(vs_request), (vs_response && (vs_response->values.response.code == YtRpcResponse_ReturnCode_SUCC || vs_response->values.response.code == YtRpcResponse_ReturnCode_CONTINUE)) )

#define VSRPC_CALL2(vs_request, vs_response)       \
    ( vs_response = SendMsg(vs_request), (vs_response && (vs_response->values.response.code == YtRpcResponse_ReturnCode_SUCC || vs_response->values.response.code == YtRpcResponse_ReturnCode_CONTINUE)) )

// RPC Response
#define VSRPC_RESPONSE(vs_response, vs_code, vs_data)   \
    std::shared_ptr<YtMsg> vs_response(new YtMsg(), [](YtMsg *p) { LOG_D("[YtMsg response] release %p\n", p); pb_release(YtMsg_fields, p); delete p; });   \
    vs_response->which_values = YtMsg_response_tag; \
    vs_response->values.response.code = VSPRC_CODE(vs_code); \
    vs_response->values.response.which_data = VSPRC_DATA_TAG(vs_data)


// Callback function types
//typedef void (*PreviewCallback)(shared_ptr<YtMsg> message);
typedef void (*OnResult)(shared_ptr<YtMsg> message);

typedef void (*RpcCallback)(shared_ptr<YtMsg> message);

class YtMessenger : public IObserver, public YtThread
{
public:
    YtMessenger(const char* dev);
    virtual ~YtMessenger();

    void PostMsg(shared_ptr<YtMsg> message); // send msg, don't need response
    shared_ptr<YtMsg> SendMsg(shared_ptr<YtMsg> message); // send msg, and wait for the response(wait up to TIMEOUT seconds)
    // void PostSuccResponseMsg();
    // void PostFailResponseMsg(int32_t value=0);
    // void PostIntResponseMsg(int32_t value);
    // void PostStringResponseMsg(std::string value);

    void RegisterCallback(int32_t func, RpcCallback callback);


    bool SendBuffer(std::string pathVS, size_t totalLength, const uint8_t *blobBuf);
    bool SendFile(std::string pathHost, std::string path);

//    void RegisterPreviewCallback(PreviewCallback callback);
    void RegisterOnFaceResult(OnResult callback);
//    bool WaitDownloadFaceLib();

    void startLoop();
    void exitLoop();

protected:
    virtual void *run();
    virtual void Update(int instanceId, shared_ptr<YtMsg> msg);

private:
    YtDataLinkPullPosix *pull;
    YtDataLinkPushPosix *push;

    sem_t mMsgReadySem;
    sem_t mMsgProcessingSem;
    shared_ptr<YtMsg> mMsg;

    sem_t mSengMsgSem;
    sem_t mSengMsgResponseSem;
    shared_ptr<YtMsg> mResponse;

    //RpcCallback mRpcCallDispatcher;
    void DispatchMsg(shared_ptr<YtMsg> message);
    std::map<int32_t, RpcCallback > mRpcTable;

    bool SendFilePart(std::string pathVS, size_t totalLength, const uint8_t *blobBuf, size_t length, size_t offset);
//    sem_t mDownloadSem;
//
//    pb_byte_t * mBuffer = NULL;
//
    OnResult mFaceResultCallback;
};

#endif
