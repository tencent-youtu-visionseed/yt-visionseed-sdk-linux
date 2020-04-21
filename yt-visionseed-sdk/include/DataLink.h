//author: chenliang @ Youtu Lab, Tencent
#ifndef YT_DATALINK_H
#define YT_DATALINK_H

#include "YtMsg.pb.h"
#include "YtLog.h"

#if defined(YTMSG_FULL)
    #define CONST const
    #define ACCESS_CONST(x) (x)

    #define YTMSG_USE_RECV_POOL true
    #define MAX_BLOB_BUF 131072
    //TODO: nanopb的YtMsg_size有bug：存在oneof修饰时，用了sizeof(union{})来计算，编译器不支持
    #undef YtMsg_size
    #define YtMsg_size (MAX_BLOB_BUF + sizeof(YtMsg)*2)

    #define YTDATALINK_RECV_BUF_SIZE 8192
    #define YTDATALINK_SEND_BUF_SIZE 4096
#elif defined(YTMSG_LITE)
    #define CONST const
    #define ACCESS_CONST(x) (x)

    #define YTMSG_USE_RECV_POOL true
#if defined(STM32)
    #define MAX_BLOB_BUF 1024
#else
    #define MAX_BLOB_BUF 10240
#endif
    //TODO: nanopb的YtMsg_size有bug：存在oneof修饰时，用了sizeof(union{})来计算，编译器不支持
    #undef YtMsg_size
    #define YtMsg_size (MAX_BLOB_BUF + sizeof(YtMsg))

    #define YTDATALINK_RECV_BUF_SIZE 256
    #define YTDATALINK_SEND_BUF_SIZE 256
#else
    #include <avr/pgmspace.h>
    #define CONST const PROGMEM
    #define ACCESS_CONST(x) pgm_read_word(&x)

    #undef YTMSG_USE_RECV_POOL
    #define YtMsg_size 600
    #define YTDATALINK_RECV_BUF_SIZE 8
    #define YTDATALINK_SEND_BUF_SIZE 8
#endif

#ifdef YTMSG_USE_RECV_POOL
#include <memory>
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
#if defined(YTMSG_FULL)
    #define VSRESULT_DATAV2(vs_result) ((pb_bytes_array_t*)&((vs_result)->values.result.dataV2))
#else
    #define VSRESULT_DATAV2(vs_result) ((pb_bytes_array_t*)((vs_result)->values.result.dataV2))
#endif
#define VSRESULT_DATAV2_SIZE(vs_result) sizeof((vs_result)->values.result.dataV2.bytes)
#define VSRESULT(vs_result) (vs_result)->values.result

#define VS_MODEL_FACE_DETECTION 1
#define VS_MODEL_FACE_LANDMARK 2
#define VS_MODEL_FACE_POSE 3
#define VS_MODEL_FACE_QUALITY 4
#define VS_MODEL_FACE_RECOGNITION 6
#define VS_MODEL_DETECTION_TRACE 8

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

typedef enum
{
    YT_RESULT_RECT = 0,
    YT_RESULT_ARRAY,
    YT_RESULT_CLASSIFICATION,
    YT_RESULT_STRING,
    YT_RESULT_POINTS,
    YT_RESULT_VARUINT32,
} YtVisionSeedResultType;
typedef struct
{
    float conf;
    uint16_t cls;
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} YtVisionSeedResultTypeRect;
typedef struct
{
    float conf;
    uint16_t cls;
} YtVisionSeedResultTypeClassification;
typedef struct
{
    uint8_t count;
    const float *p;
} YtVisionSeedResultTypeArray;
typedef struct
{
    float conf;
    const char *p;
} YtVisionSeedResultTypeString;
typedef struct
{
    struct Point {
        int16_t x;
        int16_t y;
    };
    uint8_t count;
    Point *p;
} YtVisionSeedResultTypePoints;

class YtSerialPortBase
{
protected:
    int mPortFd;
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

#ifdef YTMSG_USE_RECV_POOL
    std::shared_ptr<YtMsg> recvRunOnce();
#else
    const uint8_t *recvRunOnce(); // don't free the return pointer!
#endif

#if defined(YTMSG_FULL) || defined(YTMSG_LITE)
    void sendYtMsg(YtMsg *message);
#endif

    // 结果包解析
    static const uint8_t* getResult(const uint8_t *buf, uint32_t &ret_len, const uint8_t path[], const uint8_t path_len);
    static bool getResult(const uint8_t *buf, YtVisionSeedResultTypeRect *result, const uint8_t path[], const uint8_t path_len);
    static bool getResult(const uint8_t *buf, YtVisionSeedResultTypeClassification *result, const uint8_t path[], const uint8_t path_len);
    static bool getResult(const uint8_t *buf, YtVisionSeedResultTypeArray *result, const uint8_t path[], const uint8_t path_len);
    static bool getResult(const uint8_t *buf, uint32_t *result, const uint8_t path[], const uint8_t path_len);
    static bool getResult(const uint8_t *buf, YtVisionSeedResultTypeString *result, const uint8_t path[], const uint8_t path_len);
    static bool getResult(const uint8_t *buf, YtVisionSeedResultTypePoints *result, const uint8_t path[], const uint8_t path_len);
    static void addResultRaw(pb_bytes_array_t *data, uint32_t size, const uint8_t path[], const uint8_t path_len, YtVisionSeedResultType _type, const uint8_t *content, const uint32_t len);
    static void addResult(pb_bytes_array_t *data, uint32_t size, const uint8_t path[], const uint8_t path_len, const YtVisionSeedResultTypeRect &result);
    static void addResult(pb_bytes_array_t *data, uint32_t size, const uint8_t path[], const uint8_t path_len, const YtVisionSeedResultTypeClassification &result);
    static void addResult(pb_bytes_array_t *data, uint32_t size, const uint8_t path[], const uint8_t path_len, const YtVisionSeedResultTypeArray &result);
    static void addResult(pb_bytes_array_t *data, uint32_t size, const uint8_t path[], const uint8_t path_len, const YtVisionSeedResultTypeString &result);
    static void addResult(pb_bytes_array_t *data, uint32_t size, const uint8_t path[], const uint8_t path_len, const YtVisionSeedResultTypePoints &result);
    static void addResult(pb_bytes_array_t *data, uint32_t size, const uint8_t path[], const uint8_t path_len, const uint32_t result);
    static void initDataV2(YtMsg *message);
    const uint8_t *getPbField(int tag, uint32_t *len);

protected:
    YtSerialPortBase *mPort; //YtDataLink responsable to delete it;

    // 接收相关
    YtDataLinkStatus mStatus;
    uint32_t mMsgLen;
    uint16_t mCrc;
    uint8_t mBuf[YtMsg_size];
    size_t mBufi;
    bool mTrans;
    uint16_t mCrcCalc;
    void crcUpdate(uint8_t ch, bool first);
    //用于回退的缓存
    uint8_t buf[YTDATALINK_RECV_BUF_SIZE];
    size_t bufRemain;
    size_t bufStart;
    virtual YtMsg *alloc();

#if defined(YTMSG_FULL) || defined(YTMSG_LITE)
    // 发送相关
    uint8_t mMsgBuf[YtMsg_size];
    uint8_t mSendBuf[YTDATALINK_SEND_BUF_SIZE];
    size_t mSendBufi;
    uint16_t mCrcSendCalc;
    void crcSendUpdate(uint8_t ch, bool first);
    void writeOneByte(uint8_t ch, bool trans = true, bool first = false, bool flush = false);
#endif
};

#endif
