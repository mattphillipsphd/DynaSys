#include "timeplot.h"

TimePlot::TimePlot(DSPlot* plot) : DrawBase(plot), _lastPt(0)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("TimePlot::TimePlot", std::this_thread::get_id());
#endif
}

void TimePlot::SetNonConstOpaqueSpec(const std::string &key, void *value)
{
    if (key=="dv_data")
    {
        std::lock_guard<std::mutex> lock( Mutex() );
        std::deque<Packet*>* packets = static_cast< std::deque<Packet*>* >(value);
        if (packets)
        {
            for (size_t i=0; i<packets->size(); ++i)
                _packets.push_back( packets->at(i) );
            delete packets;
        }
    }
    DrawBase::SetNonConstOpaqueSpec(key, value);
}

void TimePlot::Initialize()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("TimePlot::Initialize", std::this_thread::get_id());
#endif
    _colors = *static_cast< const std::vector<QColor>* >( OpaqueSpec("colors") );
    const int num_diffs = (int)_modelMgr->Model(ds::DIFF)->NumPars(),
            num_vars = (int)_modelMgr->Model(ds::VAR)->NumPars();
    const int num_all_tplots = 1 + num_diffs + num_vars,
            num_colors = (int)_colors.size();
        // +1 for inner product.  The strategy is to attach all possible curves
        //but only the enabled ones have non-empty samples.
    ClearPlotItems();
    for (int i=0; i<num_all_tplots; ++i)
    {
        QwtPlotCurve* curv = new QwtPlotCurve();
        curv->setPen( _colors.at(i%num_colors), 1 );
        curv->setRenderHint( QwtPlotItem::RenderAntialiased, true );
        AddPlotItem(curv);
    }

    while (!_packets.empty())
    {
        delete _packets.front();
        _packets.pop_front();
    }
    _ip.clear();
    _diffPts = DataVec(num_diffs);
    _varPts = DataVec(num_vars);
    _lastPt = 0;

    SetSpec("past_samps_ct", 0);
    SetSpec("dv_start", 0);
    SetSpec("dv_end", DrawBase::TP_WINDOW_LENGTH);
    SetSpec("y_tp_min", 0);
    SetSpec("y_tp_max", 0);
    SetSpec("event_index", -1);
    SetSpec("num_samples", 0);
    SetNonConstOpaqueSpec("dv_data", nullptr);

    DrawBase::Initialize();
}

void TimePlot::ComputeData()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("TimePlot::ComputeData", std::this_thread::get_id());
#endif
    while (DrawState()==DRAWING)
    {
        if (!NonConstOpaqueSpec("dv_data"))
            goto label;{

        emit ComputeComplete(1);

        }label:
        std::this_thread::sleep_for( std::chrono::milliseconds(RemainingSleepMs()) );
    }
}

void TimePlot::MakePlotItems()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("TimePlot::MakePlotItems", std::this_thread::get_id());
#endif
//    QTime timer;
//    timer.start();

    //Get the available packets
    std::unique_lock<std::mutex> lock( Mutex() );
    if (_packets.empty()) return;
    while (!_packets.empty())
    {
        const Packet* packet = _packets.front();
        const size_t num_diffs = packet->diffs.size(),
                num_vars = packet->vars.size(),
                num_samples = packet->num_samples;
        for (size_t k=0; k<num_samples; ++k)
        {
            for (size_t i=0; i<num_diffs; ++i)
                _diffPts[i].push_back( packet->diffs.at(i)[k]);
            for (size_t i=0; i<num_vars; ++i)
                _varPts[i].push_back( packet->vars.at(i)[k]);
            _ip.push_back( packet->ip[k] );
        }
        _packets.pop_front();
    }
    lock.unlock();

    //Shrink the buffers if need be, and record overshoot
    const int num_diffs = (int)_modelMgr->Model(ds::DIFF)->NumPars(),
            num_vars = (int)_modelMgr->Model(ds::VAR)->NumPars();
    int past_samps_ct = Spec_toi("past_samps_ct");
    const int max_size = std::min(MAX_BUF_SIZE, Spec_toi("num_samples"));
    const int overflow = (int)_diffPts.at(0).size() - max_size;
    if (overflow>0)
    {
        for (int i=0; i<num_diffs; ++i)
            _diffPts[i].erase(_diffPts[i].begin(), _diffPts[i].begin()+overflow);
        for (int i=0; i<num_vars; ++i)
            _varPts[i].erase(_varPts[i].begin(), _varPts[i].begin()+overflow);
        _ip.erase(_ip.begin(), _ip.begin()+overflow);
        past_samps_ct += overflow;
    }
    SetSpec("past_samps_ct", past_samps_ct);

    //Get all of the information from the parameter fields, introducing new variables as needed.
    const int num_tp_points = (int)_ip.size(),
            dv_start = std::max(0, (int)_diffPts.at(0).size()-num_tp_points),
            dv_end = dv_start + num_tp_points;
        //variables, differential equations, and initial conditions, all of which can invoke named
        //values
    const int time_offset = Spec_toi("time_offset");//,
//            event_index = Spec_toi("event_index");
//    const double event_threshold = Spec_tod("event_thresh");
//    const bool thresh_above = Spec_tob("thresh_above");

    TPVTableModel* tp_model = _modelMgr->TPVModel();
    const int num_all_tplots = 1 + num_diffs + num_vars,
            step = std::max(1, num_tp_points / SamplesShown()),
            num_plotted_pts = num_tp_points/step + (int)(num_tp_points%step != 0) - 1;
        // -1 for the offset step

    //This code is necessary to avoid sampling artifacts
    static int last_step = 1;
    if (last_step!=step)
        last_step = step;
    const int dv_step_off = step - (dv_start % step);
    const double model_step = _modelMgr->ModelStep();
//    std::cerr << step << ", " << num_plotted_pts << ", " << past_samps_ct << ", " << dv_start
//              << ", " << dv_step_off << ", " << dv_end << std::endl;
    for (int i=0; i<num_all_tplots; ++i)
    {
/*        if (i==event_index)
        {
            for (int k=dv_start+dv_step_off, ct=0; ct<num_plotted_pts; k+=step, ++ct)
            {
                double val;
                if (i==0) val = _ip.at(k);
                else if (i<=num_diffs) val = _diffPts.at(i-1).at(k);
                else val = _varPts.at(i-num_diffs-1).at(k);
                if (((thresh_above && val>=event_threshold) || (!thresh_above && val<event_threshold))
                        && ((thresh_above && _lastPt<event_threshold) || (!thresh_above && _lastPt>=event_threshold)))
                    emit Flag_i(k);
                _lastPt = val;
            }
        }
*/
        QwtPlotCurve* curv = static_cast<QwtPlotCurve*>( PlotItem(i) );
        if (!tp_model->IsEnabled(i))
        {
            curv->setSamples(QPolygonF());
            continue;
        }

        const double scale = std::pow(10.0, tp_model->LogScale(i));
            // ### Use MSL for fast multiplication!

        if (i==0) //IP
        {
            QPolygonF points_tp(num_plotted_pts);
            for (int k=dv_start+dv_step_off, ct=0; ct<num_plotted_pts; k+=step, ++ct)
                points_tp[ct] = QPointF((past_samps_ct+k)*model_step+time_offset, _ip.at(k)*scale);
            curv->setSamples(points_tp);
        }
        else if (i<=num_diffs) //A differential
        {
            const int didx = i-1;
            QPolygonF points_tp(num_plotted_pts);
            for (int k=dv_start+dv_step_off, ct=0; ct<num_plotted_pts; k+=step, ++ct)
                points_tp[ct] = QPointF( (past_samps_ct+k)*model_step+time_offset, _diffPts.at(didx).at(k)*scale);
            curv->setSamples(points_tp);
        }
        else //A variable
        {
            const int vidx = i-num_diffs-1;
            QPolygonF points_tp(num_plotted_pts);
            for (int k=dv_start+dv_step_off, ct=0; ct<num_plotted_pts; k+=step, ++ct)
                points_tp[ct] = QPointF( (past_samps_ct+k)*model_step+time_offset, _varPts.at(vidx).at(k)*scale);
            curv->setSamples(points_tp);
        }
    }

    //Get axis limits
    double y_tp_min(std::numeric_limits<double>::max()),
            y_tp_max(-std::numeric_limits<double>::max());
    for (int i=0; i<num_all_tplots; ++i)
        if (tp_model->IsEnabled(i))
        {
            const QwtPlotCurve* curv = static_cast<QwtPlotCurve*>( PlotItem(i) );
            if (curv->maxYValue() > y_tp_max) y_tp_max = curv->maxYValue();
            if (curv->minYValue() < y_tp_min) y_tp_min = curv->minYValue();
        }

    SetSpec("dv_start", dv_start);
    SetSpec("dv_end", dv_end);
    SetSpec("y_tp_min", y_tp_min);
    SetSpec("y_tp_max", y_tp_max);

    SetNonConstOpaqueSpec("dv_data", nullptr);
//    std::cerr << "Timeplot::MakeItems: " << std::to_string( timer.elapsed() );
}
