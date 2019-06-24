#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "SDKWrapper.h"

YtMessenger* SDKWrapper::messenger = NULL;
pb_byte_t* SDKWrapper::mBuffer = NULL;

SDKWrapper::SDKWrapper(const char* dev)
{
    if ( messenger ) {
        delete(messenger);
        messenger = NULL;
    }
    messenger = new YtMessenger(dev);
    // REGISTER_CALLBACK(sendBlob);
    messenger->startLoop();

    LOG_D("[SDKWrapper] initialized.\n");
}

SDKWrapper::~SDKWrapper()
{
    LOG_D("[SDKWrapper] releasing, Pid: %d, tid: %ld.\n", getpid(), gettid());

    if ( messenger != NULL ) {
        messenger->exitLoop();
        delete(messenger);
        messenger = NULL;
    }
    LOG_D("[SDKWrapper] released.\n");
}

std::string SDKWrapper::GetDeviceInfo()
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

bool SDKWrapper::UpgradeFirmware(std::string path)
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

bool SDKWrapper::SetCamAutoExposure(int32_t camId)
{
    return SetCameraExposureParams(camId, CameraExposureParams_ExposureType_AUTO, 0, 0);
}

bool SDKWrapper::SetCamManualExposure(int32_t camId, int32_t timeUs, int32_t gain)
{
    return SetCameraExposureParams(camId, CameraExposureParams_ExposureType_MANUAL, timeUs, gain);
}

bool SDKWrapper::SetCameraExposureParams(int32_t camId, CameraExposureParams_ExposureType type, int32_t timeUs, int32_t gain)
{
    VSRPC(request, setExposure, cameraExposureParams, response);
    VSRPC_PARAM(request).cameraExposureParams.camId = camId;
    VSRPC_PARAM(request).cameraExposureParams.type = type;
    if (type == CameraExposureParams_ExposureType_MANUAL) {
        VSRPC_PARAM(request).cameraExposureParams.has_timeUs = true;
        VSRPC_PARAM(request).cameraExposureParams.has_gain = true;
        VSRPC_PARAM(request).cameraExposureParams.timeUs = timeUs;
        VSRPC_PARAM(request).cameraExposureParams.gain = gain;
    }

    return VSRPC_CALL(request, response);
}

bool SDKWrapper::SetFlasher(int32_t flasherIR, int32_t flasherWhite)
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

// bool SDKWrapper::TakePicture(int32_t camId, int32_t mode, std::string pathHost)
// {
//     VSRPC(request, takeRawPicture, intParams, response);
//     VSRPC_PARAM(request).intParams = camId;
//
//     if ( VSRPC_CALL(request, response) ) {
//         //return VSRPC_DATA(response).strData;
//         LOG_D("[SDKWrapper] take photo success.\n");
//     }
//     else {
//         LOG_E("[SDKWrapper] take photo failed.\n");
//     }
// }

// /*
// *
// *       AI
// */
// bool SDKWrapper::SetFaceAIAbility(int32_t ability)
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
// std::string SDKWrapper::GetFaceLibsInfo()
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
// bool SDKWrapper::ClearFaceLibs()
// {
//     VSRPC(request, clearFaceLibs, intParams, response);
//
//     return VSRPC_CALL(request, response);
// }
//

// bool SDKWrapper::RegisterFaceIdWithPic(std::string path, std::string faceId)
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
// bool SDKWrapper::RegisterFaceIdFromCamera(std::string faceId)
// {
//     VSRPC(request, registerFaceIdFromCamera, strParams, response);
//     strcpy(VSRPC_PARAM(request).strParams, faceId.c_str());
//
//     return VSRPC_CALL(request, response);
// }

// bool SDKWrapper::VerifyFaceWithPic(std::string path, int32_t libId, std::string faceId)
// {
//     // send file & do verify
//     return messenger->SendFile(BlobParams_BlobType_FACEVERIFY, path, libId, faceId);
// }
//
// bool SDKWrapper::DeleteFaceId(std::string faceId)
// {
//     VSRPC(request, deleteFaceId, strParams, response);
//     strcpy(VSRPC_PARAM(request).strParams, faceId.c_str());
//
//     return VSRPC_CALL(request, response);
// }

void SDKWrapper::RegisterOnFaceResult(OnResult callback)
{
    messenger->RegisterOnFaceResult(callback);
}
