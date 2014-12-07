#include "timeplot.h"

TimePlot::TimePlot(DSPlot* plot) : DrawBase(plot)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("TimePlot::TimePlot", std::this_thread::get_id());
#endif
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

    SetSpec("dv_start", 0);
    SetSpec("dv_end", DrawBase::TP_WINDOW_LENGTH);
    SetSpec("y_tp_min", 0);
    SetSpec("y_tp_max", 0);
    SetOpaqueSpec("dv_data", nullptr);

    DrawBase::Initialize();
}

void TimePlot::ComputeData()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("TimePlot::ComputeData", std::this_thread::get_id());
#endif
    while (DrawState()==DRAWING)
    {
//        std::lock_guard<std::mutex> lock(Mutex());
        if (!OpaqueSpec("dv_data"))
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
//    std::lock_guard<std::mutex> lock(Mutex());
    void* dv_data = NonConstOpaqueSpec("dv_data");
    if (!dv_data) return;
    auto data_tuple = static_cast< std::tuple<std::deque<double>,DataVec,DataVec>* >(dv_data);
    const auto& inner_product = std::get<0>(*data_tuple);
    const auto& diff_pts = std::get<1>(*data_tuple),
            & var_pts = std::get<2>(*data_tuple);
    const int past_dv_samps_ct = Spec_toi("past_dv_samps_ct"),
            past_ip_samps_ct = Spec_toi("past_ip_samps_ct");

    //Get all of the information from the parameter fields, introducing new variables as needed.
    const int num_diffs = (int)_modelMgr->Model(ds::DIFF)->NumPars(),
            num_vars = (int)_modelMgr->Model(ds::VAR)->NumPars(),
            num_tp_points = std::min( (int)inner_product.size(), MAX_BUF_SIZE ),
            ip_start  = std::max(0, (int)inner_product.size()-num_tp_points),
            dv_start = std::max(0, (int)diff_pts.at(0).size()-num_tp_points),
            dv_end = dv_start + num_tp_points;
        //variables, differential equations, and initial conditions, all of which can invoke named
        //values

    TPVTableModel* tp_model = _modelMgr->TPVModel();
    const int num_all_tplots = 1 + num_diffs + num_vars,
            step = std::max(1, num_tp_points / SamplesShown()),
            num_plotted_pts = num_tp_points/step + (int)(num_tp_points%step != 0) - 1;
        // -1 for the offset step

    //This code is necessary to avoid sampling artifacts
    static int last_step = 1;
    if (last_step!=step)
        last_step = step;
    const int ip_step_off = step - (ip_start % step),
            dv_step_off = step - (dv_start % step);

    for (int i=0; i<num_all_tplots; ++i)
    {
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
            for (int k=ip_start+ip_step_off, ct=0; ct<num_plotted_pts; k+=step, ++ct)
                points_tp[ct] = QPointF((past_ip_samps_ct+k)*_modelMgr->ModelStep(), inner_product[k]*scale);
            curv->setSamples(points_tp);
            continue;
        }
        else if (i<=num_diffs) //A differential
        {
            int didx = i-1;
            QPolygonF points_tp(num_plotted_pts);
            for (int k=dv_start+dv_step_off, ct=0; ct<num_plotted_pts; k+=step, ++ct)
                points_tp[ct] = QPointF( (past_dv_samps_ct+k)*_modelMgr->ModelStep(), diff_pts.at(didx).at(k)*scale);
            curv->setSamples(points_tp);
            continue;
        }
        else //A variable
        {
            int vidx = i-num_diffs-1;
            QPolygonF points_tp(num_plotted_pts);
            for (int k=dv_start+dv_step_off, ct=0; ct<num_plotted_pts; k+=step, ++ct)
                points_tp[ct] = QPointF( (past_dv_samps_ct+k)*_modelMgr->ModelStep(), var_pts.at(vidx).at(k)*scale);
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
    delete data_tuple;
}
