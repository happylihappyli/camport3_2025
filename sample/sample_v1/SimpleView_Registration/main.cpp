#include "TYImageProc.h"
#include "common.hpp"
#include <thread>
#include <chrono>

#define MAP_DEPTH_TO_COLOR  0

struct CallbackData {
  int             index;
  TY_ISP_HANDLE   IspHandle;
  TY_DEV_HANDLE   hDevice;
  DepthRender*    render;
  DepthViewer*    depthViewer;
  bool            needUndistort;

  float           scale_unit;

  bool            isTof;
  TY_CAMERA_CALIB_INFO depth_calib;
  TY_CAMERA_CALIB_INFO color_calib;
};

funny_Mat tofundis_mapx, tofundis_mapy;
static void doRegister(const TY_CAMERA_CALIB_INFO& depth_calib
                      , const TY_CAMERA_CALIB_INFO& color_calib
                      , const funny_Mat& depth
                      , const float f_scale_unit
                      , const funny_Mat& color
                      , bool needUndistort
                      , funny_Mat& undistort_color
                      , funny_Mat& out
                      , bool map_depth_to_color
                      )
{
  int32_t         image_size;   
  TY_PIXEL_FORMAT color_fmt;
  if(color.type() == CV_16U) {
    image_size = color.cols() * color.rows() * 2;
    color_fmt = TY_PIXEL_FORMAT_MONO16;
  }
  else if(color.type() == CV_16UC3)
  {
    image_size = color.cols() * color.rows() * 6;
    color_fmt = TY_PIXEL_FORMAT_RGB48;
  }
  else {
    image_size = color.cols() * color.rows() * 3;
    color_fmt = TY_PIXEL_FORMAT_RGB;
  }
  // do undistortion
  if (needUndistort) {
    if(color_fmt == TY_PIXEL_FORMAT_MONO16)
      undistort_color = funny_Mat(color.cols(), color.rows(), CV_16U);
    else if(color_fmt == TY_PIXEL_FORMAT_RGB48)
      undistort_color = funny_Mat(color.cols(), color.rows(), CV_16UC3);
    else
      undistort_color = funny_Mat(color.cols(), color.rows(), CV_8UC3);
    
    TY_IMAGE_DATA src;
    src.width = color.cols();
    src.height = color.rows();
    src.size = image_size;
    src.pixelFormat = color_fmt;
    src.buffer = color.data();

    TY_IMAGE_DATA dst;
    dst.width = color.cols();
    dst.height = color.rows();
    dst.size = image_size;
    dst.pixelFormat = color_fmt;
    dst.buffer = undistort_color.data();
    ASSERT_OK(TYUndistortImage(&color_calib, &src, NULL, &dst));
  }
  else {
    undistort_color = color;
  }

  // do register
  if (map_depth_to_color) {
    int outW = depth.cols();
    int outH = depth.cols() * undistort_color.rows() / undistort_color.cols();
    out = funny_Mat::zeros(outW, outH, CV_16U);
    ASSERT_OK(
      TYMapDepthImageToColorCoordinate(
        &depth_calib,
        depth.cols(), depth.rows(), reinterpret_cast<uint16_t*>(depth.data()),
        &color_calib,
        out.cols(), out.rows(), reinterpret_cast<uint16_t*>(out.data()), f_scale_unit
      )
    );
    // TODO: 实现funny_Mat的resize函数
    // 暂时跳过resize，直接使用原始尺寸
  }
  else {
    if(color_fmt == TY_PIXEL_FORMAT_MONO16)
    {
      out = funny_Mat::zeros(depth.cols(), depth.rows(), CV_16U);
      ASSERT_OK(
        TYMapMono16ImageToDepthCoordinate(
          &depth_calib,
          depth.cols(), depth.rows(), reinterpret_cast<uint16_t*>(depth.data()),
          &color_calib,
          undistort_color.cols(), undistort_color.rows(), reinterpret_cast<uint16_t*>(undistort_color.data()),
          reinterpret_cast<uint16_t*>(out.data()), f_scale_unit
        )
      );
    }
    else if(color_fmt == TY_PIXEL_FORMAT_RGB48)
    {
      out = funny_Mat::zeros(depth.cols(), depth.rows(), CV_16UC3);
      ASSERT_OK(
        TYMapRGB48ImageToDepthCoordinate(
          &depth_calib,
          depth.cols(), depth.rows(), reinterpret_cast<uint16_t*>(depth.data()),
          &color_calib,
          undistort_color.cols(), undistort_color.rows(), reinterpret_cast<uint16_t*>(undistort_color.data()),
          reinterpret_cast<uint16_t*>(out.data()), f_scale_unit
        )
      );
    }
    else{
      out = funny_Mat::zeros(depth.cols(), depth.rows(), CV_8UC3);
      ASSERT_OK(
        TYMapRGBImageToDepthCoordinate(
          &depth_calib,
          depth.cols(), depth.rows(), reinterpret_cast<uint16_t*>(depth.data()),
          &color_calib,
          undistort_color.cols(), undistort_color.rows(), reinterpret_cast<uint8_t*>(undistort_color.data()),
          reinterpret_cast<uint8_t*>(out.data()), f_scale_unit
        )
      );
    }
  }
}


void handleFrame(TY_FRAME_DATA* frame, void* userdata)
{
  CallbackData* pData = (CallbackData*)userdata;
  LOGD("=== Get frame %d", ++pData->index);

  funny_Mat depth, color;
  parseFrame(*frame, &depth, nullptr, nullptr, &color, pData->IspHandle);
  if (!depth.empty()) {
    if (pData->isTof)
    {
        TY_IMAGE_DATA src;
        src.width = depth.cols();
        src.height = depth.rows();
        src.size = depth.cols() * depth.rows() * 2;
        src.pixelFormat = TY_PIXEL_FORMAT_DEPTH16;
        src.buffer = depth.data();

        funny_Mat undistort_depth = funny_Mat(depth.cols(), depth.rows(), CV_16U);
        TY_IMAGE_DATA dst;
        dst.width = depth.cols();
        dst.height = depth.rows();
        dst.size = undistort_depth.cols() * undistort_depth.rows() * 2;
        dst.buffer = undistort_depth.data();
        dst.pixelFormat = TY_PIXEL_FORMAT_DEPTH16;
        ASSERT_OK(TYUndistortImage(&pData->depth_calib, &src, NULL, &dst));

        depth = undistort_depth.clone();
    }
    pData->depthViewer->show(depth);
  }
  if (!color.empty()) {
    // TODO: 实现funny_Mat的显示函数
    std::cout << "显示color图像" << std::endl;
  }

  if (!depth.empty() && !color.empty()) {
    funny_Mat undistort_color, out;
    if (pData->needUndistort || MAP_DEPTH_TO_COLOR) {
      doRegister(pData->depth_calib, pData->color_calib, depth, pData->scale_unit, color, pData->needUndistort, undistort_color, out, MAP_DEPTH_TO_COLOR);
    }
    else {
      undistort_color = color;
      out = color;
    }
    // TODO: 实现funny_Mat的显示函数
    std::cout << "显示undistort color图像" << std::endl;

    funny_Mat tmp, gray8, bgr;
    if (MAP_DEPTH_TO_COLOR) {
      funny_Mat depthDisplay = pData->render->Compute(out);
      if(undistort_color.type() == CV_16U) {
        gray8 = funny_Mat(undistort_color.cols(), undistort_color.rows(), CV_8U);
        // TODO: 实现funny_Mat的normalize和convertScaleAbs函数
        // 简单实现：将16位数据映射到8位
        for(int i = 0; i < undistort_color.rows(); i++) {
          for(int j = 0; j < undistort_color.cols(); j++) {
            uint16_t val = undistort_color.at<uint16_t>(i, j);
            gray8.at<uint8_t>(i, j) = static_cast<uint8_t>(val / 256);
          }
        }
        // TODO: 实现funny_Mat的cvtColor函数
      } else if(undistort_color.type() == CV_16UC3) {
        bgr = funny_Mat(undistort_color.cols(), undistort_color.rows(), CV_8UC3);
        // TODO: 实现funny_Mat的normalize和convertScaleAbs函数
        // 简单实现：将16位数据映射到8位
        for(int i = 0; i < undistort_color.rows(); i++) {
          for(int j = 0; j < undistort_color.cols(); j++) {
            for(int c = 0; c < 3; c++) {
              uint16_t val = undistort_color.at<uint16_t>(i, j, c);
              bgr.at<uint8_t>(i, j, c) = static_cast<uint8_t>(val / 256);
            }
          }
        }
        undistort_color = bgr.clone();
      }
      // TODO: 实现funny_Mat的加法和除法操作
      // 简单实现：像素级混合
      for(int i = 0; i < depthDisplay.rows(); i++) {
        for(int j = 0; j < depthDisplay.cols(); j++) {
          for(int c = 0; c < 3; c++) {
            uint8_t val1 = depthDisplay.at<uint8_t>(i, j, c);
            uint8_t val2 = undistort_color.at<uint8_t>(i, j, c);
            depthDisplay.at<uint8_t>(i, j, c) = (val1 + val2) / 2;
          }
        }
      }
      // TODO: 实现funny_Mat的显示函数
      std::cout << "显示depth2color RGBD图像" << std::endl;
    }
    else {
      // TODO: 实现funny_Mat的显示函数
      std::cout << "显示mapped RGB图像" << std::endl;
      if(out.type() == CV_16U) {
        gray8 = funny_Mat(out.cols(), out.rows(), CV_8U);
        // TODO: 实现funny_Mat的normalize和convertScaleAbs函数
        // 简单实现：将16位数据映射到8位
        for(int i = 0; i < out.rows(); i++) {
          for(int j = 0; j < out.cols(); j++) {
            uint16_t val = out.at<uint16_t>(i, j);
            gray8.at<uint8_t>(i, j) = static_cast<uint8_t>(val / 256);
          }
        }
        // TODO: 实现funny_Mat的cvtColor函数
      } else if(undistort_color.type() == CV_16UC3) {
        bgr = funny_Mat(out.cols(), out.rows(), CV_8UC3);
        // TODO: 实现funny_Mat的normalize和convertScaleAbs函数
        // 简单实现：将16位数据映射到8位
        for(int i = 0; i < out.rows(); i++) {
          for(int j = 0; j < out.cols(); j++) {
            for(int c = 0; c < 3; c++) {
              uint16_t val = out.at<uint16_t>(i, j, c);
              bgr.at<uint8_t>(i, j, c) = static_cast<uint8_t>(val / 256);
            }
          }
        }
        out = bgr.clone();
      }
      funny_Mat depthDisplay = pData->render->Compute(depth);
      // TODO: 实现funny_Mat的加法和除法操作
      // 简单实现：像素级混合
      for(int i = 0; i < depthDisplay.rows(); i++) {
        for(int j = 0; j < depthDisplay.cols(); j++) {
          for(int c = 0; c < 3; c++) {
            uint8_t val1 = depthDisplay.at<uint8_t>(i, j, c);
            uint8_t val2 = out.at<uint8_t>(i, j, c);
            depthDisplay.at<uint8_t>(i, j, c) = (val1 + val2) / 2;
          }
        }
      }
      // TODO: 实现funny_Mat的显示函数
      std::cout << "显示color2depth RGBD图像" << std::endl;
    }
  }

  LOGD("=== Re-enqueue buffer(%p, %d)", frame->userBuffer, frame->bufferSize);
  ASSERT_OK(TYEnqueueBuffer(pData->hDevice, frame->userBuffer, frame->bufferSize));
}

void eventCallback(TY_EVENT_INFO *event_info, void *userdata)
{
    if (event_info->eventId == TY_EVENT_DEVICE_OFFLINE) {
        LOGD("=== Event Callback: Device Offline!");
        // Note: 
        //     Please set TY_BOOL_KEEP_ALIVE_ONOFF feature to false if you need to debug with breakpoint!
    }
    else if (event_info->eventId == TY_EVENT_LICENSE_ERROR) {
        LOGD("=== Event Callback: License Error!");
    }
}

int main(int argc, char* argv[])
{
    std::string ID, IP;
    TY_INTERFACE_HANDLE hIface = NULL;
    TY_DEV_HANDLE hDevice = NULL;

    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "-id") == 0){
            ID = argv[++i];
        } else if(strcmp(argv[i], "-ip") == 0) {
            IP = argv[++i];
        }else if(strcmp(argv[i], "-h") == 0){
            LOGI("Usage: SimpleView_Registration [-h] [-id <ID>]");
            return 0;
        }
    }
    
    LOGD("=== Init lib");
    ASSERT_OK( TYInitLib() );
    TY_VERSION_INFO ver;
    ASSERT_OK( TYLibVersion(&ver) );
    LOGD("     - lib version: %d.%d.%d", ver.major, ver.minor, ver.patch);

    std::vector<TY_DEVICE_BASE_INFO> selected;
    ASSERT_OK( selectDevice(TY_INTERFACE_ALL, ID, IP, 1, selected) );
    ASSERT(selected.size() > 0);
    TY_DEVICE_BASE_INFO& selectedDev = selected[0];

    ASSERT_OK( TYOpenInterface(selectedDev.iface.id, &hIface) );
    ASSERT_OK( TYOpenDevice(hIface, selectedDev.id, &hDevice) );

    TY_COMPONENT_ID allComps;
    ASSERT_OK( TYGetComponentIDs(hDevice, &allComps) );
    if(!(allComps & TY_COMPONENT_RGB_CAM)){
        LOGE("=== Has no RGB camera, cant do registration");
        return -1;
    }
    TY_ISP_HANDLE isp_handle;
    ASSERT_OK(TYISPCreate(&isp_handle));
    ASSERT_OK(ColorIspInitSetting(isp_handle, hDevice));
    //You can turn on auto exposure function as follow ,but frame rate may reduce .
    //Device also may be casually stucked  1~2 seconds when software trying to adjust device exposure time value
#if 0
    ASSERT_OK(ColorIspInitAutoExposure(isp_handle, hDevice));
#endif


    LOGD("=== Configure components");
    TY_COMPONENT_ID componentIDs = TY_COMPONENT_DEPTH_CAM | TY_COMPONENT_RGB_CAM;
    ASSERT_OK( TYEnableComponents(hDevice, componentIDs) );

    // ASSERT_OK( TYSetEnum(hDevice, TY_COMPONENT_RGB_CAM, TY_ENUM_IMAGE_MODE, TY_IMAGE_MODE_YUYV_640x480) );
    bool hasUndistortSwitch, hasDistortionCoef;
    ASSERT_OK( TYHasFeature(hDevice, TY_COMPONENT_RGB_CAM, TY_BOOL_UNDISTORTION, &hasUndistortSwitch) );
    ASSERT_OK( TYHasFeature(hDevice, TY_COMPONENT_RGB_CAM, TY_STRUCT_CAM_DISTORTION, &hasDistortionCoef) );
    if (hasUndistortSwitch) {
      ASSERT_OK( TYSetBool(hDevice, TY_COMPONENT_RGB_CAM, TY_BOOL_UNDISTORTION, true) );
    }

    LOGD("=== Prepare image buffer");
    uint32_t frameSize;
    ASSERT_OK( TYGetFrameBufferSize(hDevice, &frameSize) );
    LOGD("     - Get size of framebuffer, %d", frameSize);
    LOGD("     - Allocate & enqueue buffers");
    char* frameBuffer[2];
    frameBuffer[0] = new char[frameSize];
    frameBuffer[1] = new char[frameSize];
    LOGD("     - Enqueue buffer (%p, %d)", frameBuffer[0], frameSize);
    ASSERT_OK( TYEnqueueBuffer(hDevice, frameBuffer[0], frameSize) );
    LOGD("     - Enqueue buffer (%p, %d)", frameBuffer[1], frameSize);
    ASSERT_OK( TYEnqueueBuffer(hDevice, frameBuffer[1], frameSize) );

    LOGD("=== Register event callback");
    ASSERT_OK(TYRegisterEventCallback(hDevice, eventCallback, NULL));

    bool hasTriggerParam = false;
    ASSERT_OK( TYHasFeature(hDevice, TY_COMPONENT_DEVICE, TY_STRUCT_TRIGGER_PARAM_EX, &hasTriggerParam) );
    if (hasTriggerParam) {
        LOGD("=== Disable trigger mode");
        TY_TRIGGER_PARAM_EX trigger;
        trigger.mode = TY_TRIGGER_MODE_OFF;
        ASSERT_OK(TYSetStruct(hDevice, TY_COMPONENT_DEVICE, TY_STRUCT_TRIGGER_PARAM_EX, &trigger, sizeof(trigger)));
    }

    DepthViewer depthViewer("Depth");
    DepthRender render;
    CallbackData cb_data;
    cb_data.index = 0;
    cb_data.hDevice = hDevice;
    cb_data.depthViewer = &depthViewer;
    cb_data.render = &render;
    cb_data.needUndistort = !hasUndistortSwitch && hasDistortionCoef;
    cb_data.IspHandle = isp_handle;

    float scale_unit = 1.;
    TYGetFloat(hDevice, TY_COMPONENT_DEPTH_CAM, TY_FLOAT_SCALE_UNIT, &scale_unit);
    cb_data.scale_unit = scale_unit;
    depthViewer.depth_scale_unit = scale_unit;


    LOGD("=== Read depth calib info");
    ASSERT_OK( TYGetStruct(hDevice, TY_COMPONENT_DEPTH_CAM, TY_STRUCT_CAM_CALIB_DATA
          , &cb_data.depth_calib, sizeof(cb_data.depth_calib)) );

    LOGD("=== Read color calib info");
    ASSERT_OK( TYGetStruct(hDevice, TY_COMPONENT_RGB_CAM, TY_STRUCT_CAM_CALIB_DATA
          , &cb_data.color_calib, sizeof(cb_data.color_calib)) );

    ASSERT_OK(TYHasFeature(hDevice, TY_COMPONENT_DEPTH_CAM, TY_STRUCT_CAM_DISTORTION, &cb_data.isTof));

    LOGD("=== Start capture");
    ASSERT_OK( TYStartCapture(hDevice) );

    LOGD("=== Wait for callback");
    bool exit_main = false;
    while(!exit_main){
        TY_FRAME_DATA frame;
        int err = TYFetchFrame(hDevice, &frame, -1);
        if( err != TY_STATUS_OK ) {
            LOGE("Fetch frame error %d: %s", err, TYErrorString(err));
            break;
        }

        handleFrame(&frame, &cb_data);
        TYISPUpdateDevice(cb_data.IspHandle);
        
        // 简化的退出逻辑，每100帧检查一次是否达到退出条件
        if (cb_data.index % 100 == 0) {
            // 注意：此处可以根据实际需求添加适当的退出条件检查
            // 例如读取键盘输入或检查外部标志位
        }
        
        // 添加简单的延时，避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_OK( TYStopCapture(hDevice) );
    ASSERT_OK( TYCloseDevice(hDevice) );
    ASSERT_OK( TYCloseInterface(hIface) );
    ASSERT_OK( TYDeinitLib() );
    delete frameBuffer[0];
    delete frameBuffer[1];

    LOGD("=== Main done!");
    return 0;
}
