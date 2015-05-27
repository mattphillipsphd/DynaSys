#include "usernullcline.h"

const int UserNullcline::XRES = 100;

UserNullcline::UserNullcline(DSPlot* plot) : DrawBase(plot)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("UserNullcline::UserNullcline", std::this_thread::get_id());
#endif
    SetDeleteOnFinish(true);
}

UserNullcline::~UserNullcline()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("UserNullcline::~UserNullcline", std::this_thread::get_id());
#endif
}

void UserNullcline::ClearEquilibria()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("UserNullcline::ClearEquilibria", std::this_thread::get_id());
#endif
    for (auto it : _eqMarkers)
    {
        it->detach();
        RemovePlotItem(it);
    }
    _eqMarkers.clear();
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
                num_ncs = (int)_modelMgr->Model(ds::NC)->NumPars();
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
            const double* const resets = parser_mgr.ConstData(ds::INIT),
                    * const dcurrent = parser_mgr.ConstData(ds::DIFF),
                    * const vcurrent = parser_mgr.ConstData(ds::VAR);
            const int num_diffs = (int)diff_model->NumPars(),
                    num_vars = (int)_modelMgr->Model(ds::VAR)->NumPars();
            for (int i=0; i<XRES; ++i)
            {
                //Set up the model
                const double xij = i*xinc + xmin;
                for (int j=0; j<num_diffs; ++j)
                    if (j==xidx)
                        parser_mgr.SetData(ds::DIFF, j, xij);
                    else if (j==yidx) //Shouldn't matter at all what this gets set to
                        parser_mgr.SetData(ds::DIFF, j, resets[j]);
                    else
                        parser_mgr.SetData(ds::DIFF, j, dcurrent[j]);
                for (int j=0; j<num_vars; ++j)
                    parser_mgr.SetData(ds::VAR, j, vcurrent[j]);

                //Evaluate the model
                parser_mgr.ParserEval(false);

                //Retrieve the results
                x[i] = xij;               
                const double* ncs = parser_mgr.ConstData(ds::NC);
                for (int j=0; j<num_ncs; ++j)
                {
                    int yidx_j = (int)diff_model->ShortKeyIndex( DependentVar( (size_t)j ) );
                    if (yidx_j != yidx) continue;
                    y[j*XRES + i] = ncs[j];
                }
            }

            if (has_jacobian)
            {
                for (int i=0; i<XRES-1; ++i)
                {
                    double ix, iy;
                    bool has_int = LineIntersection(
                                x[i], y[i], x[i+1], y[i+1],
                                x[i], y[XRES+i], x[i+1], y[XRES+i+1],
                                &ix, &iy);
                    if (has_int)
                        record->equilibria.push_back( ds::Equilibrium(ix, iy) );
                }

                const double* jacob_vec = parser_mgr.ConstData(ds::JAC);
                const size_t num_eqs = record->equilibria.size();
                for (size_t i=0; i<num_eqs; ++i)
                {
                    parser_mgr.SetData(ds::DIFF, xidx, record->equilibria.at(i).x);
                    parser_mgr.SetData(ds::DIFF, yidx, record->equilibria.at(i).y);
                    parser_mgr.ParserEval(false);

                    record->equilibria[i].eq_cat = EquilibriumCat(jacob_vec, num_diffs);
                }
            }

            for (int j=0; j<num_vars; ++j)
                parser_mgr.SetData(ds::VAR, j, vcurrent[j]);
            for (int j=0; j<num_diffs; ++j)
                parser_mgr.SetData(ds::DIFF, j, dcurrent[j]);

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

    if (DrawState()==STOPPED && DeleteOnFinish()) emit ReadyToDelete();
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
    const int num_ncs = (int)_modelMgr->Model(ds::NC)->NumPars();
    const size_t yidx = Spec_toi("yidx");
    const bool has_jacobian = Spec_tob("has_jacobian");
    if (has_jacobian)
        ClearEquilibria();
    for (int i=0; i<num_ncs; ++i)
    {
        size_t yidx_i = _modelMgr->Model(ds::DIFF)->ShortKeyIndex( DependentVar( (size_t)i ) );
        if (yidx_i != yidx) continue;
        QwtPlotCurve* curv = static_cast<QwtPlotCurve*>( PlotItem(i) );
        QPolygonF nullcline(XRES);
        for (int k=0; k<XRES; ++k)
            nullcline[k] = QPointF( x[k], y[i*XRES+k] );
        curv->setSamples(nullcline);
    }

    emit Flag_pv(nullptr); //Clear the vector of equilibria
    if (has_jacobian)
        for (const auto& it : record->equilibria)
        {
            QBrush brush;
            switch (it.eq_cat)
            {
                case ds::UNKNOWN:
                    continue;
                case ds::STABLE_NODE:
                case ds::STABLE_FOCUS:
                    brush = QBrush(Qt::black);
                    break;
                case ds::SADDLE:
                    brush = QBrush(Qt::gray);
                    break;
                case ds::UNSTABLE_NODE:
                case ds::UNSTABLE_FOCUS:
                    brush = QBrush(Qt::white);
                    break;
            }

            QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
                brush, QPen(Qt::black, 1), QSize(12, 12) );
            QwtPlotMarker* marker = new QwtPlotMarker();
            marker->setSymbol(symbol);
            marker->setXValue(it.x);
            marker->setYValue(it.y);
            marker->setZ(0.5);
            marker->setRenderHint( QwtPlotItem::RenderAntialiased, true );
            AddPlotItem(marker);
            _eqMarkers.push_back(marker);
            emit Flag_pv( new ds::Equilibrium(it) );
        }

    if (has_jacobian)
        DrawBase::Initialize();

    delete record;
}

double UserNullcline::Determinant2D(const double *mat, int size) const
{
    assert(size == 2);
    return mat[0]*mat[3] - mat[1]*mat[2];
}

double UserNullcline::Determinant3D(const double *mat, int size) const
{
    assert(size == 3);
    //0 1 2
    //3 4 5
    //6 7 8
    const double aminor[4] = {mat[4], mat[5], mat[7], mat[8]},
            bminor[4] = {mat[3], mat[5], mat[6], mat[8]},
            cminor[4] = {mat[3], mat[4], mat[6], mat[7]};
    return mat[0]*Determinant2D(aminor, size-1)
            - mat[1]*Determinant2D(bminor, size-1)
            + mat[2]*Determinant2D(cminor, size-1);
}

ds::EQ_CAT UserNullcline::EquilibriumCat(const double* mat, int size) const
{
    ds::EQ_CAT eq_cat(ds::UNKNOWN);
    const double trace = Trace(mat, size),
            det = (size==2) ? Determinant2D(mat, size) : Determinant3D(mat, size);
    if (det<0)
        eq_cat = ds::SADDLE;
    else
    {
        if (trace<0)
        {
            if (trace*trace > 4*det) eq_cat = ds::STABLE_FOCUS;
            else eq_cat = ds::STABLE_NODE;
        }
        else
        {
            if (trace*trace > 4*det) eq_cat = ds::UNSTABLE_NODE;
            eq_cat = ds::UNSTABLE_FOCUS;
        }
    }

    return eq_cat;
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

double UserNullcline::Trace(const double* mat, int size) const
{
    double trace = mat[0];
    for (int i=1; i<size; ++i)
        trace += mat[(i+1)*(i+1) - 1];
    return trace;
}

