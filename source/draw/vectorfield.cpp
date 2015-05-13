#include "vectorfield.h"

const int VectorField::DEFAULT_TAIL_LEN = 1;

VectorField::VectorField(DSPlot* plot) : DrawBase(plot)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("VectorField::VectorField", std::this_thread::get_id());
#endif
}
VectorField::~VectorField()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("VectorField::~VectorField", std::this_thread::get_id());
#endif
    if (Data()) delete[] static_cast<QPolygonF*>( Data() );
}

void VectorField::ComputeData()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("VectorField::ComputeData", std::this_thread::get_id());
#endif
    while (DrawState()==DRAWING)
    {
        if (!NeedRecompute() && !NeedNewStep())
            goto label;{

        InitParserMgrs();

        const int xidx = Spec_toi("xidx"),
                yidx = Spec_toi("yidx"),
                tail_len = Spec_toi("tail_length"),
                steps_per_sec = Spec_toi("steps_per_sec");
        const bool grow_tail = Spec_toi("grow_tail");

        const double xmin = _modelMgr->Minimum(ds::INIT, xidx),
                xmax = _modelMgr->Maximum(ds::INIT, xidx),
                ymin = _modelMgr->Minimum(ds::INIT, yidx),
                ymax = _modelMgr->Maximum(ds::INIT, yidx);
        SetSpec("xmin", xmin);
        SetSpec("xmax", xmax);
        SetSpec("ymin", ymin);
        SetSpec("ymax", ymax);
        const double xinc = (xmax - xmin) / (double)(_resolution-1),
                yinc = (ymax - ymin) / (double)(_resolution-1),
                xpix_inc = (double)Plot()->width() / (double)(_resolution-1),
                ypix_inc = (double)Plot()->height() / (double)(_resolution-1);
        SetSpec("xinc", xinc);
        SetSpec("yinc", yinc);
        ArrowHead::SetConversions(xinc, yinc, xpix_inc, ypix_inc);

        try
        {
            std::lock_guard<std::mutex> lock( Mutex() );
            RecomputeIfNeeded();
            QPolygonF* data = new QPolygonF[_resolution*_resolution];
            for (size_t i=0; i<_resolution; ++i)
                for (size_t j=0; j<_resolution; ++j)
                {
                    const int idx = i*_resolution+j;
                    ParserMgr& parser_mgr = GetParserMgr(idx);
                    const double* diffs = parser_mgr.ConstData(ds::DIFF);
                    const double x = i*xinc + xmin,
                                y = j*yinc + ymin;

                    parser_mgr.SetData(ds::DIFF, xidx, x);
                    parser_mgr.SetData(ds::DIFF, yidx, y);
                    QPolygonF& pts = data[i*_resolution+j];
                    pts = QPolygonF(_tailLength+1);
                    pts[0] = QPointF(x, y);
                    for (int k=1; k<=_tailLength; ++k)
                    {
                        parser_mgr.ParserEval(false);
                        pts[k] = QPointF(diffs[xidx], diffs[yidx]);
                    }
                }

            _packets.push_back(data);
        }
        catch (std::exception& e)
        {
            _log->AddExcept("VectorField::ComputeData: " + std::string(e.what()));
            throw (e);
        }

        emit ComputeComplete(_tailLength);

        if (grow_tail)
            _tailLength += steps_per_sec*tail_len;

        }label:
        std::this_thread::sleep_for( std::chrono::milliseconds(RemainingSleepMs()) );
    }

    if (DeleteOnFinish()) emit ReadyToDelete();
}

void VectorField::Initialize()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("VectorField::Initialize", std::this_thread::get_id());
#endif
    _tailLength = DEFAULT_TAIL_LEN;
    InitParserMgrs();
    ResetPlotItems();
}

void VectorField::MakePlotItems()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("VectorField::MakePlotItems", std::this_thread::get_id());
#endif

    std::unique_lock<std::mutex> lock( Mutex() );
    if (_packets.empty()) return;
    while (_packets.size()>1)
    {
        delete[] _packets.front();
        _packets.pop_front();
    }
    QPolygonF* data = _packets.front();
    _packets.pop_front();
    lock.unlock();

    if (!IsSpec("xmin")) return;
    const double xmin = Spec_tod("xmin"),
            ymin = Spec_tod("ymin"),
            xinc = Spec_tod("xinc"),
            yinc = Spec_tod("yinc");

    const size_t tail_length = data[0].size()-1;
    for (size_t i=0; i<_resolution; ++i)
        for (size_t j=0; j<_resolution; ++j)
        {
            const double x = i*xinc + xmin,
                        y = j*yinc + ymin;
            const size_t idx = i*_resolution+j;
            QPolygonF pts = data[idx];

            QwtPlotMarker* marker = static_cast<QwtPlotMarker*>( PlotItem(3*idx + 0) );
            marker->setXValue(x);
            marker->setYValue(y);
            marker->setZ(-1);

            QwtPlotCurve* curv = static_cast<QwtPlotCurve*>( PlotItem(3*idx + 1) );
            curv->setSamples(pts);
            curv->setZ(-0.5);

            QwtPlotCurve* arrow = static_cast<QwtPlotCurve*>( PlotItem(3*idx + 2) );
            const ArrowHead arrow_head(pts[tail_length], pts[tail_length-1]);
            arrow->setSamples(arrow_head.Points());
            arrow->setZ(0);
        }

    delete[] data;
}

void VectorField::InitParserMgrs()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("VectorField::InitParserMgrs", std::this_thread::get_id());
#endif
    _resolution = (size_t)Spec_toi("resolution");
    FreezeNonUser();
    DrawBase::InitParserMgrs(_resolution*_resolution);
}

void VectorField::ResetPlotItems()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("VectorField::ResetPlotItems", std::this_thread::get_id());
#endif
    const size_t resolution2 = _resolution*_resolution;

    ReservePlotItems(resolution2*3);
    for (size_t i=0; i<resolution2; ++i)
    {
        QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
            QBrush(Qt::gray), QPen(Qt::gray, 2), QSize(2, 2) );
        QwtPlotMarker* marker = new QwtPlotMarker();
        marker->setSymbol(symbol);

        QwtPlotCurve* curv = new QwtPlotCurve();
        curv->setPen(Qt::blue, 1);
        curv->setRenderHint( QwtPlotItem::RenderAntialiased, true );

        QwtPlotCurve* arrow = new QwtPlotCurve();
        arrow->setPen(Qt::red, 1);
        arrow->setRenderHint( QwtPlotItem::RenderAntialiased, true );

        AddPlotItem(marker);
        AddPlotItem(curv);
        AddPlotItem(arrow);
    }
    while (!_packets.empty())
    {
        delete[] _packets.front();
        _packets.pop_front();
    }

    DrawBase::Initialize();
}
