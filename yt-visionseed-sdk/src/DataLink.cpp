//author: chenliang @ Youtu Lab, Tencent

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>

#include <pb_encode.h>
#include <pb_decode.h>
#include "YtMsg.pb.h"

#include "DataLink.h"

#ifndef MIN
    #define MIN(a,b) ( ((a)<(b))?(a):(b) )
#endif

YtSerialPortBase::YtSerialPortBase(const char *dev)
{
    snprintf(mDev, sizeof(mDev), "%s", dev);
}
YtSerialPortBase::~YtSerialPortBase()
{
}
// #include <VcsHooksApi.h>

YtMsg *YtDataLink::recvRunOnce()
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
                            if ((mCrcCalc) != mCrc)
                            {
                                LOG_E("[YtMsg] Error: msg crc 0x%04x != 0x%04x\n", mCrcCalc, mCrc);
                                mStatus = YT_DL_IDLE;
                                return NULL;
                            }

                            YtMsg *message = alloc();
                            // LOG_D("[YtMsg] new %p\n", message.get());
                            // YtMsg message = YtMsg_init_zero;
                            /* Create a stream that reads from the buffer. */
                            pb_istream_t stream = pb_istream_from_buffer(mBuf, mMsgLen);

                            /* Now we are ready to decode the message. */
                            bool status = pb_decode(&stream, YtMsg_fields, message);

                            if (status)
                            {
                                mStatus = YT_DL_IDLE;
                                #ifdef __rtems__
                                    LOG_D("[YtMsg] recv %s msg: %d\n", (message->which_values == YtMsg_rpc_tag ? "rpc" : ""), (message->which_values == YtMsg_rpc_tag ? message->values.rpc.func : -1));
                                #else
                                    LOG_D("[YtMsg] recv len=%d\n", mMsgLen);
                                #endif
                                return message;
                            }
                            else
                            {
                                LOG_E("[YtMsg] Decoding failed: %s\n", PB_GET_ERROR(&stream));
                            }

                            mStatus = YT_DL_IDLE;
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
YtDataLink::YtDataLink(YtSerialPortBase *port)
{
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
YtMsg *YtDataLink::alloc()
{
    return new YtMsg();
}
