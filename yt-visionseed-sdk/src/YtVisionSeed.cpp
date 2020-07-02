#include <stdio.h>

#include "YtVisionSeed.h"

YtVisionSeed::YtVisionSeed(const char* dev)
{
    messenger = new YtMessenger(dev);
    // REGISTER_CALLBACK(sendBlob);
    messenger->startLoop();

    //等待读取清空VS上的发送队列，避免后续RPC请求回复失败
    usleep(50000);

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

YtRpcResponse_ReturnCode YtVisionSeed::getErrorCode(shared_ptr<YtMsg> response)
{
    return response ? response->values.response.code : YtRpcResponse_ReturnCode_ERROR_RPC_TIMEOUT;
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
void YtVisionSeed::SetFlasherAsync(int32_t flasherIR, int32_t flasherWhite)
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

    VSRPC_CALL_ASYNC(request);
}

bool YtVisionSeed::SetDebugDrawing(int32_t drawing)
{
    VSRPC(request, setDebugDrawing, intParams, response);
    VSRPC_PARAM(request).intParams = drawing;
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

bool YtVisionSeed::ClearFaceLib()
{
    VSRPC(request, clearFaceLib, intParams, response);

    return VSRPC_CALL(request, response);
}

YtRpcResponse_ReturnCode YtVisionSeed::RegisterFaceIdWithPic(std::string path, std::string faceName, int32_t *faceIdOut)
{
    // send file & do register
    std::size_t extPos = path.rfind(".");
    if (extPos == std::string::npos)
    {
        return YtRpcResponse_ReturnCode_ERROR_INVALID_PATH;
    }
    std::string ext = path.substr(extPos);
    std::string remotePath = std::string("/tmp/reg") + ext;
    LOG_D("remotePath = %s\n", remotePath.c_str());
    if (!messenger->SendFile(path, remotePath.c_str()))
    {
        return YtRpcResponse_ReturnCode_ERROR_INVALID_PATH;
    }

    VSRPC(request, registerFaceIdWithPic, registerFaceIdWithPicParams, response);
    strcpy(VSRPC_PARAM(request).registerFaceIdWithPicParams.filePath, remotePath.c_str());
    strcpy(VSRPC_PARAM(request).registerFaceIdWithPicParams.faceName, faceName.c_str());

    if (VSRPC_CALL(request, response))
    {
        if (faceIdOut != NULL)
        {
            *faceIdOut = response->values.response.data.intData;
        }
    }
    return getErrorCode(response);
}

YtRpcResponse_ReturnCode YtVisionSeed::RegisterFaceIdFromCamera(std::string faceName, int32_t timeoutMs, int32_t *faceIdOut)
{
    VSRPC(request, registerFaceIdFromCamera, registerFaceIdFromCameraParams, response);
    VSRPC_PARAM(request).registerFaceIdFromCameraParams.timeoutMs = timeoutMs;
    strcpy(VSRPC_PARAM(request).registerFaceIdFromCameraParams.faceName, faceName.c_str());

    if (VSRPC_CALL(request, response))
    {
        if (faceIdOut != NULL)
        {
            *faceIdOut = response->values.response.data.intData;
        }
    }
    return getErrorCode(response);
}

YtRpcResponse_ReturnCode YtVisionSeed::RegisterFaceIdWithTraceId(std::string faceName, uint32_t traceId, int32_t *faceIdOut)
{
    VSRPC(request, registerFaceIdWithTraceId, registerFaceIdWithTraceIdParams, response);
    VSRPC_PARAM(request).registerFaceIdWithTraceIdParams.traceId = traceId;
    strcpy(VSRPC_PARAM(request).registerFaceIdWithTraceIdParams.faceName, faceName.c_str());

    if (VSRPC_CALL(request, response))
    {
        if (faceIdOut != NULL)
        {
            *faceIdOut = response->values.response.data.intData;
        }
    }
    return getErrorCode(response);
}

bool YtVisionSeed::DeleteFaceId(int32_t faceId)
{
    VSRPC(request, deleteFaceId, intParams, response);
    VSRPC_PARAM(request).intParams = faceId;

    return VSRPC_CALL(request, response);
}

int32_t YtVisionSeed::DeleteFaceName(std::string faceName)
{
    VSRPC(request, deleteFaceName, strParams, response);
    strcpy(VSRPC_PARAM(request).strParams, faceName.c_str());

    if (!VSRPC_CALL(request, response))
    {
        return -1;
    }
    return response->values.response.data.intData;
}

std::vector<FaceIdInfo> YtVisionSeed::ListFaceId()
{
    int faceListBegin = 0;
    std::vector<FaceIdInfo> faces;
#ifdef YtResult_size
    while (true)
    {
        VSRPC(request, listFaceId, listFaceIdParams, response);
        VSRPC_PARAM(request).listFaceIdParams.start = faceListBegin;
        VSRPC_PARAM(request).listFaceIdParams.length = 100;

        if (!VSRPC_CALL(request, response))
        {
            break;
        }
        if (response->values.response.data.faceIdListData.faces_count == 0)
        {
            break;
        }
        for (size_t i = 0; i < response->values.response.data.faceIdListData.faces_count; i++)
        {
            FaceIdInfo &info = response->values.response.data.faceIdListData.faces[i];
            faces.push_back( info );
        }

        faceListBegin = faces[faces.size() - 1].faceId + 1;
    }
#else
    // TODO: for dynamic (nano) version of proto, it requires pb_callback_t interface
    LOG_E("[ListFaceId] Not implemented!\n");
#endif
    return faces;
}

bool YtVisionSeed::SetFaceName(int32_t faceId, std::string faceName)
{
    VSRPC(request, setFaceId, setFaceIdParams, response);
    VSRPC_PARAM(request).setFaceIdParams.faceId = faceId;
    strcpy(VSRPC_PARAM(request).setFaceIdParams.faceName, faceName.c_str());
    return VSRPC_CALL(request, response);
}

YtRpcResponse_ReturnCode YtVisionSeed::GetFacePic(int32_t faceId, std::string path)
{
    VSRPC(request, getFacePic, intParams, response);
    VSRPC_PARAM(request).intParams = faceId;
    if (VSRPC_CALL(request, response))
    {
        size_t len = response->values.response.data.filePart.totalLength;
        if (len != response->values.response.data.filePart.data->size)
        {
            LOG_E("[GetFacePic] multiple package not support yet!\n");
            return YtRpcResponse_ReturnCode_ERROR_OTHER;
        }

        FILE *outfile = fopen(path.c_str(), "wb");
        if (outfile)
        {
            fwrite(response->values.response.data.filePart.data->bytes, len, 1, outfile);
            fclose(outfile);
        }
        else
        {
            return YtRpcResponse_ReturnCode_ERROR_INVALID_PATH;
        }
    }
    return getErrorCode(response);
}

YtRpcResponse_ReturnCode YtVisionSeed::GetTracePic(int32_t traceId, std::string path)
{
    VSRPC(request, getTracePic, intParams, response);
    VSRPC_PARAM(request).intParams = traceId;
    if (VSRPC_CALL(request, response))
    {
        size_t len = response->values.response.data.filePart.totalLength;
        if (len != response->values.response.data.filePart.data->size)
        {
            LOG_E("[GetTracePic] multiple package not support yet!\n");
            return YtRpcResponse_ReturnCode_ERROR_OTHER;
        }

        FILE *outfile = fopen(path.c_str(), "wb");
        if (outfile)
        {
            fwrite(response->values.response.data.filePart.data->bytes, len, 1, outfile);
            fclose(outfile);
        }
        else
        {
            return YtRpcResponse_ReturnCode_ERROR_INVALID_PATH;
        }
    }
    return getErrorCode(response);
}

void YtVisionSeed::RegisterOnStatus(OnResultCallback callback)
{
    messenger->RegisterOnStatus(callback);
}
void YtVisionSeed::RegisterOnResult(OnResultCallback callback)
{
    messenger->RegisterOnResult(callback);
}
