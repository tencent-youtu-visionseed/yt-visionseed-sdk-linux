//author: chenliang @ Youtu Lab, Tencent
#ifndef YT_DATALINK_H
#define YT_DATALINK_H

#include "YtMsg.pb.h"

#include "YtLog.h"

#ifdef YTMSG_FULL
    #define CONST const
    #define ACCESS_CONST(x) (x)

    #define MAX_BLOB_BUF 131072
    //TODO: nanopb的YtMsg_size有bug：存在oneof修饰时，用了sizeof(union{})来计算，编译器不支持
    #undef YtMsg_size
    #define YtMsg_size (MAX_BLOB_BUF + sizeof(YtMsg)*2)

    #define YTDATALINK_RECV_BUF_SIZE 8192
    #define YTDATALINK_SEND_BUF_SIZE 4096
#else
    #include <avr/pgmspace.h>
    #define CONST const PROGMEM
    #define ACCESS_CONST(x) pgm_read_word(&x)

    #define YtMsg_size 1024
    #define YTDATALINK_RECV_BUF_SIZE 8
    #define YTDATALINK_SEND_BUF_SIZE 8
#endif

#define SOF 0x10
#define TRANS 0x11

// RPC Call
#define VSPRC_FUNC_TYPE(x)      YtRpc_Function_##x
#define VSPRC_PARAM_TAG(x)      YtRpc_##x##_tag
#define VSRPC_PARAM(vs_request) (vs_request)->values.rpc.params
#define DEFINE_SIMPLE_ONRPC_CALLBACK(vs_rpc, vs_params)     if (message->which_values == YtMsg_rpc_tag && message->values.rpc.func == VSPRC_FUNC_TYPE(vs_rpc) && message->values.rpc.which_params == VSPRC_PARAM_TAG(vs_params))
#define DEFINE_SIMPLE_ONRPC_CALLBACK_VOID(vs_rpc)     if (message->which_values == YtMsg_rpc_tag && message->values.rpc.func == VSPRC_FUNC_TYPE(vs_rpc))
// RPC Response
#define VSPRC_CODE(x)           YtRpcResponse_ReturnCode_##x
#define VSPRC_DATA_TAG(x)       YtRpcResponse_##x##_tag
#define VSRPC_DATA(vs_response) (vs_response)->values.response.data
// RPC Result
#define VSRESULT_DATA_TAG(x)      YtResult_##x##_tag
#define VSRESULT_DATA(vs_result) (vs_result)->values.result.data
#define VSRESULT(vs_result) (vs_result)->values.result

typedef enum
{
    YT_DL_IDLE = 0,
    YT_DL_LEN1_PENDING,
    YT_DL_LEN2_PENDING,
    YT_DL_LEN3_PENDING,
    YT_DL_LEN_CRC_H,
    YT_DL_LEN_CRC_L,
    YT_DL_DATA,
    YT_DL_CRC_H,
    YT_DL_CRC_L,
} YtDataLinkStatus;

class YtSerialPortBase
{
protected:
    int mPortFd = -1;
    char mDev[32];
public:
    YtSerialPortBase(const char *dev);
    virtual ~YtSerialPortBase();
    virtual bool isOpen() = 0;
    virtual int open() = 0;
    virtual int close() = 0;
    virtual int read(void *buffer, size_t len) = 0;
    virtual int write(void *buffer, size_t len) = 0;
};

class YtDataLink
{
public:
    YtDataLink(YtSerialPortBase *port);
    virtual ~YtDataLink();

    YtMsg *recvRunOnce(); //you are responsable to delete YtMsg which newed from 'virtual YtMsg *alloc()'
    void sendYtMsg(YtMsg *message);
protected:
    YtSerialPortBase *mPort; //YtDataLink responsable to delete it;

    // 接收相关
    YtDataLinkStatus mStatus = YT_DL_IDLE;
    uint32_t mMsgLen = 0;
    uint16_t mCrc = 0;
    uint8_t mBuf[YtMsg_size];
    size_t mBufi = 0;
    bool mTrans = false;
    uint16_t mCrcCalc;
    void crcUpdate(uint8_t ch, bool first);
    //用于回退的缓存
    uint8_t buf[YTDATALINK_RECV_BUF_SIZE];
    size_t bufRemain = 0;
    size_t bufStart = 0;
    virtual YtMsg *alloc();

    // 发送相关
    uint8_t mMsgBuf[YtMsg_size];
    uint8_t mSendBuf[YTDATALINK_SEND_BUF_SIZE];
    size_t mSendBufi = 0;
    uint16_t mCrcSendCalc;
    void crcSendUpdate(uint8_t ch, bool first);
    void writeOneByte(uint8_t ch, bool trans = true, bool first = false, bool flush = false);
};

#endif
