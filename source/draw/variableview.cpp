#include "variableview.h"

const int VariableView::NUM_ZFUNCS = 11;
const int VariableView::NUM_INCREMENTS = 200;

VariableView::VariableView(DSPlot* plot) : DrawBase(plot), _numFuncs(-1)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("VariableView::VariableView", std::this_thread::get_id());
#endif
}

VariableView::~VariableView()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("VariableView::~VariableView", std::this_thread::get_id());
#endif
    if (Data()) delete[] static_cast<QPolygonF*>( Data() );
}

void* VariableView::DataCopy() const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("VariableView::DataCopy", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock( Mutex() );
    QPolygonF* data = new QPolygonF[_numFuncs];
    auto cdata = static_cast<const QPolygonF*>( ConstData() );
    for (size_t i=0; i<_numFuncs; ++i)
        data[i] = cdata[i];
    return data;
}

void VariableView::ComputeData()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("VariableView::ComputeData", std::this_thread::get_id());
#endif
    SetDrawState(DrawBase::DRAWING);

    while (DrawState()==DRAWING)
    {
        if (!NeedRecompute() && !NeedNewStep())
            goto label;{

        const size_t xidx_raw = Spec_toi("xidx"),
                yidx = Spec_toi("yidx"),
                zidx_raw = Spec_toi("zidx");
        const int tail_len = Spec_toi("tail_length");
        const VSpec xspec = MakeVSpec(xidx_raw, NUM_INCREMENTS),
                zspec = MakeVSpec(zidx_raw, _numFuncs);
        const size_t xidx = xspec.idx,
                    zidx = zspec.idx;
        ds::PMODEL xmi = xspec.mi,
                zmi = zspec.mi;
        const double xinc = xspec.inc,
                xmax = xspec.max,
                xmin = xspec.min,
                zval = std::stod( _modelMgr->Value(zmi==ds::INP?zmi:ds::INIT,zidx) ),
                zinc = zspec.inc/10.0,
                zmin = zval - zinc*(_numFuncs/2);
                //So, we cluster the range of functions tightly around the range of z

        QPolygonF* points = new QPolygonF[_numFuncs];
        for (int k=0; k<_numFuncs; ++k)
            points[k] = QPolygonF(NUM_INCREMENTS);
        double ymin = std::numeric_limits<double>::max(),
                ymax = -std::numeric_limits<double>::max();
        const int num_steps = tail_len<1 ? 1 : tail_len;

        try
        {
            std::lock_guard<std::mutex> lock( Mutex() );
            for (int k=0; k<_numFuncs; ++k)
            {
                const double zval = zmin+k*zinc;
                for (int i=0; i<NUM_INCREMENTS; ++i)
                {
                    const double xval = xmin+i*xinc;
                    ParserMgr& parser_mgr = GetParserMgr(k*_numFuncs+i);
                    for (int j=0; j<num_steps; ++j)
                    {
                        parser_mgr.SetData(xmi, xidx, xval);
                        parser_mgr.SetData(zmi, zidx, zval);
                        parser_mgr.ParserEval(false);
                    }
                    const double yval = parser_mgr.ConstData(ds::VAR)[yidx];
                    if (yval<ymin) ymin = yval;
                    if (yval>ymax) ymax = yval;
                    points[k][i] = QPointF(xval, yval);
                }
            }
        }
        catch (std::exception& e)
        {
            _log->AddExcept("VariableView::ComputeData: " + std::string(e.what()));
            throw (e);
        }
        SetData(points);
        SetSpec("xmin", xmin);
        SetSpec("xmax", xmax);
        SetSpec("ymin", ymin);
        SetSpec("ymax", ymax);

        MakePlotItems();

        emit ComputeComplete(num_steps);

        }label:
        std::this_thread::sleep_for( std::chrono::milliseconds(RemainingSleepMs()) );
    }
}

void VariableView::Initialize()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("VariableView::Initialize", std::this_thread::get_id());
#endif
    FreezeNonUser();
    _numFuncs = Spec_toi("use_z")==0 ? 1 : NUM_ZFUNCS;
    InitParserMgrs(NUM_INCREMENTS*_numFuncs);

    ClearPlotItems();
    const double cinc = _numFuncs==1 ? 0 : 255.0 / (double)(_numFuncs-1);
    const int mid = _numFuncs/2;
    for (int k=0; k<_numFuncs; ++k)
    {
        QwtPlotCurve* curve = new QwtPlotCurve;
        curve->setPen( QColor(255 - k*cinc, 0.25, k*cinc), (k==mid)?3:1 );
        AddPlotItem(curve);
    }

    DrawBase::Initialize();
}

void VariableView::MakePlotItems()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("VariableView::MakePlotItems", std::this_thread::get_id());
#endif
    QPolygonF* points = static_cast<QPolygonF*>( DataCopy() );

    for (int k=0; k<_numFuncs; ++k)
    {
        QwtPlotCurve* curve = static_cast<QwtPlotCurve*>( PlotItem(k) );
        curve->setSamples(points[k]);
    }
}

VariableView::VSpec VariableView::MakeVSpec(size_t raw_idx, double num_divs)
{
    const size_t num_inputs = _modelMgr->Model(ds::INP)->NumPars();
    ds::PMODEL mi, range_mi;
    size_t idx;
    if (raw_idx<num_inputs)
    {
        range_mi = mi = ds::INP;
        idx = raw_idx;
    }
    else
    {
        mi = ds::DIFF;
        range_mi = ds::INIT;
        idx = raw_idx - num_inputs;
    }
    const double min = _modelMgr->Minimum(range_mi, idx),
            max = _modelMgr->Maximum(range_mi, idx);
    double inc = num_divs==1 ? 0 : (max - min)/((double)num_divs - 1);
#ifdef __GNUC__
    const VSpec vspec = {idx, inc, max, min, mi};
#else
    VSpec vspec; vspec.idx = idx; vspec.inc = inc; vspec.max = max; vspec.min = min; vspec.mi = mi;
#endif
    return vspec;
}

