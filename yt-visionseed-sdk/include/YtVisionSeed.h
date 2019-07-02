#ifndef __VS_WRAPPER_H__
#define __VS_WRAPPER_H__

#include <map>
#include "YtMessenger.h"

// #define DO_FACE_TRACK       (0x00000001   <<  0)        // 't'
// #define DO_FACE_FEATURE     (0x00000001   <<  1)        // 'f'
// #define DO_FACE_RETRIEVE    (0x00000001   <<  2)        // 'r'
// #define DO_FACE_SCORE       (0x00000001   <<  3)        // 's'
// #define DO_FACE_LIVENESS    (0x00000001   <<  4)        // 'l'
// #define DO_FACE_GENDER      (0x00000001   <<  5)        // 'g'
// #define DO_FACE_AGE         (0x00000001   <<  6)        // 'a'
//
// #define DO_FACE_DETECTION       ( DO_FACE_TRACK )
// #define DO_FACE_RECOGNITION     ( DO_FACE_RETRIEVE )
// #define DO_FACE_ALL         ( DO_FACE_TRACK | DO_FACE_FEATURE | DO_FACE_RETRIEVE | DO_FACE_SCORE | DO_FACE_LIVENESS | DO_FACE_GENDER | DO_FACE_AGE )

#define DECLARE_CALLBACK(vs_rpc)    static void Process##vs_rpc##Msg(shared_ptr<YtMsg> message)
#define REGISTER_CALLBACK(vs_rpc)   messenger->RegisterCallback(YtRpc_Function_##vs_rpc, &YtVisionSeed::Process##vs_rpc##Msg)
#define DEFINE_CALLBACK(vs_rpc)     void YtVisionSeed::Process##vs_rpc##Msg(shared_ptr<YtMsg> message)

typedef enum
{
    CAMERA_RGB = 0,
    CAMERA_IR,
}CameraID;

class YtVisionSeed
{
public:
    virtual ~YtVisionSeed();
    YtVisionSeed(const char* dev);

    // DECLARE_CALLBACK(sendBlob);

    /***
    *
    *       Device
    */
    std::string GetDeviceInfo();
    bool UpgradeFirmware(std::string path);


    /*
    *
    *       Camera
    */
    //int SetCameraParams();
    bool SetCamAutoExposure(CameraID camId);
    bool SetCamManualExposure(CameraID camId, int32_t timeUs, int32_t gain);
    bool SetFlasher(int32_t flasherIR, int32_t flasherWhite);
    // bool SetCameraAIAbility(int32_t ability); // !!! Not implemented.
    // bool TakePicture(CameraID camId, int32_t mode, std::string pathHost);

    // /*
    // *
    // *       AI
    // */
    // bool SetFaceAIAbility(int32_t ability);
    void RegisterOnFaceResult(OnResult callback);

    // /*
    // *
    // *       Face Library
    // */
    // std::string GetFaceLibsInfo();
    // bool ClearFaceLibs();
    // bool RegisterFaceIdWithPic(std::string path, std::string faceId);
    // bool RegisterFaceIdFromCamera(std::string faceId);
    // bool VerifyFaceWithPic(std::string path, int32_t libId, std::string faceId);
    // bool DeleteFaceId(std::string faceId);

private:
    static YtMessenger *messenger;
    static pb_byte_t* mBuffer;

    bool SetCameraExposureParams(CameraID camId, CameraExposureParams_ExposureType type, int32_t timeUs, int32_t gain);
};

#endif
