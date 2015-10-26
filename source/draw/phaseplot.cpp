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
    for (auto it : _packets) delete it;
}

/*void* PhasePlot::DataCopy() const
{
    std::lock_guard<std::mutex> lock(Mutex());
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
}*/

void* PhasePlot::DataCopy() const
{
    std::lock_guard<std::mutex> lock(Mutex());
    std::deque<Packet*>* packets = new std::deque<Packet*>();
    for (auto it : _packets)
    {
        if (!( it->read_flag & Packet::TP_READ))
                packets->push_back( new Packet(*it) );
        it->read_flag |= Packet::TP_READ;
    }
    return packets;
}

int PhasePlot::SleepMs() const
{
    return _makePlots ? 50 : 0;
}

void PhasePlot::ClearData()
{
    auto data = static_cast< std::tuple<std::deque<double>,DataVec,DataVec>* >( Data() );
    if (!data) return;
    delete data;
}

void PhasePlot::ComputeData()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("PhasePlot::ComputeData", std::this_thread::get_id());
#endif
    //Get all of the information from the parameter fields, introducing new variables as needed.
    ParserMgr& parser_mgr = GetParserMgr(0);
    const int num_diffs = (int)_modelMgr->Model(ds::DIFF)->NumPars(),
            num_vars = (int)_modelMgr->Model(ds::VAR)->NumPars();
    const double* const diffs = parser_mgr.ConstData(ds::DIFF),
            * const vars = parser_mgr.ConstData(ds::VAR);
        //variables, differential equations, and initial conditions, all of which can invoke named
        //values

    const bool is_recording = Spec_tob("is_recording");

    QFile temp(ds::TEMP_FILE.c_str());
    std::string output;
    if (is_recording)
    {
        temp.open(QFile::WriteOnly | QFile::Text);
        for (size_t i=0; i<(size_t)num_diffs; ++i)
            output += _modelMgr->Model(ds::DIFF)->ShortKey(i) + "\t";
        for (size_t i=0; i<(size_t)num_vars; ++i)
            output += _modelMgr->Model(ds::VAR)->ShortKey(i) + "\t";
        output += "\n";
        temp.write(output.c_str());
        temp.flush();
    }

    while (DrawState()==DRAWING)
    {
        if (!NeedNewStep())
            goto label;{

        //Get values which may be updated from main thread
        int pulse_steps_remaining = Spec_toi("pulse_steps_remaining");

        //Update the state vector with the value of the differentials.
            //Number of iterations to calculate in this refresh
        int steps_per_sec = Spec_toi("steps_per_sec");
        int num_steps = _makePlots
                ? ((double)steps_per_sec/_modelMgr->ModelStep()) / (double)SleepMs() + 0.5
                : 100;
        if (num_steps==0) num_steps = 1;
        if (num_steps>MAX_BUF_SIZE) num_steps = MAX_BUF_SIZE;
        Packet* packet = new Packet(num_steps, num_diffs, num_vars);
        double* pack_ip = packet->ip;
        const std::vector<double*>& pack_diffs = packet->diffs,
                pack_vars = packet->vars;

        //Go through each expression and evaluate them
        try
        {
            RecomputeIfNeeded();
            if (is_recording) output.clear();
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
                    pack_diffs[i][k] = diffs_i;
                    ip_k += diffs_i * diffs_i;
                }
                pack_ip[k] = ip_k;
                for (int i=0; i<num_vars; ++i)
                    pack_vars[i][k] = vars[i];

                if (is_recording)
                {
                    for (int i=0; i<num_diffs; ++i)
                        output += std::to_string(diffs[i]) + "\t";
                    for (int i=0; i<num_vars; ++i)
                        output += std::to_string(vars[i]) + "\t";
                    output += "\n";
                    //Saving *every* sample is incredibly unwieldy--need a parameter to control
                    //when to save values
                }
            }

            if (is_recording)
            {
                temp.write(output.c_str());
                temp.flush();
            }

            std::lock_guard<std::mutex> lock( Mutex() );
            _packets.push_back(packet);
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
            if (fabs(diffs[i])>DMAX)
                throw std::runtime_error("PhasePlot::ComputeData: model exploded");

        if (_makePlots)
            emit Flag2();

        emit ComputeComplete(num_steps);

        }label:
        std::this_thread::sleep_for( std::chrono::milliseconds(RemainingSleepMs()) );
    }

    temp.close();
}

void PhasePlot::Initialize()
{
    const int num_diffs = (int)_modelMgr->Model(ds::DIFF)->NumPars();
    while (!_packets.empty())
    {
        delete _packets.front();
        _packets.pop_front();
    }

    _makePlots = Spec_tob("make_plots");
    if (_makePlots)
    {
        _diffPts = DataVec(num_diffs);

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
        _curve->setZ(0.25);
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
//    QTime timer;
//    timer.start();

    //**********************Copy packet data into local variables
    std::unique_lock<std::mutex> lock( Mutex() );
    if (_packets.empty()) return;
    for (auto it : _packets)
    {
        if (!(it->read_flag & Packet::PP_READ))
        {
            const size_t num_diffs = it->diffs.size(),
                    num_samples = it->num_samples;
            for (size_t i=0; i<num_diffs; ++i)
                for (size_t k=0; k<num_samples; ++k)
                    _diffPts[i].push_back( it->diffs.at(i)[k]);
            it->read_flag |= Packet::PP_READ;
        }
    }

        //Delete used packets
    Packet* packet = _packets.front();
    while ((packet->read_flag & Packet::TP_READ) && (packet->read_flag & Packet::PP_READ))
    {
        _packets.pop_front();
        if (_packets.empty()) break;
        packet = _packets.front();
    }
    lock.unlock();
    //**********************

    //Shrink the buffer if need be
    const int num_diffs = _diffPts.size(),
            xy_buf_over = (int)_diffPts.at(0).size() - MAX_BUF_SIZE;
    if (xy_buf_over>0)
        for (int i=0; i<num_diffs; ++i)
            _diffPts[i].erase(_diffPts[i].begin(), _diffPts[i].begin()+xy_buf_over);

    //Plot the current state vector
    const int xidx = Spec_toi("xidx"),
            yidx = Spec_toi("yidx");
    const std::deque<double>& diff_x = _diffPts.at(xidx),
            & diff_y = _diffPts.at(yidx);
    if (diff_x.empty()) return;
    _marker->setValue(diff_x.back(), diff_y.back());

    //Plot the history (the curve)
    const int num_saved_pts = (int)diff_x.size();
    int tail_len = std::min( num_saved_pts, Spec_toi("tail_length") );
    if (tail_len==-1) tail_len = num_saved_pts;
    const int inc = tail_len < SamplesShown()/2
            ? 1
            : tail_len / (SamplesShown()/2);
    const int num_drawn_pts = tail_len / inc;
    QPolygonF points(num_drawn_pts);

    int ct_begin = std::max(0,num_saved_pts-tail_len);
    for (int k=0, ct=ct_begin; k<num_drawn_pts; ++k, ct+=inc)
        points[k] = QPointF(diff_x[ct], diff_y[ct]);
    _curve->setSamples(points);

    auto xlims = std::minmax_element(diff_x.cbegin(), diff_x.cend()),
            ylims = std::minmax_element(diff_y.cbegin(), diff_y.cend());
    const double xmin = *xlims.first,
            xmax = *xlims.second,
            ymin = *ylims.first,
            ymax = *ylims.second;
    SetSpec("xmin", xmin);
    SetSpec("xmax", xmax);
    SetSpec("ymin", ymin);
    SetSpec("ymax", ymax);

//    std::cerr << "PhasePlot::MakeItems: " << std::to_string( timer.elapsed() );
}

int PhasePlot::PacketSampCt() const
{
    int sum = 0;
    for (const auto& it : _packets) sum += it->num_samples;
    return sum;
}
