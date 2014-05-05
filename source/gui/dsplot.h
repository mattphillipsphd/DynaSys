#ifndef DSPLOT_H
#define DSPLOT_H

#include <sstream>
#include <thread>
#include <assert.h>

#include <QDebug>
#include <QEvent>
#include <QMouseEvent>

#include "../globals/globals.h"
#include "../globals/scopetracker.h"
#include <qwt_plot.h>

#ifdef DEBUG_FUNC
#define DEBUG_FUNC_DSPLOT
#endif
#undef DEBUG_FUNC_DSPLOT

class DSPlot : public QwtPlot
{
    Q_OBJECT
    Q_PROPERTY( QBrush canvasBackground
        READ canvasBackground WRITE setCanvasBackground )
    Q_PROPERTY( bool autoReplot READ autoReplot WRITE setAutoReplot )

    public:
        explicit DSPlot( QWidget * = nullptr );
        explicit DSPlot( const QwtText &title, QWidget * = nullptr );
        virtual ~DSPlot() override;

        virtual QwtScaleMap canvasMap( int axisId ) const override;

        virtual QSize sizeHint() const override;
        virtual QSize minimumSizeHint() const override;

        virtual void updateLayout() override;
        virtual void drawCanvas( QPainter * ) override;

        virtual void getCanvasMarginsHint(
            const QwtScaleMap maps[], const QRectF &canvasRect,
            double &left, double &top, double &right, double &bottom) const override;

        virtual bool event( QEvent * ) override;
        virtual bool eventFilter( QObject *, QEvent * ) override;

        virtual void drawItems( QPainter *, const QRectF &,
            const QwtScaleMap maps[axisCnt] ) const override;

        virtual QVariant itemToInfo( QwtPlotItem * ) const override;
        virtual QwtPlotItem *infoToItem( const QVariant & ) const override;

    public Q_SLOTS:
        virtual void replot() override;

        virtual void mouseMoveEvent(QMouseEvent* event) override;

    signals:
        void MousePos(QPointF pos);

    protected:
        virtual void resizeEvent( QResizeEvent *e ) override;

    private:
        const std::thread::id _tid;

};

#endif // DSPLOT_H
