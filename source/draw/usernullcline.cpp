#include "usernullcline.h"

const int UserNullcline::XRES = 100;

UserNullcline::UserNullcline(DSPlot* plot) : DrawBase(plot)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("UserNullcline::UserNullcline", std::this_thread::get_id());
#endif
}

UserNullcline::~UserNullcline()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("UserNullcline::~UserNullcline", std::this_thread::get_id());
#endif
}

void UserNullcline::ComputeData()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("UserNullcline::ComputeData", std::this_thread::get_id());
#endif
    while (DrawState()==DRAWING)
    {
        if (!NeedNewStep() && !NeedRecompute())
            goto label;{

           FreezeNonUser();
        InitParserMgrs(XRES);

        const int xidx = Spec_toi("xidx"),
                num_ncs = _modelMgr->Model(ds::NC)->NumPars();

        const double xmin = _modelMgr->Minimum(ds::INIT, xidx),
                xmax = _modelMgr->Maximum(ds::INIT, xidx);
        const double xinc = (xmax - xmin) / (double)XRES;

        Record* const record = new Record(num_ncs);
        double* const x = record->x,
                * const y = record->y;

        try
        {
            std::lock_guard<std::mutex> lock( Mutex() );
            RecomputeIfNeeded();

            for (int i=0; i<XRES; ++i)
            {
                const double xij = i*xinc + xmin;

                ParserMgr& parser_mgr = GetParserMgr(i);
                const double* ncs = parser_mgr.ConstData(ds::NC);
                parser_mgr.SetData(ds::DIFF, xidx, xij);
                parser_mgr.ParserEval(false);

                x[i] = xij;
                for (int j=0; j<num_ncs; ++j)
                    y[j*XRES + i] = ncs[j];
            }

            _packets.push_back(record);
        }
        catch (std::exception& e)
        {
            _log->AddExcept("UserNullcline::ComputeData: " + std::string(e.what()));
            throw (e);
        }

        }label:

        std::this_thread::sleep_for( std::chrono::milliseconds(RemainingSleepMs()) );
    }

    if (DeleteOnFinish()) emit ReadyToDelete();
}

void UserNullcline::Initialize()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("UserNullcline::Initialize", std::this_thread::get_id());
#endif
    _colors = *static_cast< const std::vector<QColor>* >( OpaqueSpec("colors") );
    const size_t num_colors = _colors.size();
    SetNeedRecompute(true);
    InitParserMgrs(XRES);

    ClearPlotItems();
    const size_t num_ncs = _modelMgr->Model(ds::DIFF)->NumPars();
    for (size_t i=0; i<num_ncs; ++i)
    {
        QwtPlotCurve* curv = new QwtPlotCurve();
        curv->setPen( _colors.at((i+1)%num_colors), 1 ); //+1 for IP which isn't used here
        curv->setRenderHint( QwtPlotItem::RenderAntialiased, true );
        AddPlotItem(curv);
    }

    while (!_packets.empty())
    {
        delete _packets.front();
        _packets.pop_front();
    }

    DrawBase::Initialize(); //Have to wait on this, since we don't know how many objects there
        //are until the analysis is complete
}

void UserNullcline::MakePlotItems()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("UserNullcline::MakePlotItems", std::this_thread::get_id());
#endif

    std::unique_lock<std::mutex> lock( Mutex() );
    if (_packets.empty()) return;
    while (_packets.size()>1)
    {
        delete _packets.front();
        _packets.pop_front();
    }
    Record* record = _packets.front();
    _packets.pop_front();
    if (!record) return;
    lock.unlock();

    const double* const x = record->x,
            * const y = record->y;
    const int num_ncs = _modelMgr->Model(ds::NC)->NumPars();
    for (int i=0; i<num_ncs; ++i)
    {
        QwtPlotCurve* curv = static_cast<QwtPlotCurve*>( PlotItem(i) );
        QPolygonF nullcline(XRES);
        for (int k=0; k<XRES; ++k)
            nullcline[k] = QPointF( x[k], y[i*XRES+k] );
        curv->setSamples(nullcline);
    }

    delete record;
}
