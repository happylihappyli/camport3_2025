#include <cstdint>
#include <cstring>
#include <iostream>

enum {
    CV_8UC1 = 0
};

class funny_Mat {
public:
    funny_Mat() : rows_(0), cols_(0), data_(nullptr) {}
    
    funny_Mat(int rows, int cols, int type) 
        : rows_(rows), cols_(cols) {
        data_ = new uint8_t[static_cast<size_t>(rows) * cols];
        memset(data_, 0, static_cast<size_t>(rows) * cols);
    }
    
    ~funny_Mat() {
        if (data_) {
            delete[] data_;
        }
    }
    
    int rows() const { return rows_; }
    int cols() const { return cols_; }
    
    uint8_t* data() { return data_; }
    
    funny_Mat clone() const {
        funny_Mat result(rows_, cols_, 0);
        if (data_) {
            memcpy(result.data_, data_, static_cast<size_t>(rows_) * cols_);
        }
        return result;
    }
    
private:
    int rows_;
    int cols_;
    uint8_t* data_;
};

typedef struct {
    size_t size;
    void* buffer;
    size_t width;
    size_t height;
} TY_IMAGE_DATA;

inline TY_IMAGE_DATA TYInitImageData(size_t size, void* buffer, size_t width, size_t height) {
    TY_IMAGE_DATA data;
    data.size = size;
    data.buffer = buffer;
    data.width = width;
    data.height = height;
    return data;
}

int main() {
    funny_Mat mat(10, 10, CV_8UC1);
    std::cout << "Matrix created: " << mat.rows() << "x" << mat.cols() << std::endl;
    
    funny_Mat cloned = mat.clone();
    std::cout << "Matrix cloned: " << cloned.rows() << "x" << cloned.cols() << std::endl;
    
    if (mat.data()) {
        std::cout << "Data pointer is not null" << std::endl;
        
        size_t size = 10 * 10 * 1;
        void* buffer = mat.data();
        size_t width = 10;
        size_t height = 10;
        TY_IMAGE_DATA image_data = TYInitImageData(size, buffer, width, height);
        std::cout << "TYInitImageData called successfully" << std::endl;
    }
    
    return 0;
}