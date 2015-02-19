#include "eventviewer.h"
#include "ui_eventviewer.h"

EventViewer::EventViewer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EventViewer), _ct(0)
{
    ui->setupUi(this);
    connect(&_timer, SIGNAL(timeout()), this, SLOT(UpdateCount()));
    ui->buttonGroup->setId(ui->rbtnAbove, ABOVE);
    ui->buttonGroup->setId(ui->rbtnBelow, BELOW);
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

void EventViewer::Start(int start_time)
{
    _timer.start(1000);
    _currentTime = _startTime = start_time;
}

void EventViewer::closeEvent(QCloseEvent*)
{
    _timer.stop();
}

void EventViewer::showEvent(QShowEvent*)
{
    _ct = 0;
}

void EventViewer::Event(int ev_time) //slot
{
    ++_ct;
    _currentTime = ev_time;
}

void EventViewer::on_btnReset_clicked() //slot
{
    _currentTime = _startTime = _ct = 0;
    ui->lblEventsPerSec->clear();
}

void EventViewer::on_buttonGroup_buttonClicked(int id)
{
    emit IsThreshAbove(id==ABOVE);
}

void EventViewer::on_cmbDiffVars_currentIndexChanged(int index)
{
    emit ListSelection(index);
}

void EventViewer::on_edThresh_editingFinished()
{
    emit Threshold( ui->edThresh->text().toDouble() );
}

void EventViewer::UpdateCount() //slot
{
    if (_currentTime == _startTime) return;
    double evsec = 1000.0 * (double)_ct / (double)(_currentTime - _startTime);
    ui->lblEventsPerSec->setText( std::to_string(evsec).c_str() );
}
