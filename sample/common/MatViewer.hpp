#ifndef MAT_VIEWER_HPP_
#define MAT_VIEWER_HPP_

#include <string>
#include <map>
#include "funny_Mat.hpp"

class GraphicItem
{
public:
    GraphicItem(const funny_Scalar& color = funny_Scalar(255, 255, 255));
    virtual ~GraphicItem();

    int id() const;
    funny_Scalar color() const;
    void setColor(const funny_Scalar& color);

    virtual void draw(funny_Mat& img) = 0;

protected:
    int _id;
    funny_Scalar _color;

private:
    static int globalID;
};

class OpencvViewer
{
public:
    OpencvViewer(const std::string& win);
    virtual ~OpencvViewer();

    const std::string& name() const;
    virtual void show(const funny_Mat& img);
    virtual void onMouseCallback(funny_Mat& img, int event, const funny_Point pnt, bool& repaint);

    void addGraphicItem(GraphicItem* item);
    void delGraphicItem(GraphicItem* item);

protected:
    void showImage();

    funny_Mat _orgImg;
    funny_Mat _showImg;
    int _has_win;
    std::string _win;
    std::map<int, GraphicItem*> _items;
};

class DepthViewer : public OpencvViewer
{
public:
    DepthViewer(const std::string& win);
    virtual void show(const funny_Mat& depthImage);
    virtual void onMouseCallback(funny_Mat& img, int event, const funny_Point pnt, bool& repaint);

    float depth_scale_unit;
    
private:
    funny_Mat _depth;
};

#endif // MAT_VIEWER_HPP_