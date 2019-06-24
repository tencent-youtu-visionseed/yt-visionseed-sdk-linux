#ifndef __YT_FACE_ALIGNMENT_H__
#define __YT_FACE_ALIGNMENT_H__

#ifndef DISABLE_SIMPLE_CV
#include "SimpleCV.h"
#endif

/** The face profile landmarks ordering
 *
 *     0                            20
 *     1                            19
 *      2                          18
 *      3                          17
 *       4                        16
 *        5                     15
 *         6                   14
 *          7                 13
 *            8              12
 *              9         11
 *                   10
 */

/** The left eye and eyebrow landmarks ordering
 *
 *         7   6   5
 *    0                 4
 *         1   2   3
 */

/** The right eye and eyebrow landmarks ordering
 *
 *         5   6   7
 *    4                 0
 *         3   2   1
 */

/** The nose landmarks ordering
 *
 *           1
 *        2    12
 *      3        11
 *     4     0    10
 *    5  6  7  8   9
 */

/** The mouth landmarks ordering
 *
 *                 10   9   8
 *          11                      7
 *            21  20  19 18 17
 *    0                                    6
 *            12  13  14 15 16
 *           1                      5
 *                  2   3   4
 */

/** The pupils ordering
 *
 *       0                          1
 */

#define YT_FACE_SHAPE_SIZE_LEFT_EYEBROW 8
#define YT_FACE_SHAPE_SIZE_RIGHT_EYEBROW 8
#define YT_FACE_SHAPE_SIZE_LEFT_EYE 8
#define YT_FACE_SHAPE_SIZE_RIGHT_EYE 8
#define YT_FACE_SHAPE_SIZE_NOSE 13
#define YT_FACE_SHAPE_SIZE_MOUTH 22
#define YT_FACE_SHAPE_SIZE_PROFILE 21
#define YT_FACE_SHAPE_SIZE_PUPIL 2

/************************
 * @brief 人脸配准关键点记录结构体, 具体结构参考以上的结构体定义
 * faceProfile: 人脸轮廓
 * leftEyebrow: 左眉毛
 * rightEyebrow: 右眉毛
 * leftEye: 左眼
 * rightEye: 右眼
 * nose: 鼻子
 * mouth: 嘴巴
 * pupil: 眼珠
 ************************/
#pragma pack(1)
struct YtFaceShape
{
    cv::Point2f leftEyebrow      [YT_FACE_SHAPE_SIZE_LEFT_EYEBROW];
    cv::Point2f rightEyebrow     [YT_FACE_SHAPE_SIZE_RIGHT_EYEBROW];
    cv::Point2f leftEye          [YT_FACE_SHAPE_SIZE_LEFT_EYE];
    cv::Point2f rightEye         [YT_FACE_SHAPE_SIZE_RIGHT_EYE];
    cv::Point2f nose             [YT_FACE_SHAPE_SIZE_NOSE];
    cv::Point2f mouth            [YT_FACE_SHAPE_SIZE_MOUTH];
    cv::Point2f faceProfile      [YT_FACE_SHAPE_SIZE_PROFILE];
    cv::Point2f pupil            [YT_FACE_SHAPE_SIZE_PUPIL];
    float leftEyebrowVisibility  [YT_FACE_SHAPE_SIZE_LEFT_EYEBROW];
    float rightEyebrowVisibility [YT_FACE_SHAPE_SIZE_RIGHT_EYEBROW];
    float leftEyeVisibility      [YT_FACE_SHAPE_SIZE_LEFT_EYE];
    float rightEyeVisibility     [YT_FACE_SHAPE_SIZE_RIGHT_EYE];
    float noseVisibility         [YT_FACE_SHAPE_SIZE_NOSE];
    float mouthEyebrowVisibility [YT_FACE_SHAPE_SIZE_MOUTH];
    float faceProfileVisibility  [YT_FACE_SHAPE_SIZE_PROFILE];
    float pupilVisibility        [YT_FACE_SHAPE_SIZE_PUPIL];
};
#pragma pack()

int GetYtFaceShape(Face *face, YtFaceShape &shape);

#endif
