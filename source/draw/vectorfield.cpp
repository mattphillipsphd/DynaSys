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

        ResetPPs();

        QPolygonF* data = static_cast<QPolygonF*>( Data() );

        const int xidx = Spec_toi("xidx"),
                yidx = Spec_toi("yidx"),
                tail_len = Spec_toi("tail_length"),
                resolution2 = NumParserMgrs(),
                resolution = (int)std::sqrt(resolution2),
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
        const double xinc = (xmax - xmin) / (double)(resolution-1),
                yinc = (ymax - ymin) / (double)(resolution-1),
                xpix_inc = (double)Plot()->width() / (double)(resolution-1),
                ypix_inc = (double)Plot()->height() / (double)(resolution-1);
        SetSpec("xinc", xinc);
        SetSpec("yinc", yinc);
        ArrowHead::SetConversions(xinc, yinc, xpix_inc, ypix_inc);

        try
        {
            std::lock_guard<std::mutex> lock( Mutex() );
            for (int i=0; i<resolution; ++i)
                for (int j=0; j<resolution; ++j)
                {
                    const int idx = i*resolution+j;
                    ParserMgr& parser_mgr = GetParserMgr(idx);
                    const double* diffs = parser_mgr.ConstData(ds::DIFF);
                    const double x = i*xinc + xmin,
                                y = j*yinc + ymin;

                    parser_mgr.SetData(ds::DIFF, xidx, x);
                    parser_mgr.SetData(ds::DIFF, yidx, y);
                    QPolygonF& pts = data[i*resolution+j];
                    pts = QPolygonF(_tailLength+1);
                    pts[0] = QPointF(x, y);
                    for (int k=1; k<=_tailLength; ++k)
                    {
                        parser_mgr.ParserEval(false);
                        pts[k] = QPointF(diffs[xidx], diffs[yidx]);
                    }
                }
        }
        catch (std::exception& e)
        {
            _log->AddExcept("VectorField::ComputeData: " + std::string(e.what()));
            throw (e);
        }

        MakePlotItems(); //Since these aren't used by any other plot element, no reason
            //not to do this in this thread
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
    ResetPPs();
}

void VectorField::MakePlotItems()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("VectorField::MakePlotItems", std::this_thread::get_id());
#endif
    const QPolygonF* data = static_cast<const QPolygonF*>( ConstData() );

    const int resolution2 = NumParserMgrs(),
            resolution = (int)std::sqrt(resolution2);
    const double xmin = Spec_tod("xmin"),
            ymin = Spec_tod("ymin"),
            xinc = Spec_tod("xinc"),
            yinc = Spec_tod("yinc");

    for (int i=0; i<resolution; ++i)
        for (int j=0; j<resolution; ++j)
        {
            const double x = i*xinc + xmin,
                        y = j*yinc + ymin;
            const int idx = i*resolution+j;
            QPolygonF pts = data[idx];

            QwtPlotMarker* marker = static_cast<QwtPlotMarker*>( PlotItem(3*idx + 0) );
            marker->setXValue(x);
            marker->setYValue(y);
            marker->setZ(-1);

            QwtPlotCurve* curv = static_cast<QwtPlotCurve*>( PlotItem(3*idx + 1) );
            curv->setSamples(pts);
            curv->setZ(-0.5);

            QwtPlotCurve* arrow = static_cast<QwtPlotCurve*>( PlotItem(3*idx + 2) );
            const ArrowHead arrow_head(pts[_tailLength], pts[_tailLength-1]);
            arrow->setSamples(arrow_head.Points());
            arrow->setZ(0);
        }
}

void VectorField::ResetPPs()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("VectorField::ResetPPs", std::this_thread::get_id());
#endif
    const int resolution = Spec_toi("resolution"),
            resolution2 = resolution*resolution;

    FreezeNonUser();
    InitParserMgrs(resolution2);

    ReservePlotItems(resolution2*3);
    for (int i=0; i<resolution2; ++i)
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
    if (Data()) delete[] static_cast<QPolygonF*>( Data() );
    SetData( new QPolygonF[resolution2] );

    DrawBase::Initialize();
}
