#include "eventviewer.h"
#include "ui_eventviewer.h"

#ifndef Q_OS_WIN
const int EventViewer::MAX_BINS;
#endif

EventViewer::EventViewer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EventViewer), _ct(0), _cv2(-1.0), _colors(ds::TraceColors())
{
    ui->setupUi(this);

    _evTimer.setSingleShot(true);
    ui->lblEvent->setAutoFillBackground(true);

    connect(&_evTimer, SIGNAL(timeout()), this, SLOT(RevertLabel()), Qt::QueuedConnection);
    connect(&_updateTimer, SIGNAL(timeout()), this, SLOT(UpdateCount()), Qt::QueuedConnection);
    ui->buttonGroup->setId(ui->rbtnAbove, ABOVE);
    ui->buttonGroup->setId(ui->rbtnBelow, BELOW);

    ui->qwtEventRate->setAxisTitle( QwtPlot::xBottom, "Time" );
    ui->qwtEventRate->setAxisTitle( QwtPlot::yLeft, "Thresh. crossings per unit time x1000" );

    _sdfCurve = new QwtPlotCurve;
    _sdfCurve->setPen( QPen(_colors.at(0)) );
    _sdfCurve->attach(ui->qwtEventRate);

    //Hide these until they're actually implemented
//    ui->qwtEventRate->hide();
//    ui->edRowLen->hide();
//    ui->lblRowLen->hide();
}

EventViewer::~EventViewer()
{
    delete ui;
}

void EventViewer::SetList(const VecStr& vs)
{
    ui->cmbDiffVars->clear();
    for (const auto& it : vs)
        ui->cmbDiffVars->addItem( it.c_str() );
}

void EventViewer::Start(int)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _updateTimer.start(1000);
    _currentTime = 0;
    ResetData();
    _binWidth = ui->edBinWidth->text().toInt();
    _rowLength = ui->edRowLen->text().toInt();
}
void EventViewer::Stop()
{
    _updateTimer.stop();
}

bool EventViewer::IsThreshAbove() const
{
    return ui->buttonGroup->checkedId() == ABOVE;
}
double EventViewer::Threshold() const
{
    return ui->edThresh->text().toDouble();
}
int EventViewer::VarIndex() const
{
    return ui->cmbDiffVars->currentIndex();
}

void EventViewer::closeEvent(QCloseEvent*)
{
    _updateTimer.stop();
}

void EventViewer::showEvent(QShowEvent*)
{
    _ct = 0;
}

void EventViewer::Event(int ev_time) //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("EventViewer::Event", std::this_thread::get_id());
#endif
    ++_ct;
    _evTimes.push_back(ev_time);

    ++_eventCounts[ ev_time % _rowLength ];
    _numRows = (ev_time - _startTime) / _rowLength + 1;

    ui->lblEvent->setPalette( QPalette( _colors.at(VarIndex()%_colors.size()) ) );
    _evTimer.stop();
    _evTimer.start(100);
}
void EventViewer::TimePoints(double point_ct) //slot
{
    std::lock_guard<std::mutex> lock(_mutex);
    _currentTime = point_ct;
}

void EventViewer::on_btnReset_clicked() //slot
{
    std::lock_guard<std::mutex> lock(_mutex);
    ResetAll();
}

void EventViewer::on_buttonGroup_buttonClicked(int id) //slot
{
    std::lock_guard<std::mutex> lock(_mutex);
    emit IsThreshAbove(id==ABOVE);
}

void EventViewer::on_cmbDiffVars_currentIndexChanged(int index) //slot
{
    std::lock_guard<std::mutex> lock(_mutex);
    emit ListSelection(index);
    ResetAll();
    _sdfCurve->setPen( QPen(_colors.at(index%_colors.size())) );
}

void EventViewer::on_edBinWidth_editingFinished() //slot
{
    std::lock_guard<std::mutex> lock(_mutex);
    _binWidth = std::min(_rowLength, std::max(1, ui->edBinWidth->text().toInt()));
}
void EventViewer::on_edRowLen_editingFinished() //slot
{
    std::lock_guard<std::mutex> lock(_mutex);
    _rowLength = ui->edRowLen->text().toInt();
    ResetAll();
}
void EventViewer::on_edThresh_editingFinished() //slot
{
    std::lock_guard<std::mutex> lock(_mutex);
    emit Threshold( ui->edThresh->text().toDouble() );
    ResetAll();
}

void EventViewer::RevertLabel() //slot
{
    ui->lblEvent->setPalette( QPalette(Qt::lightGray) );
}
void EventViewer::UpdateCount() //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("EventViewer::UpdateCount", std::this_thread::get_id());
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    if (_currentTime == _startTime) return;
    double evsec = 1000.0 * (double)_ct / (_currentTime - _startTime);
    ui->lblEventsPerSec->setText( std::to_string(evsec).c_str() );

    const int num_bins = _rowLength/_binWidth;
    QPolygonF sdf(num_bins);
    for (int i=0, ct=0; ct<num_bins; i+=_binWidth, ++ct)
    {
        double bct = 0;
        for (int k=0; k<_binWidth; ++k) bct += _eventCounts[i+k];
        bct /= _binWidth;
        sdf[ct] = QPointF( i, 1000.0 * bct / (double)_numRows );
    }
    _sdfCurve->setSamples(sdf);
    ui->qwtEventRate->replot();

    ui->lblNumRows->setText( ("# rows: " + std::to_string(_numRows)).c_str() );

    //Update CV2, definition taken from Luthman et al. 2011
    const int new_events = _evTimes.size(),
            prevN = _ct - new_events;
    if (new_events<3) return;
    std::vector<double> ISIs, cvs;
    for (int i=1; i<new_events; ++i)
        ISIs.push_back( (double)(_evTimes.at(i) - _evTimes.at(i-1)) );
    for (int i=1; i<ISIs.size(); ++i)
    {
        const double cv2_i = 2 * fabs(ISIs.at(i)-ISIs.at(i-1)) / (ISIs.at(i)+ISIs.at(i-1));
        cvs.push_back(cv2_i);
    }
    const double cvs_size = cvs.size();
    const double new_mean = std::accumulate(cvs.cbegin(), cvs.cend(), 0.0) / cvs_size;
    _cv2 = (prevN*_cv2 + cvs_size*new_mean) / (_ct-2);
    _evTimes.erase(_evTimes.begin(), _evTimes.begin()+cvs_size);

    ui->lblCV2->setText( std::to_string(_cv2).c_str() );
}

void EventViewer::ResetAll()
{
    ui->lblEventsPerSec->clear();
    ui->lblCV2->clear();
    ResetData();
}
void EventViewer::ResetData()
{
    _ct = 0;
    _cv2 = -1.0;
    _evTimes.clear();
    _startTime = _currentTime;
    memset(_eventCounts, 0, MAX_BINS);
    _numRows = 1;
}

