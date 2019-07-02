#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "YtVisionSeed.h"

YtMessenger* YtVisionSeed::messenger = NULL;
pb_byte_t* YtVisionSeed::mBuffer = NULL;

YtVisionSeed::YtVisionSeed(const char* dev)
{
    if ( messenger ) {
        delete(messenger);
        messenger = NULL;
    }
    messenger = new YtMessenger(dev);
    // REGISTER_CALLBACK(sendBlob);
    messenger->startLoop();

    LOG_D("[YtVisionSeed] initialized.\n");
}

YtVisionSeed::~YtVisionSeed()
{
    LOG_D("[YtVisionSeed] releasing, Pid: %d, tid: %ld.\n", getpid(), gettid());

    if ( messenger != NULL ) {
        messenger->exitLoop();
        delete(messenger);
        messenger = NULL;
    }
    LOG_D("[YtVisionSeed] released.\n");
}

std::string YtVisionSeed::GetDeviceInfo()
{
    VSRPC(request, getDeviceInfo, intParams, response);

    if ( VSRPC_CALL(request, response) ) {
        if (VSRPC_DATA(response).strData)
            return std::string(VSRPC_DATA(response).strData);
        else
            return "";
    }
    else {
        return "";
    }
}

bool YtVisionSeed::UpgradeFirmware(std::string path)
{
    // send file & do upgrade
    std::string remotePath = "/tmp/fw.bin";
    if (!messenger->SendFile(path, remotePath.c_str()))
    {
        return false;
    }

    VSRPC(request, upgradeFirmware, strParams, response);
    strcpy(VSRPC_PARAM(request).strParams, remotePath.c_str());

    return VSRPC_CALL(request, response);
}

bool YtVisionSeed::SetCamAutoExposure(CameraID camId)
{
    return SetCameraExposureParams(camId, CameraExposureParams_ExposureType_AUTO, 0, 0);
}

bool YtVisionSeed::SetCamManualExposure(CameraID camId, int32_t timeUs, int32_t gain)
{
    return SetCameraExposureParams(camId, CameraExposureParams_ExposureType_MANUAL, timeUs, gain);
}

bool YtVisionSeed::SetCameraExposureParams(CameraID camId, CameraExposureParams_ExposureType type, int32_t timeUs, int32_t gain)
{
    VSRPC(request, setExposure, cameraExposureParams, response);
    VSRPC_PARAM(request).cameraExposureParams.camId = (int32_t)camId;
    VSRPC_PARAM(request).cameraExposureParams.type = type;
    if (type == CameraExposureParams_ExposureType_MANUAL) {
        VSRPC_PARAM(request).cameraExposureParams.has_timeUs = true;
        VSRPC_PARAM(request).cameraExposureParams.has_gain = true;
        VSRPC_PARAM(request).cameraExposureParams.timeUs = timeUs;
        VSRPC_PARAM(request).cameraExposureParams.gain = gain;
    }

    return VSRPC_CALL(request, response);
}

bool YtVisionSeed::SetFlasher(int32_t flasherIR, int32_t flasherWhite)
{
    VSRPC(request, setFlasher, flasherParams, response);

    if ( flasherIR>=0 ) {
        VSRPC_PARAM(request).flasherParams.has_ir = true;
        VSRPC_PARAM(request).flasherParams.ir = flasherIR;
    }

    if ( flasherWhite>=0 ) {
        VSRPC_PARAM(request).flasherParams.has_white = true;
        VSRPC_PARAM(request).flasherParams.white = flasherWhite;
    }

    return VSRPC_CALL(request, response);
}

// bool YtVisionSeed::TakePicture(CameraID camId, int32_t mode, std::string pathHost)
// {
//     VSRPC(request, takeRawPicture, intParams, response);
//     VSRPC_PARAM(request).intParams = (int32_t)camId;
//
//     if ( VSRPC_CALL(request, response) ) {
//         //return VSRPC_DATA(response).strData;
//         LOG_D("[YtVisionSeed] take photo success.\n");
//     }
//     else {
//         LOG_E("[YtVisionSeed] take photo failed.\n");
//     }
// }

// /*
// *
// *       AI
// */
// bool YtVisionSeed::SetFaceAIAbility(int32_t ability)
// {
//     VSRPC(request, setFaceAIAbility, intParams, response);
//     VSRPC_PARAM(request).intParams = ability;
//
//     return VSRPC_CALL(request, response);
// }

// /*
// *
// *       Face Feature Database
// */
//
// std::string YtVisionSeed::GetFaceLibsInfo()
// {
//     VSRPC(request, getFaceLibsInfo, intParams, response);
//
//     if ( VSRPC_CALL(request, response) ) {
//         return VSRPC_DATA(response).strData;
//     }
//     else {
//         return "";
//     }
// }
//
// bool YtVisionSeed::ClearFaceLibs()
// {
//     VSRPC(request, clearFaceLibs, intParams, response);
//
//     return VSRPC_CALL(request, response);
// }
//

// bool YtVisionSeed::RegisterFaceIdWithPic(std::string path, std::string faceId)
// {
//     // send file & do register
//     std::size_t extPos = path.rfind(".");
//     if (extPos == std::string::npos)
//     {
//         return false;
//     }
//     std::string ext = path.substr(extPos);
//     std::string remotePath = std::string("/tmp/") + faceId + ext;
//     LOG_D("remotePath = %s\n", remotePath.c_str());
//     if (!messenger->SendFile(path, remotePath.c_str()))
//     {
//         return false;
//     }
//
//     VSRPC(request, registerFaceIdWithPic, registerFaceIdWithPicParams, response);
//     strcpy(VSRPC_PARAM(request).registerFaceIdWithPicParams.filePath, remotePath.c_str());
//     strcpy(VSRPC_PARAM(request).registerFaceIdWithPicParams.faceId, faceId.c_str());
//
//     return VSRPC_CALL(request, response);
// }
//
// bool YtVisionSeed::RegisterFaceIdFromCamera(std::string faceId)
// {
//     VSRPC(request, registerFaceIdFromCamera, strParams, response);
//     strcpy(VSRPC_PARAM(request).strParams, faceId.c_str());
//
//     return VSRPC_CALL(request, response);
// }

// bool YtVisionSeed::VerifyFaceWithPic(std::string path, int32_t libId, std::string faceId)
// {
//     // send file & do verify
//     return messenger->SendFile(BlobParams_BlobType_FACEVERIFY, path, libId, faceId);
// }
//
// bool YtVisionSeed::DeleteFaceId(std::string faceId)
// {
//     VSRPC(request, deleteFaceId, strParams, response);
//     strcpy(VSRPC_PARAM(request).strParams, faceId.c_str());
//
//     return VSRPC_CALL(request, response);
// }

void YtVisionSeed::RegisterOnFaceResult(OnResult callback)
{
    messenger->RegisterOnFaceResult(callback);
}
