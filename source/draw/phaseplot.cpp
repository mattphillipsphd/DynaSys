#include "phaseplot.h"

PhasePlot::PhasePlot(DSPlot* plot) : DrawBase(plot),
    _pastDVSampsCt(0), _pastIPSampsCt(0)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("PhasePlot::PhasePlot", std::this_thread::get_id());
#endif
}

PhasePlot::~PhasePlot()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("PhasePlot::~PhasePlot", std::this_thread::get_id());
#endif
    auto data = static_cast< std::tuple<std::deque<double>,DataVec,DataVec>* >( Data() );
    if (data) delete data;
}

void* PhasePlot::DataCopy() const
{
    auto data_tuple = static_cast<
            const std::tuple<std::deque<double>,DataVec,DataVec>* >( ConstData() );
    if (!data_tuple) return nullptr;
    std::deque<double> inner_product = std::get<0>(*data_tuple);
    DataVec diff_pts = std::get<1>(*data_tuple),
             var_pts = std::get<2>(*data_tuple);
    auto out = new std::tuple<std::deque<double>,DataVec,DataVec>(
                inner_product,
                diff_pts,
                var_pts);
    return out;
}
int PhasePlot::SleepMs() const
{
    return _makePlots ? 50 : 0;
}

void PhasePlot::ComputeData()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("PhasePlot::ComputeData", std::this_thread::get_id());
#endif
    std::unique_lock<std::recursive_mutex> lock(Mutex());
    //Get all of the information from the parameter fields, introducing new variables as needed.
    ParserMgr& parser_mgr = GetParserMgr(0);
    const int num_diffs = (int)_modelMgr->Model(ds::DIFF)->NumPars(),
            num_vars = (int)_modelMgr->Model(ds::VAR)->NumPars();
    const double* diffs = parser_mgr.ConstData(ds::DIFF),
            * vars = parser_mgr.ConstData(ds::VAR);
        //variables, differential equations, and initial conditions, all of which can invoke named
        //values
    auto data = static_cast< std::tuple<std::deque<double>,DataVec,DataVec>* >( Data() );
    std::deque<double>& inner_product = std::get<0>(*data);
    DataVec& diff_pts = std::get<1>(*data);
    DataVec& var_pts = std::get<2>(*data);
    const bool is_recording = Spec_tob("is_recording");

    QFile temp(ds::TEMP_FILE.c_str());
    std::string output;
    if (is_recording)
    {
        temp.open(QFile::WriteOnly | QFile::Text);
        for (size_t i=0; i<(size_t)num_diffs; ++i)
            output += _modelMgr->Model(ds::DIFF)->ShortKey(i) + "\t";
        for (size_t i=0; i<(size_t)num_vars; ++i)
            output += _modelMgr->Model(ds::VAR)->ShortKey(i)+ "\t";
        output += "\n";
        temp.write(output.c_str());
        temp.flush();
    }

    while (DrawState()==DRAWING)
    {
        if (!NeedNewStep())
            goto label;{

        lock.lock();
        //Get values which may be updated from main thread
        int num_tp_samples = std::stoi(Spec("num_tp_samples")),
                pulse_steps_remaining = std::stoi(Spec("pulse_steps_remaining"));

        //Update the state vector with the value of the differentials.
            //Number of iterations to calculate in this refresh
        int steps_per_sec = Spec_toi("steps_per_sec");
        int num_steps = _makePlots
                ? ((double)steps_per_sec/_modelMgr->ModelStep()) / (double)SleepMs() + 0.5
                : 100;
        if (num_steps==0) num_steps = 1;
        if (num_steps>MAX_BUF_SIZE) num_steps = MAX_BUF_SIZE;

            //Shrink the buffers if need be
        const int xy_buf_over = (int)diff_pts.at(0).size() + num_steps - MAX_BUF_SIZE;
        if (xy_buf_over>0)
        {
            for (int i=0; i<num_diffs; ++i)
                diff_pts[i].erase(diff_pts[i].begin(), diff_pts[i].begin()+xy_buf_over);
            for (int i=0; i<num_vars; ++i)
                var_pts[i].erase(var_pts[i].begin(), var_pts[i].begin()+xy_buf_over);
            _pastDVSampsCt += xy_buf_over;
        }
        const int ip_buf_over = (int)inner_product.size() + num_steps - num_tp_samples;
        if (ip_buf_over>0)
        {
            inner_product.erase(inner_product.begin(), inner_product.begin()+ip_buf_over);
            _pastIPSampsCt += ip_buf_over;
        }

        //Go through each expression and evaluate them
        try
        {
            RecomputeIfNeeded();
            std::lock_guard<std::recursive_mutex> lock( Mutex() );
            for (int k=0; k<num_steps; ++k)
            {
                parser_mgr.ParserEvalAndConds();

                if (pulse_steps_remaining>0) --pulse_steps_remaining;
                if (pulse_steps_remaining==0)
                {
                    pulse_steps_remaining = -1;
                    emit Flag1();
                }

                //Record updated variables for 2d graph, inner product, and output file
                double ip_k = 0;
                for (int i=0; i<num_diffs; ++i)
                {
                    double diffs_i = diffs[i];
                    diff_pts[i].push_back(diffs_i);
                    ip_k += diffs_i * diffs_i;
                }
                inner_product.push_back(ip_k);
                for (int i=0; i<num_vars; ++i)
                    var_pts[i].push_back( vars[i] );

                if (is_recording)
                {
                    output.clear();
                    for (int i=0; i<num_diffs; ++i)
                        output += std::to_string(diffs[i]) + "\t";
                    for (int i=0; i<num_vars; ++i)
                        output += std::to_string(vars[i]) + "\t";
                    output += "\n";
                    temp.write(output.c_str());
                    temp.flush();
                }
                    //Saving *every* sample is incredibly unwieldy--need a parameter to control
                    //when to save values
            }
        }
        catch (mu::ParserError& e)
        {
            _log->AddExcept("PhasePlot::ComputeData: " + e.GetMsg());
            throw std::runtime_error("PhasePlot::ComputeData: Parser error");
        }
        catch (std::exception& e)
        {
            _log->AddExcept("PhasePlot::ComputeData: " + std::string(e.what()));
            throw(e);
        } 
        SetSpec("pulse_steps_remaining", pulse_steps_remaining);

        //A blowup will crash QwtPlot
        const double DMAX = std::numeric_limits<double>::max()/1e100;
        for (int i=0; i<num_diffs; ++i)
            if (abs(diffs[i])>DMAX)
                throw std::runtime_error("PhasePlot::ComputeData: model exploded");

        SetSpec("past_dv_samps_ct", _pastDVSampsCt);
        SetSpec("past_ip_samps_ct", _pastIPSampsCt);

        if (_makePlots)
            emit Flag2();

        emit ComputeComplete(num_steps);

        }label:
        lock.unlock();
        std::this_thread::sleep_for( std::chrono::milliseconds(RemainingSleepMs()) );
    }

    temp.close();
}

void PhasePlot::Initialize()
{
    std::lock_guard<std::recursive_mutex> lock(Mutex());
    const int num_diffs = (int)_modelMgr->Model(ds::DIFF)->NumPars(),
            num_vars = (int)_modelMgr->Model(ds::VAR)->NumPars();
    auto inner_product = std::deque<double>();
    auto diff_pts = DataVec(num_diffs);
    auto var_pts = DataVec(num_vars);
    SetData( new std::tuple<std::deque<double>,DataVec,DataVec>(
                inner_product, diff_pts, var_pts)
             );

    _makePlots = Spec_tob("make_plots");
    if (_makePlots)
    {
        QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
            QBrush( Qt::yellow ), QPen( Qt::red, 2 ), QSize( 8, 8 ) );
        _marker = new QwtPlotMarker();
        _marker->setSymbol(symbol);
        _marker->setZ(1);
        AddPlotItem(_marker);

        //The 'tail' of the plot
        _curve = new QwtPlotCurve();
        _curve->setPen( Qt::black, 1 );
        _curve->setRenderHint( QwtPlotItem::RenderAntialiased, true );
        AddPlotItem(_curve);
    }

    InitParserMgrs(1);
    DrawBase::Initialize();
}

void PhasePlot::MakePlotItems()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("PhasePlot::MakePlotItems", std::this_thread::get_id());
#endif
    std::lock_guard<std::recursive_mutex> lock(Mutex());
    auto data = static_cast< std::tuple<std::deque<double>,DataVec,DataVec>* >( Data() );
    auto diff_pts = std::get<1>(*data);
    ParserMgr& parser_mgr = GetParserMgr(0);
    const double* diffs = parser_mgr.ConstData(ds::DIFF);

    //Plot the current state vector
    const int xidx = Spec_toi("xidx"),
            yidx = Spec_toi("yidx");
    _marker->setValue(diffs[xidx], diffs[yidx]);

    const int num_saved_pts = (int)diff_pts[0].size();
    int tail_len = std::min( num_saved_pts, std::stoi(Spec("tail_length")) );
    if (tail_len==-1) tail_len = num_saved_pts;
    const int inc = tail_len < SamplesShown()/2
            ? 1
            : tail_len / (SamplesShown()/2);
    const int num_drawn_pts = tail_len / inc;
    QPolygonF points(num_drawn_pts);

    int ct_begin = std::max(0,num_saved_pts-tail_len);
    for (int k=0, ct=ct_begin; k<num_drawn_pts; ++k, ct+=inc)
        points[k] = QPointF(diff_pts.at(xidx).at(ct), diff_pts.at(yidx).at(ct));
    _curve->setSamples(points);

    auto xlims = std::minmax_element(diff_pts.at(xidx).cbegin(), diff_pts.at(xidx).cend()),
            ylims = std::minmax_element(diff_pts.at(yidx).cbegin(), diff_pts.at(yidx).cend());
    const double xmin = *xlims.first,
            xmax = *xlims.second,
            ymin = *ylims.first,
            ymax = *ylims.second;
    SetSpec("xmin", xmin);
    SetSpec("xmax", xmax);
    SetSpec("ymin", ymin);
    SetSpec("ymax", ymax);
}
