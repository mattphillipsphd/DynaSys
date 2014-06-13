#include "dsplot.h"

#include "qwt_plot_dict.h"
#include "qwt_plot_layout.h"
#include "qwt_scale_widget.h"
#include "qwt_scale_engine.h"
#include "qwt_text_label.h"
#include "qwt_legend.h"
#include "qwt_legend_data.h"
#include "qwt_plot_canvas.h"

DSPlot::DSPlot(QWidget *parent) :
    QwtPlot(parent), _tid(std::this_thread::get_id())
{
#ifdef DEBUG_FUNC_DSPLOT
    ScopeTracker st("DSPlot::DSPlot", std::this_thread::get_id());
    std::stringstream s; s << std::this_thread::get_id();
#endif

    canvas()->setMouseTracking(true);
    setMouseTracking(true);
}
DSPlot::DSPlot( const QwtText &title, QWidget *parent ):
    QwtPlot(title, parent), _tid(std::this_thread::get_id())
{
}
DSPlot::~DSPlot()
{
}

QwtScaleMap DSPlot::canvasMap( int axisId ) const
{
#ifdef DEBUG_FUNC_DSPLOT
    ScopeTracker st("DSPlot::canvasMap", std::this_thread::get_id());
    assert(_tid == std::this_thread::get_id() && "Qwt called from worker thread!");
#endif
    return QwtPlot::canvasMap(axisId);
}

QSize DSPlot::sizeHint() const
{
#ifdef DEBUG_FUNC_DSPLOT
    ScopeTracker st("DSPlot::sizeHint", std::this_thread::get_id());
    assert(_tid == std::this_thread::get_id() && "Qwt called from worker thread!");
#endif
    return QwtPlot::sizeHint();
}
QSize DSPlot::minimumSizeHint() const
{
#ifdef DEBUG_FUNC_DSPLOT
    ScopeTracker st("DSPlot::minimumSizeHint", std::this_thread::get_id());
    assert(_tid == std::this_thread::get_id() && "Qwt called from worker thread!");
#endif
    return QwtPlot::minimumSizeHint();
}

void DSPlot::updateLayout()
{
#ifdef DEBUG_FUNC_DSPLOT
    ScopeTracker st("DSPlot::updateLayout", std::this_thread::get_id());
    assert(_tid == std::this_thread::get_id() && "Qwt called from worker thread!");
#endif
    QwtPlot::updateLayout();
}
void DSPlot::drawCanvas( QPainter * painter)
{
#ifdef DEBUG_FUNC_DSPLOT
    ScopeTracker st("DSPlot::drawCanvas", std::this_thread::get_id());
    assert(_tid == std::this_thread::get_id() && "Qwt called from worker thread!");
#endif
    return QwtPlot::drawCanvas(painter);
}


void DSPlot::getCanvasMarginsHint(
    const QwtScaleMap maps[], const QRectF &canvasRect,
    double &left, double &top, double &right, double &bottom) const
{
#ifdef DEBUG_FUNC_DSPLOT
    ScopeTracker st("DSPlot::getCanvasMarginsHint", std::this_thread::get_id());
    assert(_tid == std::this_thread::get_id() && "Qwt called from worker thread!");
#endif
    QwtPlot::getCanvasMarginsHint(maps, canvasRect, left, top, right, bottom);
}

bool DSPlot::event( QEvent * event)
{
#ifdef DEBUG_FUNC_DSPLOT
    ScopeTracker st("DSPlot::event", std::this_thread::get_id());
    Log.AddMesg("DSPlot::event: " + std::to_string(event) + ", " + std::to_string(event->type());
    //2 is QEvent::MouseButtonPress
    //3 is QEvent::MouseButtonRelease
    //4 is QEvent::MouseButtonDblClick
    //6 is QEvent::KeyPress
    //7 is QEvent::KeyRelease
    //8 is QEvent::FocusIn
    //9 is QEvent::FocusOut
    //10 is QEvent::Enter
    //11 is QEvent::Leave
    //12 is QEvent::Paint
    //13 is QEvent::Move
    //14 is QEvent::Resize
    //18 is QEvent::Hide
    //27 is QEvent::HideToParent
    //76 is QEvent::LayoutRequest
    //78 is QEvent::UpdateLater
    assert(_tid == std::this_thread::get_id() && "Qwt called from worker thread ");
#endif
    return QwtPlot::event(event);
}

bool DSPlot::eventFilter( QObject *object, QEvent * event)
{
#ifdef DEBUG_FUNC_DSPLOT
    ScopeTracker st("DSPlot::eventFilter", std::this_thread::get_id());
    assert(_tid == std::this_thread::get_id() && "Qwt called from worker thread!");
#endif
    return QwtPlot::eventFilter(object, event);
}


void DSPlot::drawItems( QPainter *painter, const QRectF &canvasRect,
                        const QwtScaleMap maps[axisCnt]) const
{
#ifdef DEBUG_FUNC_DSPLOT
    ScopeTracker st("DSPlot::drawItems", std::this_thread::get_id());
    assert(_tid == std::this_thread::get_id() && "Qwt called from worker thread!");
#endif
    QwtPlot::drawItems(painter, canvasRect, maps);
}


QVariant DSPlot::itemToInfo( QwtPlotItem * plotItem) const
{
#ifdef DEBUG_FUNC_DSPLOT
    ScopeTracker st("DSPlot::itemToInfo", std::this_thread::get_id());
    assert(_tid == std::this_thread::get_id() && "Qwt called from worker thread!");
#endif
    return QwtPlot::itemToInfo(plotItem);
}

QwtPlotItem *DSPlot::infoToItem( const QVariant & itemInfo) const
{
#ifdef DEBUG_FUNC_DSPLOT
    ScopeTracker st("DSPlot::infoToItem", std::this_thread::get_id());
    assert(_tid == std::this_thread::get_id() && "Qwt called from worker thread!");
#endif
    return QwtPlot::infoToItem(itemInfo);
}
void DSPlot::mousePressEvent(QMouseEvent*)
{
    emit MouseClick();
}
void DSPlot::mouseMoveEvent(QMouseEvent* event)
{
    QwtScaleMap mapx = canvasMap(xBottom),
             mapy = canvasMap(yLeft);
    emit MousePos( QPointF(mapx.invTransform(event->x() - canvas()->pos().x()),
                           mapy.invTransform(event->y() - canvas()->pos().y())));
}

void DSPlot::replot()
{
#ifdef DEBUG_FUNC_DSPLOT
    ScopeTracker st("DSPlot::replot", std::this_thread::get_id());
    assert(_tid == std::this_thread::get_id() && "Qwt called from worker thread!");
#endif
    QwtPlot::replot();
}

void DSPlot::resizeEvent( QResizeEvent *e )
{
#ifdef DEBUG_FUNC_DSPLOT
    ScopeTracker st("DSPlot::resizeEvent", std::this_thread::get_id());
    assert(_tid == std::this_thread::get_id() && "Qwt called from worker thread ");
#endif
    QwtPlot::resizeEvent(e);
}


