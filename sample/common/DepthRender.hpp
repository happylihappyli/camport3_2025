#ifndef PERCIPIO_SAMPLE_COMMON_DEPTH_RENDER_HPP_
#define PERCIPIO_SAMPLE_COMMON_DEPTH_RENDER_HPP_

#include "funny_Mat.hpp"
#include <map>
#include <vector>
#include <cassert>

// 定义ushort类型
typedef unsigned short ushort;


class DepthRender {
public:
    enum OutputColorType {
        COLORTYPE_RAINBOW = 0,
        COLORTYPE_BLUERED = 1,
        COLORTYPE_GRAY = 2
    };

    enum ColorRangeMode {
        COLOR_RANGE_ABS = 0,
        COLOR_RANGE_DYNAMIC = 1
    };

    DepthRender() : needResetColorTable(true)
                  , color_type(COLORTYPE_BLUERED)
                  , range_mode(COLOR_RANGE_DYNAMIC)
                  , min_distance(0)
                  , max_distance(0)
                  , invalid_label(0)
                  {}

    void SetColorType( OutputColorType ct = COLORTYPE_BLUERED ){
                if(ct != color_type){
                    needResetColorTable = true;
                    color_type = ct;
                }
            }

    void SetRangeMode( ColorRangeMode rm = COLOR_RANGE_DYNAMIC ){
                if(range_mode != rm){
                    needResetColorTable = true;
                    range_mode = rm;
                }
            }

    /// for abs mode
    void SetColorRange(int minDis, int maxDis){
                min_distance = minDis;
                max_distance = maxDis;
            }

    /// input 16UC1 output 8UC3
    void Compute(const funny_Mat &src, funny_Mat& dst ){
                dst = Compute(src);
            }
    funny_Mat Compute(const funny_Mat &src){
                funny_Mat src16U;
                // 转换为16位无符号类型
                if(src.type() != CV_16UC1){
                    // TODO: 实现类型转换功能
                    src16U.create(src.rows(), src.cols(), CV_16UC1);
                    // 简单复制数据
                    if (src.data()) {
                        // 注意：这里假设数据可以直接复制，实际使用时需要根据类型进行转换
                        // TODO: 实现完整的类型转换
                        memcpy(src16U.data(), src.data(), src.rows() * src.cols() * sizeof(uint16_t));
                    }
                } else {
                    // 复制数据
                    src16U.create(src.rows(), src.cols(), CV_16UC1);
                    if (src.data()) {
                        memcpy(src16U.data(), src.data(), src.rows() * src.cols() * sizeof(uint16_t));
                    }
                }

                if(needResetColorTable){
                    BuildColorTable();
                    needResetColorTable = false;
                }

                funny_Mat dst;
                // 创建掩码
                filtered_mask.create(src16U.rows(), src16U.cols(), CV_8UC1);
                // 创建显示图像
                clr_disp.create(src16U.rows(), src16U.cols(), CV_16UC1);
                
                // 复制数据并生成掩码
                if (src16U.data() && filtered_mask.data() && clr_disp.data()) {
                    uint16_t* src_data = reinterpret_cast<uint16_t*>(src16U.data());
                    uint16_t* clr_data = reinterpret_cast<uint16_t*>(clr_disp.data());
                    uint8_t* mask_data = reinterpret_cast<uint8_t*>(filtered_mask.data());
                    int total_pixels = src16U.rows() * src16U.cols();
                    
                    for (int i = 0; i < total_pixels; ++i) {
                        clr_data[i] = src_data[i];
                        mask_data[i] = (src_data[i] == invalid_label) ? 255 : 0;
                    }
                }

                // 创建8位灰度图像
                funny_Mat clr_8u;
                clr_8u.create(src16U.rows(), src16U.cols(), CV_8UC1);
                
                if(COLOR_RANGE_ABS == range_mode) {
                    // 截断值
                    TruncValue(clr_disp, filtered_mask, min_distance, max_distance);
                    
                    // 归一化到0-255
                    if (clr_disp.data() && clr_8u.data()) {
                        uint16_t* clr_data = reinterpret_cast<uint16_t*>(clr_disp.data());
                        uint8_t* clr_8u_data = reinterpret_cast<uint8_t*>(clr_8u.data());
                        int total_pixels = clr_disp.rows() * clr_disp.cols();
                        
                        for (int i = 0; i < total_pixels; ++i) {
                            // 减去最小值
                            if (clr_data[i] > min_distance) {
                                clr_data[i] -= min_distance;
                            } else {
                                clr_data[i] = 0;
                            }
                            
                            // 归一化
                            float norm_value = static_cast<float>(clr_data[i]) / (max_distance - min_distance);
                            clr_8u_data[i] = static_cast<uint8_t>(norm_value * 255);
                        }
                    }
                } else {
                    unsigned short vmax, vmin;
                    HistAdjustRange(clr_disp, invalid_label, min_distance, vmin, vmax);
                    
                    // 归一化到0-255
                    if (clr_disp.data() && clr_8u.data()) {
                        uint16_t* clr_data = reinterpret_cast<uint16_t*>(clr_disp.data());
                        uint8_t* clr_8u_data = reinterpret_cast<uint8_t*>(clr_8u.data());
                        int total_pixels = clr_disp.rows() * clr_disp.cols();
                        
                        for (int i = 0; i < total_pixels; ++i) {
                            // 减去最小值
                            if (clr_data[i] > vmin) {
                                clr_data[i] -= vmin;
                            } else {
                                clr_data[i] = 0;
                            }
                            
                            // 归一化
                            float norm_value = static_cast<float>(clr_data[i]) / (vmax - vmin);
                            clr_8u_data[i] = static_cast<uint8_t>(norm_value * 255);
                        }
                    }
                }

                // 创建输出图像
                dst.create(src16U.rows(), src16U.cols(), CV_8UC3);
                
                switch (color_type) {
                case COLORTYPE_GRAY:
                    // 灰度转BGR
                    if (clr_8u.data() && dst.data()) {
                        uint8_t* clr_8u_data = reinterpret_cast<uint8_t*>(clr_8u.data());
                        uint8_t* dst_data = reinterpret_cast<uint8_t*>(dst.data());
                        int total_pixels = clr_8u.rows() * clr_8u.cols();
                        
                        for (int i = 0; i < total_pixels; ++i) {
                            // 反转灰度值
                            uint8_t gray_val = 255 - clr_8u_data[i];
                            // 复制到BGR三个通道
                            dst_data[i * 3] = gray_val;
                            dst_data[i * 3 + 1] = gray_val;
                            dst_data[i * 3 + 2] = gray_val;
                        }
                    }
                    break;
                case COLORTYPE_BLUERED:
                    // 使用自定义颜色映射
                    CalcColorMap(clr_8u, dst);
                    break;
                case COLORTYPE_RAINBOW:
                    // 注意：由于移除了OpenCV依赖，彩虹色映射功能已简化
                    // 使用默认的灰度转彩色
                    if (clr_8u.data() && dst.data()) {
                        uint8_t* clr_8u_data = reinterpret_cast<uint8_t*>(clr_8u.data());
                        uint8_t* dst_data = reinterpret_cast<uint8_t*>(dst.data());
                        int total_pixels = clr_8u.rows() * clr_8u.cols();
                        
                        for (int i = 0; i < total_pixels; ++i) {
                            // 简化的彩虹色映射
                            uint8_t val = clr_8u_data[i];
                            dst_data[i * 3] = val < 128 ? 0 : (val - 128) * 2;
                            dst_data[i * 3 + 1] = val < 128 ? val * 2 : 255 - (val - 128) * 2;
                            dst_data[i * 3 + 2] = val < 128 ? 255 - val * 2 : 0;
                        }
                    }
                    break;
                }
                
                // 清除无效区域
                ClearInvalidArea(dst, filtered_mask);

                return dst;
            }

private:
    void CalcColorMap(const funny_Mat &src, funny_Mat &dst){
                std::vector<funny_Scalar> &table = _color_lookup_table;
                assert(table.size() == 256);
                assert(src.data() && dst.data());
                assert(src.type() == CV_8UC1);
                assert(dst.type() == CV_8UC3);
                
                const unsigned char* sptr = reinterpret_cast<const unsigned char*>(src.data());
                unsigned char* dptr = reinterpret_cast<unsigned char*>(dst.data());
                int total_pixels = src.rows() * src.cols();
                
                for (int i = 0; i < total_pixels; ++i) {
                    funny_Scalar &v = table[*sptr];
                    dptr[i * 3] = static_cast<unsigned char>(v.val[0]);
                    dptr[i * 3 + 1] = static_cast<unsigned char>(v.val[1]);
                    dptr[i * 3 + 2] = static_cast<unsigned char>(v.val[2]);
                    sptr += 1;
                }
            }
    void BuildColorTable(){
                _color_lookup_table.resize(256);
                funny_Scalar from(50, 0, 0xff), to(50, 200, 255);
                for (int i = 0; i < 128; i++) {
                    float a = static_cast<float>(i) / 128;
                    funny_Scalar &v = _color_lookup_table[i];
                    for (int j = 0; j < 3; j++) {
                        v.val[j] = from.val[j] * (1 - a) + to.val[j] * a;
                    }
                }
                from = to;
                to = funny_Scalar(255, 104, 0);
                for (int i = 128; i < 256; i++) {
                    float a = static_cast<float>(i - 128) / 128;
                    funny_Scalar &v = _color_lookup_table[i];
                    for (int j = 0; j < 3; j++) {
                        v.val[j] = from.val[j] * (1 - a) + to.val[j] * a;
                    }
                }
            }
    //keep value in range
    void TruncValue(funny_Mat &img, funny_Mat &mask, short min_val, short max_val){
                assert(max_val >= min_val);
                assert(img.data() && mask.data());
                assert(img.type() == CV_16UC1);
                assert(mask.type() == CV_8UC1);
                
                uint16_t* ptr = reinterpret_cast<uint16_t*>(img.data());
                uint8_t* mask_ptr = reinterpret_cast<uint8_t*>(mask.data());
                int total_pixels = img.rows() * img.cols();
                
                for (int i = 0; i < total_pixels; ++i) {
                  if (*reinterpret_cast<short*>(ptr) > max_val) {
                    *ptr = static_cast<uint16_t>(max_val);
                    *mask_ptr = 0xff;
                  } else if (*reinterpret_cast<short*>(ptr) < min_val) {
                    *ptr = static_cast<uint16_t>(min_val);
                    *mask_ptr = 0xff;
                  }
                  ptr++;
                  mask_ptr++;
                }
            }
    void ClearInvalidArea(funny_Mat &clr_disp, funny_Mat &filtered_mask){
                assert(clr_disp.data() && filtered_mask.data());
                assert(clr_disp.type() == CV_8UC3);
                assert(filtered_mask.type() == CV_8UC1);
                assert(clr_disp.rows() == filtered_mask.rows() && clr_disp.cols() == filtered_mask.cols());
                
                unsigned char* filter_ptr = reinterpret_cast<unsigned char*>(filtered_mask.data());
                unsigned char* ptr = reinterpret_cast<unsigned char*>(clr_disp.data());
                int total_pixels = clr_disp.rows() * clr_disp.cols();
                
                for (int i = 0; i < total_pixels; ++i) {
                    if (*filter_ptr != 0) {
                      ptr[i * 3] = 0;
                      ptr[i * 3 + 1] = 0;
                      ptr[i * 3 + 2] = 0;
                    }
                    filter_ptr++;
                }
            }
    void HistAdjustRange(const funny_Mat &dist, ushort invalid, int min_display_distance_range
            , ushort &min_val, ushort &max_val) {
                std::map<ushort, int> hist;
                int total_pixels = dist.rows() * dist.cols();
                const ushort* ptr = reinterpret_cast<const ushort*>(dist.data());
                int count = 0;
                
                if (ptr) {
                    for (int i = 0; i < total_pixels; ++i) {
                        if (invalid == *ptr) {
                            ptr++;
                            continue;
                        }
                        count++;
                        if (hist.find(*ptr) != hist.end()) {
                            hist[*ptr]++;
                        } else {
                            hist.insert(std::make_pair(*ptr, 1));
                        }
                        ptr++;
                    }
                }
                
                if (hist.empty()) {
                    min_val = 0;
                    max_val = 2000;
                    return;
                }
                
                const int delta = count * 0.01;
                int sum = 0;
                min_val = hist.begin()->first;
                
                for (std::map<ushort, int>::iterator it = hist.begin(); it != hist.end(); it++){
                    sum += it->second;
                    if (sum > delta) {
                        min_val = it->first;
                        break;
                    }
                }

                sum = 0;
                max_val = hist.rbegin()->first;
                
                for (std::map<ushort, int>::reverse_iterator s = hist.rbegin(); s != hist.rend(); s++) {
                    sum += s->second;
                    if (sum > delta) {
                        max_val = s->first;
                        break;
                    }
                }

                const int min_display_dist = min_display_distance_range;
                if (max_val - min_val < min_display_dist) {
                    int m = (max_val + min_val) / 2;
                    max_val = static_cast<ushort>(m + min_display_dist / 2);
                    min_val = static_cast<ushort>(m - min_display_dist / 2);
                    if (min_val < 0) {
                        min_val = 0;
                    }
                }
            }

    bool            needResetColorTable;
    OutputColorType color_type;
    ColorRangeMode  range_mode;
    int             min_distance;
    int             max_distance;
    uint16_t        invalid_label;
    funny_Mat       clr_disp;
    funny_Mat       filtered_mask;
    std::vector<funny_Scalar> _color_lookup_table;
};

#endif