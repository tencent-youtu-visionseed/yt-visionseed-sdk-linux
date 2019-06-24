#include <iostream>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "SDKWrapper.h"
#include "YtFaceAlignment.h"

void OnFaceRetrieveResult(shared_ptr<YtMsg> message)
{
    // 对于检测到的每个人脸信息，输出:
    // (1)人脸框信息
    // (2)人脸的姿态信息roll pitch yaw
    // (3)根据眼部配准点计算眼睛的开闭状态，用来演示90点配准信息的使用
    printf("frame: %lu\n", VSRESULT(message).frameId);
    for (size_t i = 0; i < VSRESULT_DATA(message).faceDetectionResult.face_count; i++)
    {
        printf("face: %d/%d\n", (int)(i+1), VSRESULT_DATA(message).faceDetectionResult.face_count);
        // 输出人脸框信息
        const Rect& rect = VSRESULT_DATA(message).faceDetectionResult.face[i].rect;
        printf("faceRect, x:%d y:%d, width:%d, height:%d\n", rect.x, rect.y, rect.w, rect.h);

        // 输出人脸姿态信息，右手坐标系定义：左耳到右耳方向为X轴正方向，下巴到头顶方向为Y轴正方向，前方到后方为Z轴正方向
        /**
        pitch	float 脸部俯仰角，绕X轴旋转角度，取值范围-90~90，在-30-30内效果准确，角度偏移过大可能造成检测失败或者点位不准
        yaw	    float 脸部左右扭头角度，绕Y轴旋转角度，取值范围-90~90，在-30-30内效果准确，角度偏移过大可能造成检测失败或者点位不准
        roll	float 脸部左右倾斜角度，绕Z轴旋转角度，取值范围-180~180，在-60-60内效果准确，角度偏移过大可能造成检测失败或者点位不准
        */
        if (VSRESULT_DATA(message).faceDetectionResult.face[i].has_pose)
        {
            const Pose& p = VSRESULT_DATA(message).faceDetectionResult.face[i].pose;
            printf("roll:%0.3f, pitch:%0.3f, yaw:%0.3f\n", p.roll, p.pitch, p.yaw);
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
        YtFaceShape shape;
        if ( GetYtFaceShape(&(VSRESULT_DATA(message).faceDetectionResult.face[i]), shape) )
        {
            float leftEyeOpeness = cv::norm(shape.leftEye[6] - shape.leftEye[2]) / cv::norm(shape.leftEye[0] - shape.leftEye[4]);
            printf("leftEyeOpeness: %0.3f\n", leftEyeOpeness);
        }
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    SDKWrapper wrapper("/dev/ttyACM0");

    //注册结果获取接口
    wrapper.RegisterOnFaceResult(OnFaceRetrieveResult);
    while ( true )
    {
        usleep(1000);
    }

    return 0;
}
