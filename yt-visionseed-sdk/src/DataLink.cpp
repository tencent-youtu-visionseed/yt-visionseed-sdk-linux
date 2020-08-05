//author: chenliang @ Youtu Lab, Tencent

#include <stdio.h>
#include <stdlib.h>

#include <pb_encode.h>
#include <pb_decode.h>
#include "YtMsg.pb.h"

#include "DataLink.h"
#ifdef YTMSG_USE_RECV_POOL
#include "YtMsgPool.h"
#endif

#ifndef MIN
    #define MIN(a,b) ( ((a)<(b))?(a):(b) )
#endif

YtSerialPortBase::YtSerialPortBase(const char *dev)
{
    mPortFd = -1;
    snprintf(mDev, sizeof(mDev), "%s", dev);
}
YtSerialPortBase::~YtSerialPortBase()
{
}
// #include <VcsHooksApi.h>

#ifdef YTMSG_USE_RECV_POOL
std::shared_ptr<YtMsg> YtDataLink::recvRunOnce()
#else
const uint8_t *YtDataLink::recvRunOnce()
#endif
{
    if (!mPort->isOpen())
    {
        mPort->open();
        return NULL;
    }

    uint8_t ch;
    int readCnt;
    // while (true)
    {
        if (bufRemain > 0)
        {
            readCnt = 1;
            if (bufStart >= sizeof(buf)) {LOG_E("[Error] buf overflow!!!\n");}
            ch = buf[bufStart ++];
            bufRemain --;
        }
        else
        {
            readCnt = mPort->read(&ch, 1);
        }
        if (readCnt > 0)
        {
            // LOG_D("[YtMsg] %02x\n", ch);
            if (ch == SOF)
            {
                if (mStatus != YT_DL_IDLE)
                {
                    LOG_E("[YtMsg] unfinished pkg(%lu/%d) %d crc 0x%04x ?= 0x%04x\n", (long)mBufi, mMsgLen, mStatus, mCrcCalc, mCrc);
                }
                mStatus = YT_DL_LEN1_PENDING;
            }
            else if (ch == TRANS)
            {
                mTrans = true;
            }
            else
            {
                //转义后，不会出现SOF, TRANS
                if (mTrans)
                {
                    ch = ch ^ TRANS;
                    mTrans = false;
                }
                switch (mStatus)
                {
                    case YT_DL_IDLE:
                        LOG_D("[YtMsg] IDLE %02x\n", ch);
                        break;
                    case YT_DL_LEN1_PENDING:
                        // LOG_D("[YtMsg] begin\n");
                        mStatus = YT_DL_LEN1_PENDING;
                        mMsgLen = 0;
                        mCrc = 0;
                        mBufi = 0;
                        memset(mBuf, 0, sizeof(mBuf));
                        //passthrough
                    case YT_DL_LEN2_PENDING:
                    case YT_DL_LEN3_PENDING:
                        crcUpdate(ch, mStatus == YT_DL_LEN1_PENDING);
                        mMsgLen = (mMsgLen << 8) | ch;
                        if (mStatus == YT_DL_LEN3_PENDING)
                        {
                            if (mMsgLen > sizeof(mBuf))
                            {
                                LOG_E("[YtMsg] Error: msg len %d > %lu\n", mMsgLen, (long)sizeof(mBuf));
                                mStatus = YT_DL_IDLE;
                                return NULL;
                            }
                        }
                        mStatus = (YtDataLinkStatus)((int)mStatus + 1);
                        break;
                    case YT_DL_LEN_CRC_H:
                        mCrc = (mCrc << 8) | ch;
                        mStatus = (YtDataLinkStatus)((int)mStatus + 1);
                        break;
                    case YT_DL_LEN_CRC_L:
                        mCrc = (mCrc << 8) | ch;
                        if ((mCrcCalc) != mCrc)
                        {
                            LOG_E("[YtMsg] Error: msg len crc 0x%04x != 0x%04x\n", mCrcCalc, mCrc);
                            mStatus = YT_DL_IDLE;
                            return NULL;
                        }
                        mStatus = (YtDataLinkStatus)((int)mStatus + 1);
                        //passthrough
                    case YT_DL_DATA:
                        while (mBufi < mMsgLen)
                        {
                            if (bufRemain == 0)
                            {
                                bufStart = 0;
                                bufRemain = 0;
                                int reads = mPort->read(buf, MIN(sizeof(buf), mMsgLen-mBufi));
                                // LOG_D("[YtMsg] read(MIN(%d,%d))=%d 0x%04x\n", sizeof(buf), mMsgLen-mBufi, reads, mCrcCalc);
                                if (reads >= 0)
                                {
                                    bufRemain = reads;
                                }
                                else
                                {
                                    LOG_E("[YtMsg] serial bulk read error %d\n", reads);
                                    mStatus = YT_DL_IDLE;
                                    return NULL;
                                }
                                // LOG_D("[YtMsg] bulk read %d\n", bufRemain);
                            }
                            while(bufRemain > 0 && mBufi < mMsgLen)
                            {
                                if (bufStart >= sizeof(buf)) {LOG_E("[Error] buf overflow!!!\n");}
                                ch = buf[bufStart++];
                                bufRemain --;

                                if (ch == SOF)
                                {
                                    mTrans = false;
                                    mStatus = YT_DL_LEN1_PENDING;
                                    LOG_D("[YtMsg] unfinished pkg(%lu/%d)\n", (long)mBufi, mMsgLen);
                                    //buf, bufStart, bufRemain下一轮会继续使用
                                    return NULL;
                                }
                                else if (ch == TRANS)
                                {
                                    mTrans = true;
                                }
                                else
                                {
                                    if (mTrans)
                                    {
                                        ch = ch ^ TRANS;
                                        mTrans = false;
                                    }
                                    crcUpdate(ch, mBufi == 0);
                                    mBuf[mBufi ++] = ch;
                                }
                            }
                        }
                        mStatus = (YtDataLinkStatus)((int)mStatus + 1);
                        break;
                    case YT_DL_CRC_H:
                        mCrc = 0;
                        //passthrough
                    case YT_DL_CRC_L:
                        mCrc = (mCrc << 8) | ch;
                        if (mStatus == YT_DL_CRC_L)
                        {
                            mStatus = YT_DL_IDLE;
                            if ((mCrcCalc) != mCrc)
                            {
                                LOG_E("[YtMsg] Error: msg crc 0x%04x != 0x%04x\n", mCrcCalc, mCrc);
                                return NULL;
                            }

#ifdef YTMSG_USE_RECV_POOL
                            std::shared_ptr<YtMsg> message = YtMsgPool::getInstance()->receive();
                            if (!message)
                            {
                                LOG_E("memory pool empty or no memory.\n");
                                return NULL;
                            }
                            // LOG_D("[YtMsg] new %p\n", message.get());
                            // YtMsg message = YtMsg_init_zero;
                            /* Create a stream that reads from the buffer. */
                            pb_istream_t stream = pb_istream_from_buffer(mBuf, mMsgLen);

                            /* Now we are ready to decode the message. */
                            bool status = pb_decode(&stream, YtMsg_fields, message.get());

                            if (status)
                            {
                                #ifdef __rtems__
                                    LOG_D("[YtMsg] recv %s msg: %d\n", (message->which_values == YtMsg_rpc_tag ? "rpc" : ""), (message->which_values == YtMsg_rpc_tag ? message->values.rpc.func : -1));
                                #else
                                    LOG_I("[YtMsg] recv len=%d\n", mMsgLen);
                                #endif
                                return message;
                            }
                            else
                            {
                                LOG_E("[YtMsg] Decoding failed: %s\n", PB_GET_ERROR(&stream));
                            }
#else
                            uint32_t len;
                            return getPbField(101, &len);
#endif

                            return NULL;
                        }
                        mStatus = (YtDataLinkStatus)((int)mStatus + 1);
                        break;

                    default:
                        LOG_E("[YtMsg] Error: unknown status %d\n", mStatus);
                        break;
                }
            }
        }
        else if (readCnt < 0)
        {
            LOG_E("[YtMsg] serial read error %d\n", readCnt);
        }
        // write(mPortFd, &ch, 1);
    }

    return NULL;
}

static uint64_t decode_varint(const uint8_t *buf, size_t *i)
{
    pb_byte_t byte;
    uint_fast8_t bitpos = 0;
    uint64_t result = 0;
    do
    {
        byte = buf[(*i) ++];
        result |= (uint64_t)(byte & 0x7F) << bitpos;
        bitpos = (uint_fast8_t)(bitpos + 7);
    } while (byte & 0x80);
    return result;
}

const uint8_t *YtDataLink::getPbField(int tag, uint32_t *len)
{
    size_t i = 1;
    // 跳进整个{}
    size_t objend = decode_varint(mBuf, &i) + i;
    while (i < MIN(objend, sizeof(mBuf)))
    {
        uint64_t field = decode_varint(mBuf, &i);
        uint8_t type = (field & 0x7);
        // LOG_E("i=%d, tag=%d, type=%d\n", i, (int)(field >> 3), type);
        if (tag == (field >> 3))
        {
            *len = decode_varint(mBuf, &i);
            return &mBuf[i];
        }
        uint64_t objlen = decode_varint(mBuf, &i);
        if (type == 2) //Length-delimited
        {
            // 跳过内容
            i += objlen;
        }
    }
    return NULL;
}

CONST static uint16_t ccitt_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

ModelPath::ModelPath(int a, int b, int c, int d)
{
    len = 0;
    path[0] = a;
    path[1] = b;
    path[2] = c;
    path[3] = d;
    if (d < 0) len = 3;
    if (c < 0) len = 2;
    if (b < 0) len = 1;
    if (a < 0) len = 0;
}

YtDataLink::YtDataLink(YtSerialPortBase *port)
{
    mStatus = YT_DL_IDLE;
    mMsgLen = 0;
    mCrc = 0;
    mBufi = 0;
    mTrans = false;
    bufRemain = 0;
    bufStart = 0;
#if defined(YTMSG_FULL) || defined(YTMSG_LITE)
    mSendBufi = 0;
#endif
    mPort = port;
}
YtDataLink::~YtDataLink()
{
    mPort->close();
    delete mPort;
}
void YtDataLink::crcUpdate(uint8_t ch, bool first)
{
    if (first)
    {
        mCrcCalc = 0xffff;
    }
    mCrcCalc = ACCESS_CONST(ccitt_table[(mCrcCalc >> 8 ^ ch) & 0xff]) ^ (mCrcCalc << 8);
}

#if defined(YTMSG_FULL) || defined(YTMSG_LITE)
void YtDataLink::crcSendUpdate(uint8_t ch, bool first)
{
    if (first)
    {
        mCrcSendCalc = 0xffff;
    }
    mCrcSendCalc = ACCESS_CONST(ccitt_table[(mCrcSendCalc >> 8 ^ ch) & 0xff]) ^ (mCrcSendCalc << 8);
}
void YtDataLink::writeOneByte(uint8_t ch, bool trans, bool first, bool flush)
{
    if (mSendBufi >= sizeof(mSendBuf) - 2)
    {
        mPort->write(mSendBuf, mSendBufi);
        mSendBufi = 0;
    }
    if ( (ch == SOF || ch == TRANS) && trans )
    {
        mSendBuf[mSendBufi ++] = TRANS;
        mSendBuf[mSendBufi ++] = ch ^ TRANS;
    }
    else
    {
        mSendBuf[mSendBufi ++] = ch;
    }
    if (flush)
    {
        mPort->write(mSendBuf, mSendBufi);
        mSendBufi = 0;
    }

    crcSendUpdate(ch, first);
}
void YtDataLink::sendYtMsg(YtMsg *message)
{
    // uint8_t mMsgBuf[YtMsg_size]; //翻车现场……stack一般都只有80K，越界也无提示，仅crash概率增加

    if (!mPort->isOpen())
    {
        mPort->open();
        if (!mPort->isOpen())
            return;
    }

    /* Create a stream that will write to our buffer. */
    pb_ostream_t stream = pb_ostream_from_buffer(mMsgBuf, sizeof(mMsgBuf));

    /* Now we are ready to encode the message! */
    bool status = pb_encode(&stream, YtMsg_fields, message);
    uint32_t message_length = stream.bytes_written;
    LOG_I("[YtMsg] len=%d\n", message_length);

    if (status)
    {
        writeOneByte(SOF, false);
        writeOneByte((message_length >> 16) & 0xff, true, true);
        writeOneByte((message_length >> 8) & 0xff);
        writeOneByte((message_length >> 0) & 0xff);
        {
            uint16_t crc = mCrcSendCalc;
            writeOneByte((crc >> 8) & 0xff);
            writeOneByte(crc & 0xff);
        }
        for (size_t i = 0; i < message_length; i++)
        {
            writeOneByte(mMsgBuf[i], true, i == 0);
        }
        {
            uint16_t crc = mCrcSendCalc;
            writeOneByte((crc >> 8) & 0xff);
            writeOneByte(crc & 0xff, true, false, true);
        }
    }
    else
    {
        LOG_E("[YtMsg] Encoding failed: %s\n", PB_GET_ERROR(&stream));
    }
}
#endif

YtMsg *YtDataLink::alloc()
{
    YtMsg *ret = new YtMsg();
    memset(ret, 0, sizeof(YtMsg));
    return ret;
}

// len_path, model, idx, model, idx, ..., len_data, data0, ...
// 2, VS_MODEL_FACE_DETECTION, 0, 12, ...
// 3, VS_MODEL_FACE_DETECTION, 0, VS_MODEL_FACE_POSE, 6, ...
// 1, MODEL_CUSTOM_LANE, 8, ...
bool unpackInt16(const uint8_t *&p, uint8_t len, int16_t *ret)
{
    if (len < 2) return false;
    *ret = *(p+1);
    *ret <<= 8;
    *ret |= *(p);
    p += 2;
    return true;
}
bool unpackUInt16(const uint8_t *&p, uint8_t len, uint16_t *ret)
{
    if (len < 2) return false;
    *ret = *(p+1);
    *ret <<= 8;
    *ret |= *(p);
    p += 2;
    return true;
}
bool unpackVarUInt32(const uint8_t *&p, uint32_t *ret)
{
    // LEB128
    *ret = 0;
    uint8_t shift = 0;
    while (true) {
        uint32_t byte = *(p ++);
        *ret |= (byte & 0x7f) << shift;
        if ((byte & 0x80) == 0)
            break;
        shift += 7;
    }
    return true;
}
uint8_t packVarUInt32(uint8_t *&p, uint32_t val)
{
    // LEB128
    uint8_t ret = 0;
    do {
        uint32_t byte = val & 0x7f;
        val >>= 7;
        if (val != 0) /* more bytes to come */
            byte |= 0x80;
        *(p ++) = byte & 0xff;
        ret ++;
    } while (val != 0);
    return ret;
}

const uint8_t* YtDataLink::getResult(const uint8_t *buf, uint32_t &ret_len, const class ModelPath &path)
{
    const uint8_t *p = buf + 1;
    for (size_t i = 0; i < buf[0]; i++)
    {
        uint8_t cur_path_len = p[0];
        p += 1;
        const uint8_t *p_path = p;
        p += cur_path_len;
        uint8_t cur_data_type = p[0];
        p += 1;
        uint32_t cur_data_len;
        unpackVarUInt32(p, &cur_data_len);
        if (cur_path_len == path.len)
        {
            if (memcmp(p_path, path.path, path.len) == 0)
            {
                ret_len = cur_data_len;
                return p;
            }
        }
        p += cur_data_len;
    }
    ret_len = 0;
    return NULL;
}
void YtDataLink::initDataV2(YtMsg *message)
{
#if defined(YTMSG_FULL)
    message->values.result.has_dataV2 = true;
    pb_bytes_array_t *data = VSRESULT_DATAV2(message);

    uint8_t *buf = data->bytes;
    uint8_t *p = buf + 1;
    buf[0] = 0;
    data->size = p - buf;
#endif
}
void YtDataLink::addResultRaw(pb_bytes_array_t *data, uint32_t size, const class ModelPath &path, YtVisionSeedResultType _type, const uint8_t *content, const uint32_t len)
{
    uint8_t *buf = data->bytes;
    const uint8_t *p = buf + 1;
    if (data->size + len + 4 > size)
    {
         LOG_E("[addResult] " COLOR_RED "full" COLOR_NO " cnt=%d, sz = %d + %d > %d\n", buf[0], data->size, len + 4, size);
         return;
    }
    for (size_t i = 0; i < buf[0]; i++)
    {
        uint8_t cur_path_len = p[0];
        p += 1;
        const uint8_t *p_path = p;
        p += cur_path_len;
        uint8_t cur_data_type = p[0];
        p += 1;
        uint32_t cur_data_len;
        unpackVarUInt32(p, &cur_data_len);
        p += cur_data_len;
    }
    uint8_t *pp = (uint8_t*)p;
    pp[0] = path.len;
    pp += 1;
    memcpy(pp, path.path, path.len);
    pp += path.len;
    pp[0] = _type;
    pp += 1;
    packVarUInt32(pp, len);
    memcpy(pp, content, len);
    pp += len;
    buf[0] ++;
    data->size = pp - buf;
    LOG_I("[addResult] cnt=%d, sz=%d / %d\n", buf[0], data->size, size);
}
bool YtDataLink::getResult(const uint8_t *buf, YtVisionSeedResultTypeRect *rect, const class ModelPath &path)
{
    uint32_t len = 0;
    const uint8_t *p0 = getResult(buf, len, path);
    if (p0 == NULL)
    {
        return false;
    }
    const uint8_t *p = p0;
    uint16_t conf;
    if (!unpackUInt16(p, len - (p - p0), &conf)) return false;
    rect->conf = conf / 65535.f;
    if (!unpackUInt16(p, len - (p - p0), &rect->cls)) return false;
    if (!unpackInt16(p, len - (p - p0), &rect->x)) return false;
    if (!unpackInt16(p, len - (p - p0), &rect->y)) return false;
    if (!unpackInt16(p, len - (p - p0), &rect->w)) return false;
    if (!unpackInt16(p, len - (p - p0), &rect->h)) return false;
    return true;
}
bool YtDataLink::getResult(const uint8_t *buf, YtVisionSeedResultTypeClassification *result, const class ModelPath &path)
{
    uint32_t len = 0;
    const uint8_t *p0 = getResult(buf, len, path);
    if (p0 == NULL)
    {
        return false;
    }
    const uint8_t *p = p0;
    uint16_t conf;
    if (!unpackUInt16(p, len - (p - p0), &conf)) return false;
    result->conf = conf / 65535.f;
    if (!unpackUInt16(p, len - (p - p0), &result->cls)) return false;
    return true;
}
bool YtDataLink::getResult(const uint8_t *buf, YtVisionSeedResultTypeArray *result, const class ModelPath &path)
{
    uint32_t len = 0;
    const uint8_t *p0 = getResult(buf, len, path);
    if (p0 == NULL)
    {
        return false;
    }
    const uint8_t *p = p0;
    result->p = (const float*)p;
    result->count = len / 4;
    return true;
}
bool YtDataLink::getResult(const uint8_t *buf, uint32_t *result, const class ModelPath &path)
{
    uint32_t len = 0;
    const uint8_t *p0 = getResult(buf, len, path);
    if (p0 == NULL)
    {
        return false;
    }
    const uint8_t *p = p0;
    unpackVarUInt32(p, result);
    return true;
}
bool YtDataLink::getResult(const uint8_t *buf, YtVisionSeedResultTypeString *result, const class ModelPath &path)
{
    uint32_t len = 0;
    const uint8_t *p0 = getResult(buf, len, path);
    if (p0 == NULL)
    {
        return false;
    }
    const uint8_t *p = p0;
    uint16_t conf;
    if (!unpackUInt16(p, len - (p - p0), &conf)) return false;
    result->conf = conf / 65535.f;
    result->p = (const char*)p;
    return true;
}

bool YtDataLink::getResult(const uint8_t *buf, YtVisionSeedResultTypePoints *result, const class ModelPath &path)
{
    uint32_t len = 0;
    const uint8_t *p0 = getResult(buf, len, path);
    if (p0 == NULL)
    {
        return false;
    }
    const uint8_t *p = p0;
    result->count = len / 4;
    result->p = (YtVisionSeedResultTypePoints::Point*)p;
    return true;
}

void YtDataLink::addResult(pb_bytes_array_t *data, uint32_t size, const class ModelPath &path, const YtVisionSeedResultTypeRect &result)
{
    uint16_t conf = result.conf*65535;
    uint8_t buf[] = {
        (uint8_t)(conf), (uint8_t)(conf >> 8),
        (uint8_t)(result.cls), (uint8_t)(result.cls >> 8),
        (uint8_t)(result.x), (uint8_t)(result.x >> 8),
        (uint8_t)(result.y), (uint8_t)(result.y >> 8),
        (uint8_t)(result.w), (uint8_t)(result.w >> 8),
        (uint8_t)(result.h), (uint8_t)(result.h >> 8),
    };
    addResultRaw(data, size, path, YT_RESULT_RECT, buf, sizeof(buf));
}
void YtDataLink::addResult(pb_bytes_array_t *data, uint32_t size, const class ModelPath &path, const YtVisionSeedResultTypeClassification &result)
{
    uint16_t conf = result.conf*65535;
    uint8_t buf[] = {
        (uint8_t)(conf), (uint8_t)(conf >> 8),
        (uint8_t)(result.cls), (uint8_t)(result.cls >> 8),
    };
    addResultRaw(data, size, path, YT_RESULT_CLASSIFICATION, buf, sizeof(buf));
}
void YtDataLink::addResult(pb_bytes_array_t *data, uint32_t size, const class ModelPath &path, const YtVisionSeedResultTypeArray &result)
{
    uint32_t len = result.count * 4;
    addResultRaw(data, size, path, YT_RESULT_ARRAY, (const uint8_t *)result.p, len);
}
void YtDataLink::addResult(pb_bytes_array_t *data, uint32_t size, const class ModelPath &path, const YtVisionSeedResultTypeString &result)
{
#if defined(YTMSG_FULL)
    uint16_t conf = result.conf*65535;
    uint32_t len = strlen(result.p);
    uint8_t buf[len + 1 + 2] = {(uint8_t)(conf), (uint8_t)(conf >> 8)};
    memcpy(buf + 2, result.p, len);
    buf[2 + len] = 0;
    addResultRaw(data, size, path, YT_RESULT_STRING, buf, len + 1 + 2);
#else
    LOG_E("[E] Not implemented!\n");
#endif
}
void YtDataLink::addResult(pb_bytes_array_t *data, uint32_t size, const class ModelPath &path, const YtVisionSeedResultTypePoints &result)
{
    addResultRaw(data, size, path, YT_RESULT_POINTS, (const uint8_t*)result.p, result.count * 4);
}
void YtDataLink::addResult(pb_bytes_array_t *data, uint32_t size, const class ModelPath &path, const uint32_t result)
{
    uint8_t buf[17];
    uint8_t *p = buf;
    uint8_t len = packVarUInt32(p, result);
    addResultRaw(data, size, path, YT_RESULT_VARUINT32, buf, len);
}
