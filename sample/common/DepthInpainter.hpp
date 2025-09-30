#ifndef XYZ_INPAINTER_HPP_
#define XYZ_INPAINTER_HPP_

#include "funny_Mat.hpp"
#include "ImageSpeckleFilter.hpp"


class DepthInpainter
{
public:
    int         _kernelSize;
    int         _maxInternalHoleToBeFilled;
    double      _inpaintRadius;
    bool        _fillAll;


    DepthInpainter()
        : _kernelSize(5)
        , _maxInternalHoleToBeFilled(50)
        , _inpaintRadius(1)
        , _fillAll(true)
    {
    }

    void inpaint(const funny_Mat& inputDepth, funny_Mat& out, const funny_Mat& mask);

private:
    funny_Mat genValidMask(const funny_Mat& depth);
};

#endif