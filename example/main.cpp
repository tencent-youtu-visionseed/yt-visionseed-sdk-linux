#include <iostream>
#include <string>
#include <stdio.h>
#include <inttypes.h>

#include "YtVisionSeed.h"
#include "YtFaceAlignment.h"

YtVisionSeed *visionseed;

void OnResult(shared_ptr<YtMsg> message)
{
    if (message)
    {
        if (message->which_values == YtMsg_result_tag &&
            VSRESULT_DATAV2(message)->size > 0)
        {
            // 从message中提取人脸检测结果，这里的VS_MODEL_FACE_DETECTION==1对应工具中“AI模型配置”标签中人脸检测模型的序号“1”
            uint8_t vs_path[3] = {VS_MODEL_FACE_DETECTION};
            uint32_t count = 0; // 检测到的人脸数量
            YtDataLink::getResult(VSRESULT_DATAV2(message)->bytes, &count, vs_path, 1);
            // visionseed->SetFlasher( count == 0 ? 0 : 50, 0 );
            for (int i = 0; i < min((uint32_t)1, count); i ++)
            {
                YtVisionSeedResultTypeRect rect;
                YtVisionSeedResultTypeString faceName = {.conf = 0, .p=0};
                YtVisionSeedResultTypeArray pose = {.count =0, .p = 0};
                YtVisionSeedResultTypePoints points = {.count = 0, .p = 0};

                vs_path[1] = i; // 如果i==0，获取的算法结果为[VS_MODEL_FACE_DETECTION, 0], 即人脸框0。
                if (!YtDataLink::getResult(VSRESULT_DATAV2(message)->bytes, &rect, vs_path, 2))
                {
                    continue;
                }
                vs_path[2] = VS_MODEL_FACE_RECOGNITION; // 如果i==0，获取的算法结果为[VS_MODEL_FACE_DETECTION, 0, VS_MODEL_FACE_RECOGNITION]，即人脸框0的识别结果
                YtDataLink::getResult(VSRESULT_DATAV2(message)->bytes, &faceName, vs_path, 3);

                vs_path[2] = VS_MODEL_FACE_POSE; // 如果i==0，获取的算法结果为[VS_MODEL_FACE_DETECTION, 0, VS_MODEL_FACE_POSE]，即人脸框0的pose
                YtDataLink::getResult(VSRESULT_DATAV2(message)->bytes, &pose, vs_path, 3);

                vs_path[2] = VS_MODEL_FACE_LANDMARK; // 如果i==0，获取的算法结果为[VS_MODEL_FACE_DETECTION, 0, VS_MODEL_FACE_LANDMARK]，即人脸框0的90个面部关键点
                YtDataLink::getResult(VSRESULT_DATAV2(message)->bytes, &points, vs_path, 3);

                printf("face: %d/%d\n", (int)(i+1), count);

                // 输出人脸ID
                const char* name = faceName.p;
                if (name)
                {
                    float confidence = faceName.conf;
                    printf("Who: %s (confidence: %0.3f)\n", name, confidence);
                }
                // 对于检测到的每个人脸信息，输出:
                // (1)人脸框信息
                // (2)人脸的姿态信息roll pitch yaw
                // (3)根据眼部配准点计算眼睛的开闭状态，用来演示90点配准信息的使用
                // 输出人脸框信息
                printf("faceRect, x:%d y:%d, width:%d, height:%d\n", rect.x, rect.y, rect.w, rect.h);

                // 输出人脸姿态信息，右手坐标系定义：左耳到右耳方向为X轴正方向，下巴到头顶方向为Y轴正方向，前方到后方为Z轴正方向
                /**
                pitch	float 脸部俯仰角，绕X轴旋转角度，取值范围-90~90，在-30-30内效果准确，角度偏移过大可能造成检测失败或者点位不准
                yaw	    float 脸部左右扭头角度，绕Y轴旋转角度，取值范围-90~90，在-30-30内效果准确，角度偏移过大可能造成检测失败或者点位不准
                roll	float 脸部左右倾斜角度，绕Z轴旋转角度，取值范围-180~180，在-60-60内效果准确，角度偏移过大可能造成检测失败或者点位不准
                */
                if (pose.count == 3)
                {
                    printf("roll:%0.3f, yaw:%0.3f, pitch:%0.3f\n", pose.p[0], pose.p[1], pose.p[2]);
                }

                // 根据眼部配准点计算眼睛的开闭
                /** 左眼配准点序号
                 *
                 *         7   6   5
                 *    0                 4
                 *         1   2   3
                    右眼配准点序号
                *
                *         5   6   7
                *    4                 0
                *         3   2   1
                */
                // 从接收到的关键点构造shape结构体,填入关键点
                if (points.count == 90)
                {
                    YtFaceShape shape;
                    cv::Point2f *raw = (cv::Point2f *)&shape;
                    for (size_t i = 0; i < points.count; i++)
                    {
                        raw[i].x = points.p[i].x;
                        raw[i].y = points.p[i].y;
                    }
                    float leftEyeOpeness = cv::norm(shape.leftEye[6] - shape.leftEye[2]) / cv::norm(shape.leftEye[0] - shape.leftEye[4]);
                    printf("leftEyeOpeness: %0.3f\n", leftEyeOpeness);
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    visionseed = new YtVisionSeed("/dev/ttyACM0");

    //注册结果获取接口
    visionseed->RegisterOnResult(OnResult);
    // visionseed->RegisterOnStatus(OnResult);
    while ( true )
    {
        usleep(1000);
    }

    return 0;
}
