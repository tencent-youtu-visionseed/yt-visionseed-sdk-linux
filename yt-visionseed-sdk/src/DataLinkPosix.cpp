//author: chenliang @ Youtu Lab, Tencent

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory>

#include <pb_encode.h>
#include <pb_decode.h>
#include "YtMsg.pb.h"

#include "DataLink.h"
#include "DataLinkPosix.h"
#include "IDataLinkObserver.h"
#include "YtMsgPool.h"

#ifdef __rtems__
#include <rtems.h>
#include <Flic.h>
#endif

#if !defined(strlcpy)
size_t strlcpy(char *dst, const char *src, size_t size)
{
	size_t srclen = strlen(src);
	strncpy(dst, src, min(size, srclen));
	dst[size - 1] = 0;
	return srclen;
}
#endif

#if defined(ESP32)
	#include "DataLinkESP32.h"
#elif defined(STM32)
    #include "DataLinkSTM32HAL.h"
    extern "C" int usleep(useconds_t us)
    {
        vTaskDelay(us /1000 / portTICK_PERIOD_MS);
        return 0;
    }
#endif

#ifdef INC_FREERTOS_H
#else
#include <termios.h>
#include <unistd.h>
int sem_timedwait(sem_t *sem, const int64_t ms)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_nsec += ms * 1000000;
    ts.tv_sec += ts.tv_nsec / 1000000000;
    ts.tv_nsec %= 1000000000;
    return sem_timedwait(sem, &ts);
}
#endif

vector<YtDataLinkPullPosix *> YtDataLinkPullPosix::mInstance;

int YtDataLinkPullPosix::createInstance(const char *dev)
{
    YtDataLinkPullPosix *inst = new YtDataLinkPullPosix(dev);
    mInstance.push_back(inst);
    inst->instanceId = mInstance.size() - 1;
    return inst->instanceId;
}
YtDataLinkPullPosix *YtDataLinkPullPosix::getInstance(int instanceId)
{
    return mInstance[instanceId];
}
void YtDataLinkPullPosix::AttachAll(IObserver *p)
{
    for (size_t i = 0; i < mInstance.size(); i++)
    {
        mInstance[i]->Attach(p);
    }
}
void YtDataLinkPullPosix::DetachAll(IObserver *p)
{
    for (size_t i = 0; i < mInstance.size(); i++)
    {
        mInstance[i]->Detach(p);
    }
}
YtDataLinkPullPosix::YtDataLinkPullPosix(const char *dev) : YtSubject(), YtThread("YtDataLinkPull"), YtDataLink(NULL)
{
#if defined(ESP32)
    mPort = (new YtSerialPortESP32(dev));
#elif defined(STM32)
    mPort = (new YtSerialPortSTM32HAL(dev));
#else
    mPort = (new YtSerialPortPosix(dev));
#endif

    start();
}
YtDataLinkPullPosix::~YtDataLinkPullPosix()
{
}
void *YtDataLinkPullPosix::run()
{
    while (!shouldExit)
    {
#ifdef YTMSG_USE_RECV_POOL
        std::shared_ptr<YtMsg> message = recvRunOnce();
        if (message)
        {
#else
        YtMsg *msg = recvRunOnce();
        if (msg)
        {
            std::shared_ptr<YtMsg> message(msg, [](YtMsg *p) { /*printf("[YtDataLink] release %p\n", p);*/ pb_release(YtMsg_fields, p); delete p; });
#endif
            Notify(instanceId, message);

            if (message->which_values == YtMsg_rpc_tag)
            {
                LOG_D("[YtDataLink] - YtMsg (%d)\n", message->values.rpc.func);
                if (message->values.rpc.which_params == YtRpc_cameraExposureParams_tag)
                {
                    LOG_D("[YtDataLink] | CameraExposureParams type is %d!\n", (int)message->values.rpc.params.cameraExposureParams.type);
                }
            }
        }
    }
    return NULL;
}





vector<YtDataLinkPushPosix *> YtDataLinkPushPosix::mInstance;
int YtDataLinkPushPosix::createInstance(YtDataLink *dev)
{
    YtDataLinkPushPosix *inst = new YtDataLinkPushPosix(dev);
    mInstance.push_back(inst);
    return mInstance.size() - 1;
}
YtDataLinkPushPosix *YtDataLinkPushPosix::getInstance(int instanceId)
{
    return mInstance[instanceId];
}
void YtDataLinkPushPosix::broadcastYtMsgAsync(shared_ptr<YtMsg> msg)
{
    for (size_t i = 0; i < mInstance.size(); i++)
    {
        mInstance[i]->sendYtMsgAsync(msg);
    }
}
YtDataLinkPushPosix::YtDataLinkPushPosix(YtDataLink *dev) : YtThread("YtDataLinkPush")
{
#ifdef INC_FREERTOS_H
    mUpdateSem = xSemaphoreCreateCounting(10, 1);
#else
    if (sem_init(&mUpdateSem, 0, 1) != 0)
    {
        LOG_E("[YtDataLink] Error: sem_init failed\n");
    }
#endif
    mDev = dev;
    //发送
    start();
}
YtDataLinkPushPosix::~YtDataLinkPushPosix()
{
}
void YtDataLinkPushPosix::sendYtMsgAsync(shared_ptr<YtMsg> msg)
{
    sem_wait(&mUpdateSem);
    if (mQueue.size() < 5)
    {
        mQueue.push(msg);
    }
    else
    {
        LOG_I("[YtDataLink] msg sending queue full, skipping\n");
    }
    sem_post(&mUpdateSem);
}
void YtDataLinkPushPosix::sendResponseAsync(YtRpcResponse_ReturnCode code, bool has_sequenceId, int32_t sequenceId)
{
    sendResponseWithIntDataAsync(code, 0, has_sequenceId, sequenceId);
}
void YtDataLinkPushPosix::sendResponseWithIntDataAsync(YtRpcResponse_ReturnCode code, int intData, bool has_sequenceId, int32_t sequenceId)
{
    std::shared_ptr<YtMsg> msg = YtMsgPool::getInstance()->receive();
    if (msg)
    {
        msg->which_values = YtMsg_response_tag;
        msg->values.response.which_data = YtRpcResponse_intData_tag;
        msg->values.response.data.intData = intData;

        msg->values.response.has_sequenceId = has_sequenceId;
        msg->values.response.sequenceId = sequenceId;
        msg->values.response.code = code;

        sendYtMsgAsync(msg);
    }
}
void YtDataLinkPushPosix::sendResponseWithStrDataAsync(YtRpcResponse_ReturnCode code, std::string strData, bool has_sequenceId, int32_t sequenceId)
{
    std::shared_ptr<YtMsg> msg = YtMsgPool::getInstance()->receive();
    if (msg)
    {
        msg->which_values = YtMsg_response_tag;
        msg->values.response.which_data = YtRpcResponse_strData_tag;
        strlcpy(msg->values.response.data.strData, strData.c_str(), sizeof(msg->values.response.data.strData));

        msg->values.response.has_sequenceId = has_sequenceId;
        msg->values.response.sequenceId = sequenceId;
        msg->values.response.code = code;

        sendYtMsgAsync(msg);
    }
}

void *YtDataLinkPushPosix::run()
{
    while (!shouldExit)
    {
        sem_wait(&mUpdateSem);
        shared_ptr<YtMsg> msg;
        if (mQueue.size() > 0)
        {
            msg = mQueue.front();
            mQueue.pop();
        }
        sem_post(&mUpdateSem);
        if (msg)
        {
            mDev->sendYtMsg(msg.get());
        }
        usleep(10000);
    }
    return NULL;
}





void *YtThread::threadFunc(void *args)
{
    YtThread *yt = (YtThread*)args;
    LOG_D("YtThread starting %p\n", yt);
    void *ret = yt->run();
    LOG_D("YtThread finished %p\n", yt);
    return ret;
}
YtThread::YtThread(string name) : mName(name)
{
}
YtThread::~YtThread()
{
}
void YtThread::start()
{
    shouldExit = false;
#ifdef INC_FREERTOS_H
#else
    int ret;
    pthread_attr_t attr;
    ret = pthread_attr_init(&attr);
    if (0 != ret)
    {
       LOG_E("pthread_attr_init error %d\n", ret);
       return;
    }
#endif
#ifdef __rtems__
    ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (0 != ret)
    {
        LOG_E("pthread_attr_set_schedule error %d\n", ret);
        return;
    }
#endif

#ifdef INC_FREERTOS_H
#ifdef STM32
    if (xTaskCreate((TaskFunction_t)threadFunc, mName.c_str(), 512, this, 5, &thread) != pdPASS)
    {
        LOG_E("create task %s failed.\n", mName.c_str());
        return;
    }
#else
    // ESP32平台，如果不绑定CPU核心，会发生奇怪的内存泄露（shared_ptr构析函数有概率不被执行，导致YtMsgPool不断增长）
    xTaskCreatePinnedToCore((TaskFunction_t)threadFunc, mName.c_str(), 4096, this, 5, &thread, 1);
    // xTaskCreate((TaskFunction_t)threadFunc, mName.c_str(), 4096, this, 5, &thread);
#endif
#else
    ret = pthread_attr_setschedpolicy (&attr, SCHED_RR);
    if (0 != ret)
    {
        LOG_E("pthread_attr_set_schedule_policy error %d\n", ret);
        return;
    }
    ret = pthread_create(&thread, &attr, threadFunc, this);
    if (0 != ret)
    {
        LOG_E("pthread_create error %d\n", ret);
        return;
    }
#endif

#ifdef __rtems__
    rtems_object_set_name(thread, mName.c_str()); //for moviDebug
#endif
#ifndef INC_FREERTOS_H
    pthread_setname_np(thread, mName.c_str()); //for rtems ps
#endif

#ifdef INC_FREERTOS_H
#else
    ret = pthread_attr_destroy(&attr);
    if (0 != ret)
    {
        LOG_E("pthread_attr_destroy error %d\n", ret);
        return;
    }
#endif
}
void YtThread::exit()
{
    shouldExit = true;
#ifdef INC_FREERTOS_H
    vTaskDelete(thread);
#else
    int ret = pthread_join(thread, NULL);
    if (ret != 0)
    {
        LOG_E("pthread_join error %d\n", ret);
    }
#endif
}




YtSerialPortPosix::YtSerialPortPosix(const char *dev) : YtSerialPortBase(dev)
{
}
YtSerialPortPosix::~YtSerialPortPosix()
{
}

bool YtSerialPortPosix::isOpen()
{
    //TODO: USB断开后处理
    return mPortFd >= 0;
}
int YtSerialPortPosix::open()
{
    mPortFd = ::open(mDev, O_RDWR);
    if (mPortFd < 0) {
        #ifndef INC_FREERTOS_H
            LOG_E("[YtDataLink] Error opening %s, %d\n", mDev, errno);
        #endif
        usleep(100000);
        return 0;
    }
    //有一些特殊的usb-serial设备（比如stm32 usb-serial）默认会设置标志位，将发送的0x0a变成0x0d, 0x0a
#if !defined(INC_FREERTOS_H)
    struct termios options;
    if (tcgetattr(mPortFd, &options) != -1)
    {
        options.c_cflag = (options.c_cflag & ~CSIZE) | CS8; // 8
        options.c_cflag = (options.c_cflag & ~PARENB); // N
        options.c_cflag = (options.c_cflag & ~CSTOPB); // 1
        options.c_cflag = (options.c_cflag & ~CRTSCTS); //流控
        options.c_lflag = (options.c_lflag & ~ECHO);
        // cfsetospeed(&options, B921600);
        cfsetospeed(&options, B115200);
        // cfsetospeed(&options, B9600);
        options.c_iflag &= ~(INLCR | ICRNL | IGNCR | IXON | IXOFF);
        options.c_oflag &= ~(ONLCR | OCRNL | ONOCR | ONLRET | OPOST);
        cfmakeraw(&options);
        tcsetattr(mPortFd, TCSANOW, &options);
    }
#endif
    LOG_D("[YtDataLink] %s opened: %d\n", mDev, mPortFd);
    return 1;
}
int YtSerialPortPosix::close()
{
    if (mPortFd != 0)
    {
        ::close(mPortFd);
        mPortFd = -1;
        return 1;
    }
    return 0;
}
int YtSerialPortPosix::read(void *buffer, size_t len)
{
    int rv;
#if !defined(__rtems__) && !defined(INC_FREERTOS_H)
    fd_set set;
    struct timeval timeout;
    FD_ZERO(&set); /* clear the set */
    FD_SET(mPortFd, &set); /* add our file descriptor to the set */

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    rv = select(mPortFd + 1, &set, NULL, NULL, &timeout);
    if(rv == -1)
    {
    }
    else if(rv == 0)
    {
    }
    else
    {
        rv = ::read(mPortFd, buffer, len);
    }
#else
    rv = ::read(mPortFd, buffer, len);
#endif
    if (rv < 0)
    {
        LOG_E("[serial] read error %d!", rv);
        close();
    }
    // //echo data for test
    // else
    // {
    //     for (int i = 0; i < rv; i ++)
    //     {
    //         printf("0x%02x\n", ((uint8_t*)buffer)[i]);
    //     }
    //     ::write(mPortFd, buffer, rv);
    // }
    return rv;
}
int YtSerialPortPosix::write(void *buffer, size_t len)
{
    int ret = 0;
#ifdef __rtems__
    uint32_t HglUartWrite(uint32_t base_addr, uint32_t len, const char *buf);
    if (strcmp(mDev, "/dev/ttyS3") == 0)
    {
        ret = HglUartWrite(0x201B0000, len, (const char *)buffer);
    }
    else
    {
        ret = ::write(mPortFd, buffer, len);
    }
#else
    ret = ::write(mPortFd, buffer, len);
#endif
    // for (size_t k = 0; k < len; k++)
    //     printf("0x%02x%c", ((uint8_t*)buffer)[k], k==len-1?'\n':',');
    if (ret <= 0)
    {
        LOG_E("[serial] write error %d!\n", ret);
        close();
    }
    return ret;
}
