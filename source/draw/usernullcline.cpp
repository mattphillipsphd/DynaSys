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

        const int xidx = Spec_toi("xidx"),
                yidx = Spec_toi("yidx"),
                num_ncs = _modelMgr->Model(ds::NC)->NumPars();
        const bool has_jacobian = Spec_tob("has_jacobian");

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

            ParserMgr& parser_mgr = GetParserMgr(0);
            const ParamModelBase* const diff_model = _modelMgr->Model(ds::DIFF);
            const double* const resets = parser_mgr.ConstData(ds::INIT);
            const int num_diffs = diff_model->NumPars();
            for (int i=0; i<XRES; ++i)
            {
                const double xij = i*xinc + xmin;

                const double* ncs = parser_mgr.ConstData(ds::NC);
                for (int j=0; j<num_diffs; ++j)
                    if (j==xidx) parser_mgr.SetData(ds::DIFF, xidx, xij);
                    else parser_mgr.SetData(ds::DIFF, j, resets[j]);
                parser_mgr.ParserEval(false);

                x[i] = xij;
                for (int j=0; j<num_ncs; ++j)
                {
                    size_t yidx_j = diff_model->ShortKeyIndex( DependentVar( (size_t)j ) );
                    if (yidx_j != yidx) continue;
                    y[j*XRES + i] = ncs[j];
                }
            }

            for (int i=0; i<XRES-1; ++i)
            {
                double ix, iy;
                bool has_int = LineIntersection(
                            x[i], y[i], x[i+1], y[i+1],
                            x[i], y[XRES+i], x[i+1], y[XRES+i+1],
                            &ix, &iy);
                if (has_int)
                    record->equilibria.push_back( Equilibrium(ix, iy) );
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
std::string UserNullcline::DependentVar(size_t i) const
{
    std::string full_key = _modelMgr->Model(ds::NC)->Key(i);
    size_t pos = full_key.find(ds::UNC_INFIX);
    return full_key.substr(0,pos);
}
std::string UserNullcline::EquationVar(size_t i) const
{
    std::string full_key = _modelMgr->Model(ds::NC)->Key(i);
    size_t pos = full_key.find(ds::UNC_INFIX),
            len = ds::UNC_INFIX.size();
    return full_key.substr(pos+len);
}

void UserNullcline::Initialize()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("UserNullcline::Initialize", std::this_thread::get_id());
#endif
    _colors = *static_cast< const std::vector<QColor>* >( OpaqueSpec("colors") );
    const size_t num_colors = _colors.size();
    SetNeedRecompute(true);
    FreezeNonUser();
    InitParserMgrs(1);

    ClearPlotItems();
    const size_t num_ncs = _modelMgr->Model(ds::NC)->NumPars();
    for (size_t i=0; i<num_ncs; ++i)
    {
        size_t eq_idx = _modelMgr->Model(ds::DIFF)->ShortKeyIndex( EquationVar(i) );
        QwtPlotCurve* curv = new QwtPlotCurve();
        curv->setPen( _colors.at((eq_idx+1)%num_colors), 1 ); //+1 for IP which isn't used here
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
    const size_t yidx = Spec_toi("yidx");
    const bool has_jacobian = Spec_tob("has_jacobian");
    for (int i=0; i<num_ncs; ++i)
    {
        size_t yidx_i = _modelMgr->Model(ds::DIFF)->ShortKeyIndex( DependentVar( (size_t)i ) );
        if (yidx_i != yidx) continue;
        QwtPlotCurve* curv = static_cast<QwtPlotCurve*>( PlotItem(i) );
        QPolygonF nullcline(XRES);
        for (int k=0; k<XRES; ++k)
            nullcline[k] = QPointF( x[k], y[i*XRES+k] );
        curv->setSamples(nullcline);

        if (has_jacobian)
        {
            for (const auto& it : record->equilibria)
            {
                QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
                    QBrush(Qt::black), QPen(Qt::black, 2), QSize(50, 50) );
                QwtPlotMarker* marker = new QwtPlotMarker();
                marker->setSymbol(symbol);
                marker->setXValue(it.x);
                marker->setYValue(it.y);
                marker->setZ(-1);
                marker->setRenderHint( QwtPlotItem::RenderAntialiased, true );
                AddPlotItem(marker);
            }
        }
    }

    delete record;
}

//http://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
//Slight modification of answer by Gavin
// Returns 1 if the lines intersect, otherwise 0. In addition, if the lines
// intersect the intersection point may be stored in the doubles i_x and i_y.
bool UserNullcline::LineIntersection(double p0_x, double p0_y, double p1_x, double p1_y,
    double p2_x, double p2_y, double p3_x, double p3_y, double *i_x, double *i_y)
{
    double s1_x = p1_x - p0_x,
            s1_y = p1_y - p0_y,
            s2_x = p3_x - p2_x,
            s2_y = p3_y - p2_y;

    double s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / (-s2_x * s1_y + s1_x * s2_y),
            t = ( s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / (-s2_x * s1_y + s1_x * s2_y);

    if (s >= 0.0 && s <= 1.0 && t >= 0.0 && t <= 1.0)
    {
        // Collision detected
        if (i_x != NULL)
            *i_x = p0_x + (t * s1_x);
        if (i_y != NULL)
            *i_y = p0_y + (t * s1_y);
        return true;
    }

    return false; // No collision
}
