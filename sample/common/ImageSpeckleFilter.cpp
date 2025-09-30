
#include "ImageSpeckleFilter.hpp"
#include <stdio.h>
#include <stdexcept>

#ifdef WIN32
#include <stdint.h>
#endif
struct Point2s {
  Point2s(short _x, short _y) {
    x = _x;
    y = _y;
  }
  short x, y;
};

template <typename T>
void filterSpecklesImpl(funny_Mat& img, int newVal, int maxSpeckleSize, int maxDiff, std::vector<char> &_buf) {
  int width = img.cols();
  int height = img.rows();
  int npixels = width * height;//number of pixels
  size_t bufSize = npixels * (int)(sizeof(Point2s) + sizeof(int) + sizeof(uint8_t));//all pixel buffer
  if (_buf.size() < bufSize) {
    _buf.resize((int)bufSize);
  }

  uint8_t* buf = (uint8_t*)(&_buf[0]);
  int i, j, dstep = img.cols();//(int)(img.step / sizeof(T));
  int* labels = (int*)buf;
  buf += npixels * sizeof(labels[0]);
  Point2s* wbuf = (Point2s*)buf;
  buf += npixels * sizeof(wbuf[0]);
  uint8_t* rtype = (uint8_t*)buf;
  int curlabel = 0;

  // clear out label assignments
  memset(labels, 0, npixels * sizeof(labels[0]));

  for (i = 0; i < height; i++) {
    // 注意：这里使用data()方法获取数据指针，并计算行偏移
    T* ds = (T*)(img.data() + i * img.cols() * sizeof(T));
    int* ls = labels + width * i;//label ptr for a row

    for (j = 0; j < width; j++) {
      if (ds[j] != newVal) { // not a bad disparity
        if (ls[j]) {   // has a label, check for bad label
          if (rtype[ls[j]]) // small region, zero out disparity
            ds[j] = (T)newVal;
        }
        // no label, assign and propagate
        else {
          Point2s* ws = wbuf; // initialize wavefront
          Point2s p((short)j, (short)i);  // current pixel
          curlabel++; // next label
          int count = 0;  // current region size
          ls[j] = curlabel;

          // wavefront propagation
          while (ws >= wbuf) { // wavefront not empty
            count++;
            // put neighbors onto wavefront
            // 注意：这里使用data()方法获取数据指针，并计算位置偏移
            T* dpp = (T*)(img.data() + p.y * img.cols() * sizeof(T) + p.x * sizeof(T));
            T dp = *dpp;
            int* lpp = labels + width * p.y + p.x;

            if (p.x < width - 1 && !lpp[+1] && dpp[+1] != newVal && std::abs(dp - dpp[+1]) <= maxDiff) {
              lpp[+1] = curlabel;
              *ws++ = Point2s(p.x + 1, p.y);
            }

            if (p.x > 0 && !lpp[-1] && dpp[-1] != newVal && std::abs(dp - dpp[-1]) <= maxDiff) {
              lpp[-1] = curlabel;
              *ws++ = Point2s(p.x - 1, p.y);
            }

            if (p.y < height - 1 && !lpp[+width] && ((T*)(img.data() + (p.y + 1) * img.cols() * sizeof(T)))[p.x] != newVal && std::abs((int)(dp - ((T*)(img.data() + (p.y + 1) * img.cols() * sizeof(T)))[p.x])) <= maxDiff) {
              lpp[+width] = curlabel;
              *ws++ = Point2s(p.x, p.y + 1);
            }

            if (p.y > 0 && !lpp[-width] && ((T*)(img.data() + (p.y - 1) * img.cols() * sizeof(T)))[p.x] != newVal && std::abs((int)(dp - ((T*)(img.data() + (p.y - 1) * img.cols() * sizeof(T)))[p.x])) <= maxDiff) {
              lpp[-width] = curlabel;
              *ws++ = Point2s(p.x, p.y - 1);
            }

            // pop most recent and propagate
            // NB: could try least recent, maybe better convergence
            p = *--ws;
          }

          // assign label type
          if (count <= maxSpeckleSize) { // speckle region
            rtype[ls[j]] = 1;   // small region label
            ds[j] = (T)newVal;
          } else
            rtype[ls[j]] = 0;   // large region label
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////

ImageSpeckleFilter gSpeckleFilter;

void ImageSpeckleFilter::Compute(funny_Mat &image, int newVal, int maxSpeckleSize, int maxDiff)
{
    if(image.type() == CV_8UC1){
        filterSpecklesImpl<uint8_t>(image, newVal, maxSpeckleSize, maxDiff, _labelBuf);
    } else if(image.type() == CV_16U || image.type() == CV_16UC1){
        filterSpecklesImpl<uint16_t>(image, newVal, maxSpeckleSize, maxDiff, _labelBuf);
    } else {
        char sz[10];
        sprintf(sz, "%d", image.type());
        throw std::runtime_error(std::string("ImageSpeckleFilter only support 8u and 16u, not ") + sz);
    }
}