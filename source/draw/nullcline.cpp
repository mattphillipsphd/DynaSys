#include "nullcline.h"

Nullcline::Nullcline(DSPlot* plot) : DrawBase(plot)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Nullcline::Nullcline", std::this_thread::get_id());
#endif
}
Nullcline::~Nullcline()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Nullcline::~Nullcline", std::this_thread::get_id());
#endif
    if (Data()) delete static_cast<Record*>( Data() );
}

void* Nullcline::DataCopy() const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Nullcline::DataCopy", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock( Mutex() );
    if (!ConstData()) return nullptr;
    return new Record(*static_cast<const Record*>( ConstData() ));
}

void Nullcline::ComputeData()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Nullcline::ComputeData", std::this_thread::get_id());
#endif
    while (DrawState()==DRAWING)
    {
        if (!NeedNewStep() && !NeedRecompute())
            goto label;{

        const int xidx = Spec_toi("xidx"),
                yidx = Spec_toi("yidx"),
                resolution = Spec_toi("resolution")*2,
                resolution2 = resolution*resolution;
        InitParserMgrs(resolution2);

        const double xmin = _modelMgr->Minimum(ds::INIT, xidx),
                xmax = _modelMgr->Maximum(ds::INIT, xidx),
                ymin = _modelMgr->Minimum(ds::INIT, yidx),
                ymax = _modelMgr->Maximum(ds::INIT, yidx);
        const double xinc = (xmax - xmin) / (double)(resolution-1),
                yinc = (ymax - ymin) / (double)(resolution-1);

        Record* record = new Record(resolution2);
        double* x = record->x,
                * y = record->y,
                * xdiff = new double[resolution2],
                * ydiff = new double[resolution2];

        try
        {
            std::lock_guard<std::mutex> lock( Mutex() );
            RecomputeIfNeeded();
            for (int i=0; i<resolution; ++i)
                for (int j=0; j<resolution; ++j)
                {
                    const double xij = i*xinc + xmin,
                                yij = j*yinc + ymin;
                    const int idx = i*resolution+j;

                    ParserMgr& parser_mgr = GetParserMgr(idx);
                    const double* diffs = parser_mgr.ConstData(ds::DIFF);

                    parser_mgr.SetData(ds::DIFF, xidx, xij);
                    parser_mgr.SetData(ds::DIFF, yidx, yij);
                    parser_mgr.ParserEval(false);
                    xdiff[idx] = diffs[xidx] - xij;
                    ydiff[idx] = diffs[yidx] - yij;

                    x[idx] = xij;
                    y[idx] = yij;
                }

            double lastx, lasty;
            std::vector< std::pair<int,int> >& xcross_h = record->xcross_h,
                    & xcross_v = record->xcross_v,
                    & ycross_h = record->ycross_h,
                    & ycross_v = record->ycross_v;
            std::unordered_set<int> xidx_set, yidx_set; //To avoid duplicates;
            //The grid is being searched *column-wise*
            for (int j=0; j<resolution; ++j)
            {
                lastx = xdiff[j];
                lasty = ydiff[j];
                for (int i=0; i<resolution; ++i)
                {
                    const int idx = i*resolution+j;
                    if (ds::sgn(lastx) != ds::sgn(xdiff[idx]))
                    {
                        xcross_v.push_back( std::make_pair(i,j) );
                        xidx_set.insert(idx);
                    }
                    lastx = xdiff[idx];

                    if (ds::sgn(lasty) != ds::sgn(ydiff[idx]))
                    {
                        ycross_v.push_back( std::make_pair(i,j) );
                        yidx_set.insert(idx);
                    }
                    lasty = ydiff[idx];
                }
            }

            //Switch indices so as to search row-wise
            for (int i=0; i<resolution; ++i)
            {
                lastx = xdiff[i*resolution];
                lasty = ydiff[i*resolution];
                for (int j=0; j<resolution; ++j)
                {
                    const int idx = i*resolution+j;
                    if (ds::sgn(lastx) != ds::sgn(xdiff[idx]) && xidx_set.count(idx)==0)
                        xcross_h.push_back( std::make_pair(i,j) );
                    lastx = xdiff[idx];

                    if (ds::sgn(lasty) != ds::sgn(ydiff[idx]) && yidx_set.count(idx)==0)
                        ycross_h.push_back( std::make_pair(i,j) );
                    lasty = ydiff[idx];
                }
            }

            _packets.push_back(record);
        }
        catch (std::exception& e)
        {
            _log->AddExcept("Nullcline::ComputeData: " + std::string(e.what()));
            throw (e);
        }

        delete[] xdiff;
        delete[] ydiff;

        emit ComputeComplete(1);

        }label:
        std::this_thread::sleep_for( std::chrono::milliseconds(RemainingSleepMs()) );
    }

    if (DeleteOnFinish()) emit ReadyToDelete();
}

void Nullcline::Initialize()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Nullcline::Initialize", std::this_thread::get_id());
#endif
    _colors = *static_cast< const std::vector<QColor>* >( OpaqueSpec("colors") );
    SetNeedRecompute(true);
    FreezeNonUser();
    const int resolution = Spec_toi("resolution")*2,
            resolution2 = resolution*resolution;
    InitParserMgrs(resolution2);
}

void Nullcline::MakePlotItems()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Nullcline::MakePlotItems", std::this_thread::get_id());
#endif

    std::unique_lock<std::mutex> lock( Mutex() );
    if (_packets.empty()) return;
    while (_packets.size()>1)
    {
        delete[] _packets.front();
        _packets.pop_front();
    }
    Record* record = _packets.front();
    _packets.pop_front();
    if (!record) return;
    lock.unlock();

    ClearPlotItems();

    const size_t xidx = Spec_toi("xidx"),
            yidx = Spec_toi("yidx"),
            resolution2 = NumParserMgrs(),
            resolution = (int)sqrt(resolution2);

    const std::vector< std::pair<int,int> >& xcross_h = record->xcross_h,
            & xcross_v = record->xcross_v,
            & ycross_h = record->ycross_h,
            & ycross_v = record->ycross_v;
    const double* x = record->x,
            * y = record->y;
    const double xinc = record->xinc,
            yinc = record->yinc;

    const size_t xnum_pts_h = xcross_h.size(),
            xnum_pts_v = xcross_v.size(),
            ynum_pts_h = ycross_h.size(),
            ynum_pts_v = ycross_v.size();

    ReservePlotItems(xnum_pts_h + xnum_pts_v + ynum_pts_h + ynum_pts_v);

    const QColor& xcolor = _colors.at(xidx+1);
    for (size_t i=0; i<xnum_pts_h; ++i)
    {
        QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
            QBrush(xcolor), QPen(xcolor, 2), QSize(5, 5) );
        QwtPlotMarker* marker = new QwtPlotMarker();
        marker->setSymbol(symbol);
        const int idx = xcross_h.at(i).first*resolution + xcross_h.at(i).second;
        marker->setXValue(x[idx] + xinc/2.0);
        marker->setYValue(y[idx]);
        marker->setZ(-1);
        AddPlotItem(marker);
    }
    for (size_t i=0; i<xnum_pts_v; ++i)
    {
        QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
            QBrush(xcolor), QPen(xcolor, 2), QSize(5, 5) );
        QwtPlotMarker* marker = new QwtPlotMarker();
        marker->setSymbol(symbol);
        const int idx = xcross_v.at(i).first*resolution + xcross_v.at(i).second;
        marker->setXValue(x[idx]);
        marker->setYValue(y[idx] + yinc/2.0);
        marker->setZ(-1);
        AddPlotItem(marker);
    }

    QColor ycolor = _colors.at(yidx+1);
    for (size_t i=0; i<ynum_pts_h; ++i)
    {
        QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
            QBrush(ycolor), QPen(ycolor, 2), QSize(5, 5) );
        QwtPlotMarker* marker = new QwtPlotMarker();
        marker->setSymbol(symbol);
        const int idx = ycross_h.at(i).first*resolution + ycross_h.at(i).second;
        marker->setXValue(x[idx] + xinc/2.0);
        marker->setYValue(y[idx]);
        marker->setZ(-1);
        AddPlotItem(marker);
    }
    for (size_t i=0; i<ynum_pts_v; ++i)
    {
        QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
            QBrush(ycolor), QPen(ycolor, 2), QSize(5, 5) );
        QwtPlotMarker* marker = new QwtPlotMarker();
        marker->setSymbol(symbol);
        const int idx = ycross_v.at(i).first*resolution + ycross_v.at(i).second;
        marker->setXValue(x[idx]);
        marker->setYValue(y[idx] + yinc/2.0);
        marker->setZ(-1);
        AddPlotItem(marker);
    }

    DrawBase::Initialize(); //Have to wait on this, since we don't know how many objects there
        //are until the analysis is complete

    delete record;
}
