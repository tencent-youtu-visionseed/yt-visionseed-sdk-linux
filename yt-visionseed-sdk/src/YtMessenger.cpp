#include <unistd.h>

#include "YtMessenger.h"

#define DEVICE_STATUS_BUSY      0x01

YtMessenger::YtMessenger(const char* dev) : IObserver(), YtThread("YtMessenger"), mNGResultCallback(NULL), mStatusCallback(NULL)
{
    int instanceId = YtDataLinkPullPosix::createInstance(dev);
    YtDataLinkPushPosix::createInstance(YtDataLinkPullPosix::getInstance(instanceId));

    YtDataLinkPullPosix::AttachAll(this);

    mMsg = NULL;
#ifdef INC_FREERTOS_H
    mMsgReadySem = xSemaphoreCreateCounting(10, 0);
    mMsgProcessingSem = xSemaphoreCreateCounting(10, 1);
    mSengMsgSem = xSemaphoreCreateCounting(1, 1);
    mSengMsgResponseSem = xSemaphoreCreateCounting(10, 0);
#else
    if (sem_init(&mMsgReadySem, 0, 0) != 0) {
        LOG_E("[YtMessenger] response semaphore init failed.\n");
    }

    if (sem_init(&mMsgProcessingSem, 0, 1) != 0) {
        LOG_E("[YtMessenger] response semaphore init failed.\n");
    }

    if (sem_init(&mSengMsgSem, 0, 1) != 0) {
        LOG_E("[YtMessenger] response semaphore init failed.\n");
    }

    if (sem_init(&mSengMsgResponseSem, 0, 0) != 0) {
        LOG_E("[YtMessenger] response semaphore init failed.\n");
    }
#endif
}

YtMessenger::~YtMessenger()
{
    YtDataLinkPullPosix::DetachAll(this);

    LOG_D("[YtMessenger] released.\n");
}

void YtMessenger::startLoop()
{
    start();
}

void YtMessenger::exitLoop()
{
    LOG_D("[YtMessenger] exiting, Pid: %d, tid: %ld.\n", getpid(), gettid());

    mMsg = NULL;
    exit();
}

void *YtMessenger::run()
{
    LOG_D("[YtMessenger] messenger start, Pid: %d, tid: %ld.\n", getpid(), gettid());

    while ( !shouldExit )
    {
        //LOG_D("[YtMessenger] Pid %d, tid %ld : asked for msg\n", getpid(), gettid());
        sem_wait(&mMsgReadySem); // wait for a new message
        //LOG_D("[YtMessenger] Pid %d, tid %ld : asked for resource\n", getpid(), gettid());
        sem_wait(&mMsgProcessingSem); // wait for processing resource

        // LOG_D("[YtMessenger] Pid %d, tid %ld : msg is processing\n", getpid(), gettid());



        if ( mMsg )
        {
            // LOG_D("[YtMessenger] Pid %d, tid %ld : recv result\n", getpid(), gettid());
            switch (mMsg->values.result.which_data)
            {
                case YtResult_systemStatusResult_tag:
                    if (mStatusCallback != NULL)
                    {
                        mStatusCallback(mMsg);
                    }
                    break;
            }
            if (VSRESULT_DATAV2(mMsg) != NULL &&
                VSRESULT_DATAV2(mMsg)->size > 0 &&
                mMsg->values.result.which_data != YtResult_faceDetectionResult_tag)
            {
                if (mNGResultCallback != NULL)
                {
                    mNGResultCallback(mMsg);
                }
            }
            // DispatchMsg(mMsg);
            mMsg = NULL;
        }

        if ( mMsg )
        {
            LOG_D("[YtMessenger] msg is not null, Pid: %d, tid: %ld.\n", getpid(), gettid());
        }

        //LOG_D("[YtMessenger] Pid %d, tid %ld : msg is processed\n", getpid(), gettid());
        sem_post(&mMsgProcessingSem); // release processing resource
    }

    return NULL;
}

void YtMessenger::PostMsg(shared_ptr<YtMsg> message)
{
   YtDataLinkPushPosix::getInstance(0)->sendYtMsgAsync(message);
   usleep(10000);
   LOG_D("[YtMessenger] request posted.\n");
   LOG_D("[YtMessenger] Pid: %d, tid: %ld.\n", getpid(), gettid());
}

shared_ptr<YtMsg> YtMessenger::SendMsg(shared_ptr<YtMsg> message)
{
    sem_wait(&mSengMsgSem);

    LOG_D("[YtMessenger::SendMsg] Pid %d, tid %ld : send msg\n", getpid(), gettid());

    mResponse = NULL;

    YtDataLinkPushPosix::getInstance(0)->sendYtMsgAsync(message);
    usleep(10000);
    LOG_D("[YtMessenger::SendMsg] request sent, wait for response.\n");

    // wait for response
    shared_ptr<YtMsg> response;
    int ret = sem_timedwait(&mSengMsgResponseSem, TIMEOUT*1000);
    if ( ret == 0 )
    {
        response = mResponse;
        //LOG_D("[YtMessenger] response received.\n");
        LOG_D("[YtMessenge::SendMsgr] Pid %d, tid %ld : recv response\n", getpid(), gettid());
    }
    else
    {
        LOG_W("[YtMessenger::SendMsg] no response received.\n");
    }

    mResponse = NULL;

    sem_post(&mSengMsgSem);
    return response;
}

// void YtMessenger::PostSuccResponseMsg()
// {
//     VSRPC_RESPONSE(responseMsg, SUCC, intData);
//     VSRPC_DATA(responseMsg).intData = 0;
//     VSRPC_POST(responseMsg);
// }
//
// void YtMessenger::PostFailResponseMsg(int32_t value)
// {
//     VSRPC_RESPONSE(responseMsg, FAIL, intData);
//     VSRPC_DATA(responseMsg).intData = value;
//     VSRPC_POST(responseMsg);
// }
//
// void YtMessenger::PostIntResponseMsg(int32_t value)
// {
//     VSRPC_RESPONSE(responseMsg, SUCC, intData);
//     VSRPC_DATA(responseMsg).intData = value;
//     VSRPC_POST(responseMsg);
// }
//
// void YtMessenger::PostStringResponseMsg(std::string value)
// {
//     VSRPC_RESPONSE(responseMsg, SUCC, strData);
//     strcpy(VSRPC_DATA(responseMsg).strData, value.c_str());
//     VSRPC_POST(responseMsg);
// }

// NOTE!!!: The function below is running in a diffrent thread, which is YtDataLinkPullPosix.
// DO NOT use wait semaphore in this thread, wait for semaphore will block receiving data from the serial port
void YtMessenger::Update(int instanceId, shared_ptr<YtMsg> message)
{
    // dispatch rpc & result msg
    int svalProcessing=0;
    int svalResponse=0;
    int svalSending=0;
#ifdef INC_FREERTOS_H
    svalProcessing = uxSemaphoreGetCount(mMsgProcessingSem);
    svalResponse = uxSemaphoreGetCount(mSengMsgResponseSem);
    svalSending = uxSemaphoreGetCount(mSengMsgSem);
#else
    sem_getvalue(&mMsgProcessingSem, &svalProcessing);
    sem_getvalue(&mSengMsgResponseSem, &svalResponse);
    sem_getvalue(&mSengMsgSem, &svalSending);
#endif
    // LOG_D("[YtMessenger] Pid %d, tid %ld : svalProcessing: %d, svalResponse: %d\n", getpid(), gettid(), svalProcessing, svalResponse);

    switch ( message->which_values )
    {
    case YtMsg_rpc_tag:
        break;

    case YtMsg_result_tag:
        if ( svalProcessing == 0 ) {// is processing
            // too fast, drop result
            // PostFailResponseMsg(DEVICE_STATUS_BUSY);
            LOG_D("[YtMessenger] dropped result\n");
        }
        else { // idle
            sem_wait(&mMsgProcessingSem); // wait for processing resource
            mMsg = message;
            sem_post(&mMsgProcessingSem); // wait for processing resource
            sem_post(&mMsgReadySem);
            LOG_D("[YtMessenger] Pid %d, tid %ld : msg is ready\n", getpid(), gettid());
        }
        break;

    case YtMsg_response_tag:
        if (svalSending == 1) {
            LOG_D("[YtMessenger] response received, but no rpc calls!\n");
            break;
        }
        if ( svalResponse == 0 ) {
            LOG_D("[YtMessenger] response received.\n");
            mResponse = message;
            sem_post(&mSengMsgResponseSem);
        }

        LOG_D("[YtMessenger] Pid %d, tid %ld : recv response\n", getpid(), gettid());
        break;

    }
}

void YtMessenger::DispatchMsg(shared_ptr<YtMsg> message)
{
    if ( message && message->which_values == YtMsg_rpc_tag) // RPC msg
    {
        if ( mRpcTable.empty() ) {
            return ;
        }

        std::map<int32_t, RpcCallback >::iterator rpc_it = mRpcTable.find(message->values.rpc.func);
        if ( rpc_it == mRpcTable.end() ) {
            return ;
        }

        rpc_it->second(message);
    }
    else { // result message
    }

}

void YtMessenger::RegisterCallback(int32_t func, RpcCallback callback)
{
    mRpcTable[func] = callback;
    LOG_D("[VSWrapper] add callback for func: %d.\n", func);
}

bool YtMessenger::SendFilePart(std::string pathVS, size_t totalLength, const uint8_t *blobBuf, size_t length, size_t offset, std::string auth)
{
    VSRPC(request, uploadFile, filePart, response);
    strcpy(VSRPC_PARAM(request).filePart.path, pathVS.c_str());
    VSRPC_PARAM(request).filePart.totalLength = totalLength;
    VSRPC_PARAM(request).filePart.offset = offset;
    bool hasAuth= (auth != "") ? true : false;
    request->values.rpc.has_auth = hasAuth;
    if (hasAuth)
        memcpy(request->values.rpc.auth, auth.c_str(), auth.length());
    size_t len = ((size_t)length + offsetof(pb_bytes_array_t, bytes));
    VSRPC_PARAM(request).filePart.data = (pb_bytes_array_t*)malloc(len);//(pb_bytes_array_t*)malloc(PB_BYTES_ARRAY_T_ALLOCSIZE(MAX_BLOB_BUF));
    VSRPC_PARAM(request).filePart.data->size = length;
    memcpy(VSRPC_PARAM(request).filePart.data->bytes, blobBuf, length);

    LOG_D("[SendFilePart] sending: %lu/%lu\n", (long)(offset + length), (long)totalLength);
    bool ret = VSRPC_CALL2(request, response);
    if (!ret && response)
    {
        LOG_E("[SendFilePart] failed: %d\n", response->values.response.code);
    }
    return ret;
}

bool YtMessenger::SendBuffer(std::string pathVS, size_t totalLength, const uint8_t *blobBuf, std::string auth)
{
    // Write in a loop to exhaust the buffer
    off_t current_offset= 0;
    off_t remaining = totalLength;
    off_t size_to_transmit = 0;

    while(remaining > 0){
        size_to_transmit = remaining > MAX_BLOB_BUF ? MAX_BLOB_BUF : remaining;
        if ( !SendFilePart(pathVS, totalLength, blobBuf + current_offset, size_to_transmit, current_offset, auth) ) {
            LOG_E("[MsgProcessor] failed to send blob package\n");
            return false;
        }
        current_offset += size_to_transmit;
        remaining = totalLength - current_offset;
    }

    return true;
}

//bool YtMessenger::ResponseBuffer(BlobParams_BlobType type, int32_t arg1, std::string arg2, size_t totalLength, const uint8_t *blobBuf)
//{
//
//}

bool YtMessenger::SendFile(std::string pathHost, std::string path, std::string auth)
{
    FILE *fin = fopen(pathHost.c_str(), "rb");
    if(!fin) {
        LOG_E("open file error");
        return false;
    }

    fseek(fin, 0, SEEK_END);
    off_t totalLength = ftell(fin);
    uint8_t *blobBuf = (uint8_t*) malloc(totalLength);
    fseek(fin, 0, SEEK_SET);
    LOG_D("Reading file: %s (size %d)\n", pathHost.c_str(), (int)totalLength);

    int ret_read = fread(blobBuf, 1, totalLength, fin);
    if(ret_read <= 0 || ret_read != totalLength) {
        LOG_E("failed to read file: %s\n", pathHost.c_str());
        return false;
    }

    LOG_D("Closing file\n" );
    fclose(fin);

    if ( SendBuffer(path, totalLength, blobBuf, auth) ) {
        free(blobBuf);
        return true;
    }

    free(blobBuf);
    return false;
}

void YtMessenger::RegisterOnStatus(OnResultCallback callback)
{
    mStatusCallback = callback;
}
void YtMessenger::RegisterOnResult(OnResultCallback callback)
{
    mNGResultCallback = callback;
}
