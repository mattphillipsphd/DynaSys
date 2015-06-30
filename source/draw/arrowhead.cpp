#include "arrowhead.h"

const double ArrowHead::_length = 0.2;
const double ArrowHead::_width = ds::PI/8.0;
double ArrowHead::_xPixInc = 0.0;
double ArrowHead::_xPix2Val = 0.0;
double ArrowHead::_xVal2Pix = 0.0;
double ArrowHead::_yPixInc = 0.0;
double ArrowHead::_yPix2Val = 0.0;
double ArrowHead::_yVal2Pix = 0.0;

ArrowHead::ArrowHead(const QPointF& a, const QPointF& b) : _pts(MakePoints(a,b))
{
}

void ArrowHead::SetConversions(double xval, double yval, double xpix, double ypix)
{
    _xPixInc = xpix;
    _yPixInc = ypix;
    _xPix2Val = xval/xpix;
    _yPix2Val = yval/ypix;
    _xVal2Pix = xpix/xval;
    _yVal2Pix = ypix/yval;
}

QPolygonF ArrowHead::MakePoints(const QPointF& a, const QPointF& b)
{
    QPolygonF pts(3);
    const double lastx = a.x(),
            lasty = a.y();
    pts[1] = QPointF(lastx, lasty);
    const double theta = std::atan2( _yVal2Pix*(b.y()-lasty), _xVal2Pix*(b.x()-lastx) ),
            length = _length*std::hypot(_xPixInc, _yPixInc);
    const double
            p1x = lastx + length*cos(theta + _width)*_xPix2Val,
            p1y = lasty + length*sin(theta + _width)*_yPix2Val,
            p2x = lastx + length*cos(theta - _width)*_xPix2Val,
            p2y = lasty + length*sin(theta - _width)*_yPix2Val;
    pts[0] = QPointF(p1x, p1y);
    pts[2] = QPointF(p2x, p2y);
    return pts;
}
