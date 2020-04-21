#include <stdio.h>
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


int GetYtFaceShape(YtVisionSeedResultTypePoints &points, YtFaceShape &shape)
{
    if (points.count == 90)
    {
        cv::Point2f *raw = (cv::Point2f *)&shape;
        for (size_t i = 0; i < points.count; i++)
        {
            raw[i].x = points.p[i].x;
            raw[i].y = points.p[i].y;
        }
        float* pVisibility = (((float*)&shape)+points.count*2);
        for (size_t i = 0; i < points.count; i++)
        {
            pVisibility[i] = 1;
        }
        //
        shape.confidence = 1;
        return 1;
    }
    return 0;
}
