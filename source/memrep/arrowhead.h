#ifndef ARROWHEAD_H
#define ARROWHEAD_H

#include <QPolygonF>

#include "../globals/globals.h"

class ArrowHead
{
    public:
        ArrowHead(const QPointF& a, const QPointF& b);

        static void SetConversions(double xval, double yval, double xpix, double ypix);

        const QPolygonF& Points() const { return _pts; }

    private:
        inline QPolygonF MakePoints(const QPointF& a, const QPointF& b);

        static const double _length; //As fraction of dx, dy
        static const double _width;
        static double _xPixInc, _yPixInc,
            _xPix2Val, _yPix2Val;
        const QPolygonF _pts;
};

#endif // ARROWHEAD_H
