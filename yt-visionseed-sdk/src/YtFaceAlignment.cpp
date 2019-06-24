#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include "pb_decode.h"
#include "YtMsg.pb.h"
#include "DataLink.h"

#include "YtFaceAlignment.h"

#define YT_FACE_ALIGMENT_IDX_LEFT_EYEBROW 0
#define YT_FACE_ALIGMENT_IDX_RIGHT_EYEBROW 8
#define YT_FACE_ALIGMENT_IDX_LEFT_EYE 16
#define YT_FACE_ALIGMENT_IDX_RIGHT_EYE 24
#define YT_FACE_ALIGMENT_IDX_NOSE 32
#define YT_FACE_ALIGMENT_IDX_MOUTH 45
#define YT_FACE_ALIGMENT_IDX_PROFILE 67
#define YT_FACE_ALIGMENT_IDX_PUPIL 88
#define YT_FACE_SHAPE_SIZE (YT_FACE_ALIGMENT_IDX_PUPIL + YT_FACE_SHAPE_SIZE_PUPIL)


int GetYtFaceShape(Face *face, YtFaceShape &shape)
{
    if (face->has_shape)
    {
        if (sizeof(YtFaceShape) != YT_FACE_SHAPE_SIZE*(sizeof(cv::Point2f)+sizeof(float)))
        {
            LOG_E("[GetYtFaceShape] %lu != %lu\n", sizeof(YtFaceShape), YT_FACE_SHAPE_SIZE*(sizeof(cv::Point2f)+sizeof(float)));
            return 0;
        }
        cv::Point2f *raw = (cv::Point2f *)&shape;
        for (size_t i = 0; i < face->shape.x_count; i++)
        {
            raw[i].x = face->shape.x[i];
            raw[i].y = face->shape.y[i];
        }
        //
        int ptsNum = face->shape.x_count;
        float* pVisibility = (((float*)&shape)+ptsNum*2);
        for (size_t i = 0; i < ptsNum; i++)
        {
            pVisibility[i] = face->shape.visibility[i];
        }
        
        return 1;
    }
    return 0;
}
