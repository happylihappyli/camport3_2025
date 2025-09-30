#include "MatViewer.hpp"
#include <cstdio>

// 静态成员变量初始化
int GraphicItem::globalID = 0;

// GraphicItem类的构造函数实现
GraphicItem::GraphicItem(const funny_Scalar& color) : _id(++globalID), _color(color) {}

// GraphicItem类的析构函数实现
GraphicItem::~GraphicItem() {}

// GraphicItem类的id()方法实现
int GraphicItem::id() const { return _id; }

// GraphicItem类的color()方法实现
funny_Scalar GraphicItem::color() const { return _color; }

// GraphicItem类的setColor()方法实现
void GraphicItem::setColor(const funny_Scalar& color) { _color = color; }

// OpencvViewer类的构造函数实现
OpencvViewer::OpencvViewer(const std::string& win) : _win(win) {
    _has_win = 0;
}

// OpencvViewer类的析构函数实现
OpencvViewer::~OpencvViewer() {
    if (_has_win) {
        // 注意：OpenCV窗口相关功能已移除
    }
}

// OpencvViewer类的name()方法实现
const std::string& OpencvViewer::name() const { return _win; }

// OpencvViewer类的show()方法实现
void OpencvViewer::show(const funny_Mat& img)
{
    _has_win = 1;
    _orgImg = img.clone();
    showImage();
}

// OpencvViewer类的onMouseCallback()方法实现
void OpencvViewer::onMouseCallback(funny_Mat& /*img*/, int /*event*/, const funny_Point /*pnt*/, bool& repaint)
{
    repaint = false;
}

// OpencvViewer类的addGraphicItem()方法实现
void OpencvViewer::addGraphicItem(GraphicItem* item) { _items.insert(std::make_pair(item->id(), item)); }

// OpencvViewer类的delGraphicItem()方法实现
void OpencvViewer::delGraphicItem(GraphicItem* item) { _items.erase(item->id()); }

// OpencvViewer类的showImage()方法实现
void OpencvViewer::showImage()
{
    // 复制原始图像到显示图像
    _showImg = _orgImg.clone();
    
    // 绘制所有图形项
    for (auto& itemPair : _items)
    {
        itemPair.second->draw(_showImg);
    }
}

// DepthViewer类的构造函数实现
DepthViewer::DepthViewer(const std::string& win) : OpencvViewer(win)
{
    depth_scale_unit = 1.0f;
}

// DepthViewer类的show()方法实现
void DepthViewer::show(const funny_Mat& depthImage)
{
    // 保存深度图像
    _depth = depthImage.clone();
    
    // 调用父类的show方法
    OpencvViewer::show(_depth);
}

// DepthViewer类的onMouseCallback()方法实现
void DepthViewer::onMouseCallback(funny_Mat& img, int event, const funny_Point pnt, bool& repaint)
{
    // 调用父类的onMouseCallback方法
    OpencvViewer::onMouseCallback(img, event, pnt, repaint);
    
    // 检查点是否在图像范围内
    if (pnt.x >= 0 && pnt.x < img.cols() && pnt.y >= 0 && pnt.y < img.rows())
    {
        // 从深度图像获取深度值
        if (_depth.type() == CV_16U || _depth.type() == CV_16UC1)
        {
            // 这里可以添加深度图像的特殊处理
            repaint = true;
        }
    }
}