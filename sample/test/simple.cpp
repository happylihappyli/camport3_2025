#include <cstdint>
#include <cstring>
#include <iostream>

class funny_Mat {
public:
    funny_Mat(int rows, int cols) : rows_(rows), cols_(cols) {
        data_ = new uint8_t[static_cast<size_t>(rows) * cols];
        memset(data_, 0, static_cast<size_t>(rows) * cols);
    }
    
    ~funny_Mat() {
        delete[] data_;
    }
    
    int rows() const { return rows_; }
    int cols() const { return cols_; }
    
private:
    int rows_;
    int cols_;
    uint8_t* data_;
};

int main() {
    funny_Mat mat(10, 10);
    std::cout << "Matrix: " << mat.rows() << "x" << mat.cols() << std::endl;
    return 0;
}