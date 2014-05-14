#include "mainwindow.h"
#include "ui_mainwindow.h"

const int MainWindow::DEFAULT_SINGLE_STEP = 10;
const int MainWindow::DEFAULT_SINGLE_TAIL = -1;
const int MainWindow::DEFAULT_VF_STEP = 1;
const int MainWindow::DEFAULT_VF_TAIL = 1;
const int MainWindow::MAX_BUF_SIZE = 8 * 1024 * 1024;
const double MainWindow::MIN_MODEL_STEP = 1e-7;
const int MainWindow::SLEEP_MS = 50;
const int MainWindow::SLIDER_INT_LIM = 10000;
const int MainWindow::TP_SAMPLES_SHOWN = 2 * 1024;
const int MainWindow::TP_WINDOW_LENGTH = 1000;
const int MainWindow::XY_SAMPLES_SHOWN = 64 * 1024;
const int MainWindow::VF_RESOLUTION = 20;
const int MainWindow::VF_SLEEP_MS = 250;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _aboutGui(new AboutGui()), _logGui(new LogGui()), _notesGui(new NotesGui()), _paramEditor(new ParamEditor()),
    _conditions(nullptr), _differentials(nullptr), _initConds(nullptr),
    _parameters(nullptr), _variables(nullptr),
    _fileName(""), _isVFAttached(false), _log(Log::Instance()),
    _needClearVF(false), _needInitialize(true), _needUpdateExprns(false),
    _needUpdateNullclines(false), _needUpdateVF(false), _numTPSamples(TP_WINDOW_LENGTH),
    _playState(STOPPED), _plotMode(SINGLE), _pulseResetValue("-666"), _pulseStepsRemaining(-1),
    _singleStepsSec(DEFAULT_SINGLE_STEP), _singleTailLen(DEFAULT_SINGLE_TAIL),
    _tid(std::this_thread::get_id()),
    _tpColors(InitTPColors()),
    _vfStepsSec(DEFAULT_VF_STEP), _vfTailLen(DEFAULT_VF_TAIL)
{
#ifdef Q_OS_WIN
    ds::InitThreadColors();
#endif

    ds::AddThread(_tid);
#ifdef DEBUG_FUNC
    ScopeTracker::InitThread(std::this_thread::get_id());
    std::stringstream s; s << _tid;
    qDebug() << "Enter MainWindow::MainWindow, thread id:" << s.str().c_str();
    ScopeTracker st("MainWindow::MainWindow", _tid);
#endif

    qRegisterMetaType<ViewRect>("ViewRect");

    ui->setupUi(this);

    QStringList modes;
    modes << "Single" << "Vector field";
    ui->cmbPlotMode->setModel( new QStringListModel(modes) );

    ui->edModelStep->setText( QString("%1").arg(ds::DEFAULT_MODEL_STEP) );
    ui->edNumTPSamples->setText( std::to_string(TP_WINDOW_LENGTH).c_str() );
    ui->tblTimePlot->horizontalHeader()->setStretchLastSection(true);
    ui->sldParameter->setRange(0, SLIDER_INT_LIM);
    ui->spnVFResolution->setValue(VF_RESOLUTION);
    ui->spnStepsPerSec->setValue(_singleStepsSec);
    ui->spnTailLength->setValue(_singleTailLen);
    ui->spnTailLength->setMaximum(XY_SAMPLES_SHOWN);

    ui->qwtPhasePlot->setAutoDelete(false);
    ui->qwtTimePlot->setAutoDelete(false);

    setWindowTitle("DynaSys " + QString(ds::VERSION_STR.c_str()) + " - default model");
    setWindowIcon( QIcon("Logo.ico") );
    //Use converticon.com to quickly create icon file

    InitDefaultModel();

    ResetPhasePlotAxes();
    UpdateTimePlotTable();

    ConnectModels();

    connect(this, SIGNAL(DoAttachVF(bool)), this, SLOT(AttachVectorField(bool)), Qt::BlockingQueuedConnection);
    connect(this, SIGNAL(DoInitParserMgr()), this, SLOT(InitParserMgr()), Qt::BlockingQueuedConnection);
    connect(this, SIGNAL(DoReplot(const ViewRect&, const ViewRect&)),
            this, SLOT(Replot(const ViewRect&, const ViewRect&)), Qt::BlockingQueuedConnection);
    connect(this, SIGNAL(DoUpdateParams()), this, SLOT(UpdateParams()), Qt::BlockingQueuedConnection);

    _aboutGui->setWindowModality(Qt::ApplicationModal);

    connect(_log, SIGNAL(OpenGui()), this, SLOT(Error()));

    connect(_logGui, SIGNAL(ShowParser()), this, SLOT(ParserToLog()));

    connect(ui->qwtPhasePlot, SIGNAL(MousePos(QPointF)), this, SLOT(UpdateMousePos(QPointF)));
    connect(ui->qwtPhasePlot, SIGNAL(MouseClick()), this, SLOT(Pause()));
    connect(ui->qwtTimePlot, SIGNAL(MouseClick()), this, SLOT(Pause()));
    connect(_notesGui, SIGNAL(SaveNotes()), this, SLOT(SaveNotes()));

    connect(_paramEditor, SIGNAL(CloseEditor()), this, SLOT(ParamEditorClosed()));
    connect(_paramEditor, SIGNAL(ModelChanged(void*)), this, SLOT(LoadTempModel(void*)));
}
MainWindow::~MainWindow()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::~MainWindow", _tid);
#endif
    delete ui;
    QFile temp_file(ds::TEMP_FILE.c_str());
    if (temp_file.exists()) temp_file.remove();
}
void MainWindow::Error()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::Error", std::this_thread::get_id());
#endif
    _playState = STOPPED;
    ui->btnStart->setText("Start");
    _logGui->show();
}
void MainWindow::LoadTempModel(void* models) //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::LoadTempModel", _tid);
#endif
    try
    {
        SysFileOut out(ds::TEMP_MODEL_FILE);
        out.Save(*(static_cast<VecStr*>(models)), _parserMgr.ModelStep(), _notesGui->GetNotes());

        LoadModel(ds::TEMP_MODEL_FILE);
        SaveModel(_fileName);
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::LoadTempModel: " + std::string(e.what()));
    }
}
void MainWindow::ParamEditorClosed() //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ParamEditorClosed", _tid);
#endif
    ui->btnStart->setEnabled(true);
    SetButtonsEnabled(true);
    SetParamsEnabled(true);
}
void MainWindow::ParserToLog() //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ParserToLog", _tid);
#endif
    std::string parser = _parserMgr.ParserContents();
    std::replace(parser.begin(), parser.end(), ',', '\n');
    _log->AddMesg(parser);
}
void MainWindow::Pause() //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::Pause", _tid);
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    switch (_playState)
    {
        case STOPPED:
            break;
        case DRAWING:
            _playState = PAUSED;
            ui->btnStart->setText("Resume");
            break;
        case PAUSED:
            _playState = DRAWING;
            ui->btnStart->setText("Stop");
            ResumeDraw();
            break;
    }
}
void MainWindow::SaveNotes() //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::SaveNotes", _tid);
#endif
    SysFileIn in(_fileName);
    std::vector<ParamModelBase*> models;
    ConditionModel* conditions = new ConditionModel(this);
    std::string model_step;
    in.Load(models, model_step, conditions, nullptr);

    SysFileOut out(_fileName);
    std::vector<const ParamModelBase*> cmodels;
    for (auto it : models)
        cmodels.push_back(it);
    const Notes* notes = _notesGui->GetNotes();
    out.Save(cmodels, std::atof(model_step.c_str()),
             conditions, notes);
}
void MainWindow::UpdateMousePos(QPointF pos) //slot
{
//#ifdef DEBUG_FUNC
//    ScopeTracker st("MainWindow::UpdateMousePos", _tid);
//#endif
    ui->lblMouseX->setText( std::to_string(pos.x()).c_str() );
    ui->lblMouseY->setText( std::to_string(pos.y()).c_str() );
}
void MainWindow::UpdateTimePlot() //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateTimePlot", _tid);
#endif
    if (_playState!=DRAWING)
        DrawTimePlot(true);
}

void MainWindow::closeEvent(QCloseEvent *)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::closeEvent", _tid);
#endif
    _aboutGui->close();
    _logGui->close();
    _notesGui->close();
    _paramEditor->close();
}

void MainWindow::on_actionAbout_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionAbout_triggered", _tid);
#endif
    _aboutGui->show();
}

void MainWindow::on_actionClear_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionClear_triggered", _tid);
#endif
    if (_playState!=STOPPED)
        throw std::runtime_error("MainWindow::on_actionClear_triggered, Bad state");
    _needUpdateVF = false;
    InitModels();
    UpdateLists();
}

void MainWindow::on_actionLoad_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionLoad_triggered", _tid);
#endif
    std::string file_name = QFileDialog::getOpenFileName(nullptr,
                                                             "Load dynamical system",
#ifdef QT_DEBUG
                                                             "../DynaSysFiles").toStdString();
#else
                                                             "../../DynaSysFiles").toStdString();
#endif
    if (file_name.empty()) return;
    _fileName = file_name;
    _needInitialize = true;
    LoadModel(_fileName);
}
void MainWindow::on_actionLog_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionLog_triggered", _tid);
#endif
    _logGui->SetFileName(_fileName);
    _logGui->show();
}
void MainWindow::on_actionNotes_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionNotes_triggered", _tid);
#endif
    _notesGui->show();
}
void MainWindow::on_actionParameters_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionParameters_triggered", _tid);
#endif
    SaveModel(ds::TEMP_MODEL_FILE);
    ui->btnStart->setEnabled(false);
    SetButtonsEnabled(false);
    SetParamsEnabled(false);
    _paramEditor->SetFileName(_fileName);
    _paramEditor->show();
}

void MainWindow::on_actionReload_Current_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionReload_Current_triggered", _tid);
#endif
    if (_fileName.empty()) InitDefaultModel();
    else LoadModel(_fileName);
}

void MainWindow::on_actionSave_Data_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionSave_Data_triggered", _tid);
#endif
    if ( !QFile(ds::TEMP_FILE.c_str()).exists() ) return;
    QString file_name = QFileDialog::getSaveFileName(nullptr,
                                                         "Save generated data",
                                                         "");
    if (file_name.isEmpty()) return;
    QFile old(file_name);
    if (old.exists()) old.remove();
    QFile::rename(ds::TEMP_FILE.c_str(), file_name);
}
void MainWindow::on_actionSave_Model_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionSave_Model_triggered", _tid);
#endif
    if (_fileName.empty()) return;
    SaveModel(_fileName);
}
void MainWindow::on_actionSave_Model_As_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionSave_Model_As_triggered", _tid);
#endif
    std::string file_name = QFileDialog::getSaveFileName(nullptr,
                                                         "Save dynamical system",
                                                         "").toStdString();
    if (file_name.empty()) return;

    _fileName = file_name;

    SaveModel(file_name);
    setWindowTitle(("DynaSys " + ds::VERSION_STR + " - " + file_name).c_str());
}
void MainWindow::on_actionSave_Phase_Plot_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionSave_Phase_Plot_triggered", _tid);
#endif
    SaveFigure(ui->qwtPhasePlot, "phase plot", QSizeF(100, 100));
}
void MainWindow::on_actionSave_Time_Plot_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionSave_Time_Plot_triggered", _tid);
#endif
    SaveFigure(ui->qwtTimePlot, "time plot", QSizeF(200, 75));
}
void MainWindow::on_actionSave_Vector_Field_triggered()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_actionSave_Vector_Field_triggered", _tid);
#endif
    SaveFigure(ui->qwtPhasePlot, "vector field", QSizeF(100, 100));
}

void MainWindow::on_btnAddCondition_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnAddCondition_clicked", _tid);
#endif
    std::string cond = QInputDialog::getText(this, "New Condition",
                                                 "Condition (evaluates to true/false):",
                                                 QLineEdit::Normal).toStdString();
    if (!cond.empty())
    {
        _conditions->AddCondition(cond, VecStr());
        _parserMgr.SetConditions();
    }
}
void MainWindow::on_btnPulse_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnPulse_clicked", _tid);
#endif
    _pulseParIdx = ui->cmbPulsePars->currentIndex();
    if (_pulseParIdx==-1) _pulseParIdx = 0;
    _pulseResetValue = _parameters->Value(_pulseParIdx);
    _pulseStepsRemaining = (int)( ui->edPulseDuration->text().toDouble() / _parserMgr.ModelStep() );
    std::string val = ui->edPulseValue->text().toStdString();
    _parameters->SetPar((int)_pulseParIdx, val);

    if (ui->cboxVectorField->isChecked()) _needUpdateVF = true;
    if (ui->cboxNullclines->isChecked()) UpdateNullclines();
}
void MainWindow::on_btnAddDiff_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnAddDiff_clicked", _tid);
#endif
    std::string diff = QInputDialog::getText(this, "New Differential",
                                                 "Differential Name:",
                                                 QLineEdit::Normal).toStdString();
    if (!diff.empty())
    {
        _differentials->AddParameter(diff + "'", ParamModelBase::Param::DEFAULT_VAL);
        _initConds->AddParameter(diff + "(0)", ParamModelBase::Param::DEFAULT_VAL);
        UpdateTimePlotTable();
    }
}
void MainWindow::on_btnAddExpression_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnAddExpression_clicked", _tid);
#endif
    QModelIndex index = ui->lsConditions->currentIndex();
    if (index.parent().isValid() || index.row()==-1) return;

    std::string expr = QInputDialog::getText(this, "New Condition Result",
                                                 "Statement evaluated when condition satisfied",
                                                 QLineEdit::Normal).toStdString();
    if (expr.empty()) return;

    _conditions->AddExpression(index.row(), expr);
    ResetResultsList(index.row());
}
void MainWindow::on_btnAddParameter_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnAddParameter_clicked", _tid);
#endif
    std::string par = QInputDialog::getText(this, "New Parameter",
                                                 "Parameter Name:",
                                                 QLineEdit::Normal).toStdString();
    if (!par.empty())
    {
        _parameters->AddParameter(par, ParamModelBase::Param::DEFAULT_VAL);
        _parserMgr.InitModels();
    }
    UpdatePulseVList();
    UpdateSliderPList();
}
void MainWindow::on_btnAddVariable_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnAddVariable_clicked", _tid);
#endif
    std::string var = QInputDialog::getText(this, "New Variable",
                                                 "Variable Name:",
                                                 QLineEdit::Normal).toStdString();
    if (!var.empty())
    {
        _variables->AddParameter(var, ParamModelBase::Param::DEFAULT_VAL);
        UpdateTimePlotTable();
    }
}
void MainWindow::on_btnFitView_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnFitView_clicked", _tid);
#endif
    ui->cboxFitLimits->setChecked(false);
    const size_t xidx = ui->cmbDiffX->currentIndex(),
            yidx = ui->cmbDiffY->currentIndex();
    _initConds->SetMinimum(xidx, ui->qwtPhasePlot->axisScaleDiv(QwtPlot::xBottom).lowerBound());
    _initConds->SetMaximum(xidx, ui->qwtPhasePlot->axisScaleDiv(QwtPlot::xBottom).upperBound());
    _initConds->SetMinimum(yidx, ui->qwtPhasePlot->axisScaleDiv(QwtPlot::yLeft).lowerBound());
    _initConds->SetMaximum(yidx, ui->qwtPhasePlot->axisScaleDiv(QwtPlot::yLeft).upperBound());
    ui->tblInitConds->update();
    if (IsVFPresent()) UpdateVectorField();
    if (ui->cboxNullclines->isChecked()) UpdateNullclines();
}
void MainWindow::on_btnRemoveCondition_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnRemoveCondition_clicked", _tid);
#endif
    QModelIndexList rows = ui->lsConditions->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->lsConditions->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    _parserMgr.SetConditions();
}
void MainWindow::on_btnRemoveDiff_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnRemoveDiff_clicked", _tid);
#endif
    QModelIndexList rows = ui->tblDifferentials->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->tblDifferentials->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    ui->tblInitConds->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    UpdateTimePlotTable();
}
void MainWindow::on_btnRemoveExpression_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnRemoveExpression_clicked", _tid);
#endif
    QModelIndexList rows = ui->lsResults->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->lsResults->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());

    int row = ui->lsConditions->currentIndex().row();
    if (row == -1) row = 0;
    UpdateResultsModel(row);
}
void MainWindow::on_btnRemoveParameter_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnRemoveParameter_clicked", _tid);
#endif
    QModelIndexList rows = ui->tblParameters->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->tblParameters->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    UpdatePulseVList();
    UpdateSliderPList();
}
void MainWindow::on_btnRemoveVariable_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnRemoveVariable_clicked", _tid);
#endif
    QModelIndexList rows = ui->tblVariables->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->tblVariables->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    UpdateTimePlotTable();
}
void MainWindow::on_btnStart_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_btnStart_clicked", _tid);
#endif
    switch (_playState)
    {
        case STOPPED:
            _playState = DRAWING;
            _needInitialize = true;
            SetButtonsEnabled(false);
            ui->btnStart->setText("Stop");
            if (_plotMode==VECTOR_FIELD) _vfTailLen = 1;
            InitDraw();
            break;
        case DRAWING:
            _playState = STOPPED;
            SetButtonsEnabled(true);
            ui->btnStart->setText("Start");
            break;
        case PAUSED:
            _playState = DRAWING;
            ui->btnStart->setText("Stop");
            ResumeDraw();
            break;
    }
}
void MainWindow::on_cboxVectorField_stateChanged(int state)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_cboxVectorField_stateChanged", _tid);
#endif
    if (state==Qt::Checked)
    {
        _needUpdateVF = true;
        _needClearVF = false;
        if (_playState==STOPPED) InitDraw();
    }
    else
    {
        if (_plotMode==SINGLE) _needUpdateVF = _needClearVF = true;
    }
}
void MainWindow::on_cboxNullclines_stateChanged(int state)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_cboxNullclines_stateChanged", _tid);
#endif
    if (state==Qt::Checked)
        DrawNullclines();
    else
    {
        for (auto it : _ncPlotItems)
        {
            it->detach();
            delete it;
        }
        _ncPlotItems.clear();
    }
}

void MainWindow::on_cmbPlotMode_currentIndexChanged(const QString& text)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_cmbPlotMode_currentIndexChanged", _tid);
#endif
    ClearPlots();

    if (text=="Single")
    {
        _plotMode = SINGLE;
        ui->btnPulse->setEnabled(true);
        ui->qwtTimePlot->show();
        ui->tblTimePlot->show();
        ui->lblTimePlotN->show();
        ui->edNumTPSamples->show();
        ui->spnStepsPerSec->setValue(_singleStepsSec);
        ui->spnStepsPerSec->setMinimum(-1);
        ui->spnTailLength->setValue(_singleTailLen);
        _vfTailLen = 1;
    }
    else if (text=="Vector field")
    {
        _plotMode = VECTOR_FIELD;
        ui->btnPulse->setEnabled(false);
        ui->qwtTimePlot->hide();
        ui->tblTimePlot->hide();
        ui->lblTimePlotN->hide();
        ui->edNumTPSamples->hide();
        ui->spnStepsPerSec->setValue(_vfStepsSec);
        ui->spnStepsPerSec->setMinimum(1);
        ui->spnTailLength->setValue(_vfTailLen); //This updates the vector field
    }
}
void MainWindow::on_cmbSlidePars_currentIndexChanged(int index)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_cmbSlidePars_currentIndexChanged", _tid);
#endif
    if (index==-1) return;
    const double val = std::stod( _parameters->Value(index) ),
            min = _parserMgr.Minimum(_parameters, index),
            range = _parserMgr.Range(_parameters, index);
    const int scaled_val = ((val-min)/range) * SLIDER_INT_LIM + 0.5;
    ui->sldParameter->setValue( qBound(0, scaled_val, SLIDER_INT_LIM) );
}

void MainWindow::on_edModelStep_editingFinished()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_edModelStep_editingFinished", _tid);
#endif
    std::string text = ui->edModelStep->text().toStdString();
    size_t pos;
    double step = std::stod(text, &pos);
    if (pos>0)
    {
        _parserMgr.SetModelStep(step);
        static_cast<DifferentialModel*>(_differentials)->SetModelStep(text);
        _parserMgr.SetExpressions();
        _numTPSamples = (int)( ui->edNumTPSamples->text().toInt() / _parserMgr.ModelStep() );
    }
}
void MainWindow::on_edNumTPSamples_editingFinished()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_edNumTPSamples_editingFinished", _tid);
#endif
    _numTPSamples = (int)( ui->edNumTPSamples->text().toInt() / _parserMgr.ModelStep() );
}

void MainWindow::on_lsConditions_clicked(const QModelIndex& index)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_lsConditions_clicked", _tid);
#endif
    const int row = index.row();
    ResetResultsList(row);
}

void MainWindow::on_sldParameter_valueChanged(int value)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_sldParameter_valueChanged", _tid);
#endif
    const int index = ui->cmbSlidePars->currentIndex();
    const double range = _parserMgr.Range(_parameters, index);
    const double pct = (double)value / (double)SLIDER_INT_LIM,
            dval = pct*range + _parserMgr.Minimum(_parameters, index);
    _parameters->SetPar(index, std::to_string(dval));
    ui->tblParameters->update();
    if (ui->cboxNullclines->isChecked()) UpdateNullclines();
}
void MainWindow::on_spnStepsPerSec_valueChanged(int value)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_spnStepsPerSec_valueChanged", _tid);
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    switch (_plotMode)
    {
        case SINGLE:
            _singleStepsSec = value;
            break;
        case VECTOR_FIELD:
            _vfStepsSec = value;
            break;
    }
}
void MainWindow::on_spnTailLength_valueChanged(int value)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_spnTailLength_valueChanged", _tid);
#endif
    std::unique_lock<std::mutex> lock(_mutex);
    switch (_plotMode)
    {
        case SINGLE:
            _singleTailLen = value;
            break;
        case VECTOR_FIELD:
            _vfTailLen = value;
            lock.unlock();
            UpdateVectorField();
            break;
    }
}
void MainWindow::on_spnVFResolution_valueChanged(int)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::on_spnVFResolution_valueChanged", _tid);
#endif
    if (IsVFPresent()) UpdateVectorField();
    if (ui->cboxNullclines->isChecked()) UpdateNullclines();
}

void MainWindow::ClearPlots()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ClearPlots", _tid);
#endif
    AttachPhasePlot(false);
    AttachVectorField(false);
    AttachTimePlot(false);
}
void MainWindow::ConnectModels()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ConnectModels", _tid);
#endif
    connect(_parameters, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ParamChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    connect(_variables, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ExprnChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    connect(_differentials, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ExprnChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    connect(_initConds, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ExprnChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    connect(_conditions, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ExprnChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    if (_conditions->NumPars()==0)
        delete ui->lsResults->model();
    else
        ResetResultsList(0);
}
void MainWindow::Draw()
{
    ds::AddThread(std::this_thread::get_id());
#ifdef DEBUG_FUNC
    ScopeTracker::InitThread(std::this_thread::get_id());
    ScopeTracker st("MainWindow::Draw", std::this_thread::get_id());
#endif
    try
    {
        switch (_plotMode)
        {
            case SINGLE:
                DrawPhasePortrait();
                break;
            case VECTOR_FIELD:
                DrawVectorField();
                break;
        }
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::Draw: " + std::string(e.what()));
        ds::RemoveThread(std::this_thread::get_id());
    }

    ds::RemoveThread(std::this_thread::get_id());
}
void MainWindow::DrawNullclines()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::DrawNullclines", std::this_thread::get_id());
#endif
    for (auto it : _ncPlotItems)
    {
        it->detach();
        delete it;
    }
    _ncPlotItems.clear();

    const int num_diffs = (int)_differentials->NumPars(),
            num_vars = (int)_variables->NumPars();
    const double* diffs = _parserMgr.ConstData(_differentials),
            * vars = _parserMgr.ConstData(_variables);
    const int xidx = ui->cmbDiffX->currentIndex(),
            yidx = ui->cmbDiffY->currentIndex();

    const int vf_resolution = ui->spnVFResolution->value()*2;
    const double xmin = _parserMgr.Minimum(_initConds, xidx),
            xmax = _parserMgr.Maximum(_initConds, xidx),
            ymin = _parserMgr.Minimum(_initConds, yidx),
            ymax = _parserMgr.Maximum(_initConds, yidx);
    const double xinc = (xmax - xmin) / (double)(vf_resolution-1),
            yinc = (ymax - ymin) / (double)(vf_resolution-1);

    double* x = new double[vf_resolution*vf_resolution],
            * y = new double[vf_resolution*vf_resolution],
            * xdiff = new double[vf_resolution*vf_resolution],
            * ydiff = new double[vf_resolution*vf_resolution];

    try
    {
        if (_needInitialize) InitParserMgr();

        std::lock_guard<std::mutex> lock(_mutex);

        std::vector<double> dvals_orig(num_diffs),
                vars_orig(num_vars);
        for (int i=0; i<num_diffs; ++i)
            dvals_orig[i] = diffs[i];
        for (int i=0; i<num_vars; ++i)
            vars_orig[i] = vars[i];

        for (int i=0; i<vf_resolution; ++i)
            for (int j=0; j<vf_resolution; ++j)
            {
                const double xij = i*xinc + xmin,
                            yij = j*yinc + ymin;
                const int idx = i*vf_resolution+j;

                if (num_diffs>2)
                    _parserMgr.ResetDifferentials();
                _parserMgr.SetData(_differentials, xidx, xij);
                _parserMgr.SetData(_differentials, yidx, yij);
                _parserMgr.ParserEval(false);
                xdiff[idx] = diffs[xidx] - xij;
                ydiff[idx] = diffs[yidx] - yij;

                x[idx] = xij;
                y[idx] = yij;
            }

        for (int i=0; i<num_diffs; ++i)
            _parserMgr.SetData(_differentials, i, dvals_orig.at(i));
        for (int i=0; i<num_vars; ++i)
            _parserMgr.SetData(_variables, i, vars_orig.at(i));
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::DrawNullclines: " + std::string(e.what()));
        throw (e);
    }

    double lastx, lasty;
    std::vector< std::pair<int,int> > xcross_h, xcross_v, ycross_h, ycross_v;
    std::unordered_set<int> xidx_set, yidx_set; //To avoid duplicates;
    //The grid is being searched *column-wise*
    for (int j=0; j<vf_resolution; ++j)
    {
        lastx = xdiff[j];
        lasty = ydiff[j];
        for (int i=0; i<vf_resolution; ++i)
        {
            const int idx = i*vf_resolution+j;
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
    for (int i=0; i<vf_resolution; ++i)
    {
        lastx = xdiff[i*vf_resolution];
        lasty = ydiff[i*vf_resolution];
        for (int j=0; j<vf_resolution; ++j)
        {
            const int idx = i*vf_resolution+j;
            if (ds::sgn(lastx) != ds::sgn(xdiff[idx]) && xidx_set.count(idx)==0)
                xcross_h.push_back( std::make_pair(i,j) );
            lastx = xdiff[idx];

            if (ds::sgn(lasty) != ds::sgn(ydiff[idx]) && yidx_set.count(idx)==0)
                ycross_h.push_back( std::make_pair(i,j) );
            lasty = ydiff[idx];
        }
    }

    const size_t xnum_pts_h = xcross_h.size(),
            xnum_pts_v = xcross_v.size(),
            ynum_pts_h = ycross_h.size(),
            ynum_pts_v = ycross_v.size();
/*    //Now interpolate
    QPolygonF xpts(xnum_pts);
    for (size_t ct=0; ct<xnum_pts; ++ct)
    {
        int i = xcross.at(ct).first,
                j = xcross.at(ct).second,
                idx = i*vf_resolution+j;
        double xval = xinc*xdiff[idx-1]/std::abs(xdiff[idx]-xdiff[idx-vf_resolution])
                + x[idx-vf_resolution];
        xpts[ct] = QPointF(xval, y[idx]);
    }
    qDebug() << "\n\n";
    const size_t ynum_pts = ycross.size();
    QPolygonF ypts(ynum_pts);
    for (size_t ct=0; ct<ynum_pts; ++ct)
    {
        int i = ycross.at(ct).first,
                j = ycross.at(ct).second,
                idx = i*vf_resolution+j;
        double yval = yinc*ydiff[idx-1]/std::abs(ydiff[idx]-ydiff[idx-1]) + y[idx-1];
        ypts[ct] = QPointF(x[idx], yval);
    }
*/
/*    qDebug() << "x:";
    for (int i=0; i<vf_resolution; ++i)
    {
        std::string line;
        for (int j=0; j<vf_resolution; ++j)
        {
            const int idx = i*vf_resolution+j;
            line += std::to_string(xdiff[idx]) + "\t";
//            line += std::to_string(x[idx]) + "\t";
//            line += std::to_string(xcross[idx].first) + "\t";
        }
        qDebug() << line.c_str();
    }
    qDebug() << "\n\n";
    qDebug() << "y:";
    for (int i=0; i<vf_resolution; ++i)
    {
        std::string line;
        for (int j=0; j<vf_resolution; ++j)
        {
            const int idx = i*vf_resolution+j;
            line += std::to_string(ydiff[idx]) + "\t";
//            line += std::to_string(y[idx]) + "\t";
//            line += std::to_string(xcross[idx].second) + "\t";
        }
        qDebug() << line.c_str();
    }
    qDebug() << "\n\n";
*/

    QColor xcolor = _tpColors.at(xidx+1);
    for (size_t i=0; i<xnum_pts_h; ++i)
    {
        QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
            QBrush(xcolor), QPen(xcolor, 2), QSize(5, 5) );
        QwtPlotMarker* marker = new QwtPlotMarker();
        marker->setSymbol(symbol);
//        marker->setValue(xpts[i]);
        const int idx = xcross_h.at(i).first*vf_resolution + xcross_h.at(i).second;
        marker->setXValue(x[idx] + xinc/2.0);
        marker->setYValue(y[idx]);
        marker->setZ(-1);
        _ncPlotItems.push_back(marker);
    }
    for (size_t i=0; i<xnum_pts_v; ++i)
    {
        QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
            QBrush(xcolor), QPen(xcolor, 2), QSize(5, 5) );
        QwtPlotMarker* marker = new QwtPlotMarker();
        marker->setSymbol(symbol);
//        marker->setValue(xpts[i]);
        const int idx = xcross_v.at(i).first*vf_resolution + xcross_v.at(i).second;
        marker->setXValue(x[idx]);
        marker->setYValue(y[idx] + yinc/2.0);
        marker->setZ(-1);
        _ncPlotItems.push_back(marker);
    }

    QColor ycolor = _tpColors.at(yidx+1);
    for (size_t i=0; i<ynum_pts_h; ++i)
    {
        QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
            QBrush(ycolor), QPen(ycolor, 2), QSize(5, 5) );
        QwtPlotMarker* marker = new QwtPlotMarker();
        marker->setSymbol(symbol);
//        marker->setValue(ypts[i]);
        const int idx = ycross_h.at(i).first*vf_resolution + ycross_h.at(i).second;
        marker->setXValue(x[idx] + xinc/2.0);
        marker->setYValue(y[idx]);
        marker->setZ(-1);
        _ncPlotItems.push_back(marker);
    }
    for (size_t i=0; i<ynum_pts_v; ++i)
    {
        QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
            QBrush(ycolor), QPen(ycolor, 2), QSize(5, 5) );
        QwtPlotMarker* marker = new QwtPlotMarker();
        marker->setSymbol(symbol);
//        marker->setValue(ypts[i]);
        const int idx = ycross_v.at(i).first*vf_resolution + ycross_v.at(i).second;
        marker->setXValue(x[idx]);
        marker->setYValue(y[idx] + yinc/2.0);
        marker->setZ(-1);
        _ncPlotItems.push_back(marker);
    }

    delete[] x;
    delete[] y;
    delete[] xdiff;
    delete[] ydiff;
/*
    QwtPlotCurve* xcurve = new QwtPlotCurve();
    xcurve->setPen( _tpColors.at(xidx+1), 2 );
    xcurve->setRenderHint( QwtPlotItem::RenderAntialiased, true );
    xcurve->setSamples(xpts);
    xcurve->attach(ui->qwtPhasePlot);

    QwtPlotCurve* ycurve = new QwtPlotCurve();
    ycurve->setPen( _tpColors.at(yidx+1), 2 );
    ycurve->setRenderHint( QwtPlotItem::RenderAntialiased, true );
    ycurve->setSamples(ypts);
    ycurve->attach(ui->qwtPhasePlot);
*/
    ui->qwtPhasePlot->setAxisScale( QwtPlot::xBottom, xmin, xmax );
    ui->qwtPhasePlot->setAxisScale( QwtPlot::yLeft, ymin, ymax );
    for (auto it : _ncPlotItems)
        it->attach(ui->qwtPhasePlot);
    if (_playState==STOPPED) ui->qwtPhasePlot->replot();
}
void MainWindow::DrawPhasePortrait()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::DrawPhasePortrait", std::this_thread::get_id());
#endif

    //Get all of the information from the parameter fields, introducing new variables as needed.
    const int num_diffs = (int)_differentials->NumPars(),
            num_vars = (int)_variables->NumPars();
    const double* diffs = _parserMgr.ConstData(_differentials),
            * vars = _parserMgr.ConstData(_variables);
        //variables, differential equations, and initial conditions, all of which can invoke named
        //values

    QFile temp(ds::TEMP_FILE.c_str());
    std::string output;
    bool is_recording = ui->cboxRecord->isChecked();
    if (is_recording)
    {
        temp.open(QFile::WriteOnly | QFile::Text);
        for (size_t i=0; i<(size_t)num_diffs; ++i)
            output += _differentials->ShortKey(i) + "\t";
        for (size_t i=0; i<(size_t)num_vars; ++i)
            output += _variables->ShortKey(i)+ "\t";
        output += "\n";
        temp.write(output.c_str());
        temp.flush();
    }

    while (_playState==DRAWING)
    {
        if (_needUpdateVF) DrawVectorField();
        if (_needUpdateNullclines)
        {
            DrawNullclines();
            _needUpdateNullclines = false;
        }

        static auto last_step = std::chrono::system_clock::now();
        auto step_diff = std::chrono::system_clock::now() - last_step;
        auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(step_diff);
        bool need_new_step = diff_ms.count() > 1000.0/((double)_singleStepsSec/_parserMgr.ModelStep());
        if (!need_new_step)
            goto label;
        last_step = std::chrono::system_clock::now();
        {
        //Update the state vector with the value of the differentials.
            //Number of iterations to calculate in this refresh
        int num_steps = ((double)_singleStepsSec/_parserMgr.ModelStep()) / (double)SLEEP_MS + 0.5;
        if (num_steps==0) num_steps = 1;
        if (num_steps>MAX_BUF_SIZE) num_steps = MAX_BUF_SIZE;

            //Shrink the buffers if need be
        const int xy_buf_over = (int)_diffPts.at(0).size() + num_steps - MAX_BUF_SIZE;
        if (xy_buf_over>0)
        {
            for (int i=0; i<num_diffs; ++i)
                _diffPts[i].erase(_diffPts[i].begin(), _diffPts[i].begin()+xy_buf_over);
            for (int i=0; i<num_vars; ++i)
                _varPts[i].erase(_varPts[i].begin(), _varPts[i].begin()+xy_buf_over);
            _pastDVSampsCt += xy_buf_over;
        }
        const int ip_buf_over = (int)_ip.size() + num_steps - _numTPSamples;
        if (ip_buf_over>0)
        {
            _ip.erase(_ip.begin(), _ip.begin()+ip_buf_over);
            _pastIPSampsCt += ip_buf_over;
        }

            //Go through each expression and evaluate them
        try
        {
            if (_needInitialize)
            {
                emit DoInitParserMgr();
                std::unique_lock<std::mutex> lock(_mutex);
                _condVar.wait(lock, [&]{ return !_needInitialize; });
            }
            if (_needUpdateExprns)
            {
                _parserMgr.SetExpressions();
                _parserMgr.SetConditions();
                _needUpdateExprns = false;
            }

            for (int k=0; k<num_steps; ++k)
            {
                _parserMgr.ParserEvalAndConds();

                if (_pulseStepsRemaining>0) --_pulseStepsRemaining;
                if (_pulseStepsRemaining==0)
                {
                    emit DoUpdateParams();
                    ui->tblParameters->update();
                    _pulseStepsRemaining = -1;
                }
                // ### Somehow this isn't working if the (cumulative) pulse is longer than steps per second

                //Record updated variables for 2d graph, inner product, and output file
                double ip_k = 0;
                for (int i=0; i<num_diffs; ++i)
                {
                    _diffPts[i].push_back( diffs[i] );
                    ip_k += diffs[i] * diffs[i];
                }
                _ip.push_back(ip_k);
                for (int i=0; i<num_vars; ++i)
                    _varPts[i].push_back( vars[i] );

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
            }
        }
        catch (mu::ParserError& e)
        {
            _log->AddExcept("MainWindow::DrawPhasePortrait: " + e.GetMsg());
            throw std::runtime_error("MainWindow::DrawPhasePortrait: Parser error");
        }
        catch (std::exception& e)
        {
            _log->AddExcept("MainWindow::DrawPhasePortrait: " + std::string(e.what()));
            throw(e);
        }

        //A blowup will crash QwtPlot
        const double DMAX = std::numeric_limits<double>::max()/1e50;
        for (int i=0; i<num_diffs; ++i)
            if (abs(diffs[i])>DMAX)
            {
                _playState = STOPPED;
                ui->btnStart->setText("Start");
                return;
            }

        //Plot the current state vector
        const int xidx = ui->cmbDiffX->currentIndex(),
                yidx = ui->cmbDiffY->currentIndex();
        _marker->setValue(diffs[xidx], diffs[yidx]);

        const int num_saved_pts = (int)_diffPts[0].size();
        int tail_len = std::min(num_saved_pts, ui->spnTailLength->text().toInt());
        if (tail_len==-1) tail_len = num_saved_pts;
        const int inc = tail_len < XY_SAMPLES_SHOWN/2
                ? 1
                : tail_len / (XY_SAMPLES_SHOWN/2);
        const int num_drawn_pts = tail_len / inc;
        QPolygonF points(num_drawn_pts);

        int ct_begin = std::max(0,num_saved_pts-tail_len);
        for (int k=0, ct=ct_begin; k<num_drawn_pts; ++k, ct+=inc)
            points[k] = QPointF(_diffPts.at(xidx).at(ct), _diffPts.at(yidx).at(ct));
        _curve->setSamples(points);

        //Plot points for the time plot
        ViewRect tp_lims = DrawTimePlot(false);

            //Get axis limits
        double xmin, xmax, ymin, ymax;
        if (ui->cboxFitLimits->isChecked())
        {
            xmin = _initConds->Minimum(xidx);
            xmax = _initConds->Maximum(xidx);
            ymin = _initConds->Minimum(yidx);
            ymax = _initConds->Maximum(yidx);
        }
        else
        {
            auto xlims = std::minmax_element(_diffPts.at(xidx).cbegin(), _diffPts.at(xidx).cend()),
                    ylims = std::minmax_element(_diffPts.at(yidx).cbegin(), _diffPts.at(yidx).cend());
            xmin = *xlims.first;
            xmax = *xlims.second;
            ymin = *ylims.first;
            ymax = *ylims.second;
        }
        ViewRect pp_lims(xmin, xmax, ymin, ymax);

        _finishedReplot = false;
        emit DoReplot(pp_lims, tp_lims);
        std::unique_lock<std::mutex> lock(_mutex);
        _condVar.wait(lock, [&]{ return _finishedReplot; });
        lock.unlock();
        }
        label:
        std::this_thread::sleep_for( std::chrono::milliseconds(SLEEP_MS) );
    }

    temp.close();
}
MainWindow::ViewRect MainWindow::DrawTimePlot(bool replot_now)
{
//#ifdef DEBUG_FUNC
//    ScopeTracker st("MainWindow::DrawTimePlot", std::this_thread::get_id());
//#endif
    if (_diffPts.empty()) return ViewRect();
    TPVTableModel* tp_model = qobject_cast<TPVTableModel*>( ui->tblTimePlot->model() );
    const int num_diffs = (int)_differentials->NumPars(),
            num_vars = (int)_variables->NumPars(),
            num_tp_points = std::min( (int)_ip.size(), MAX_BUF_SIZE ),
            ip_start  = std::max(0, (int)_ip.size()-num_tp_points),
            dv_start = std::max(0, (int)_diffPts.at(0).size()-num_tp_points),
            dv_end = dv_start + num_tp_points;
    const int num_all_tplots = 1 + num_diffs + num_vars,
            step = std::max(1, num_tp_points / TP_SAMPLES_SHOWN),
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
        QwtPlotCurve* curv = _tpCurves[i];
        if (!tp_model->IsEnabled(i))
        {
            curv->setSamples(QPolygonF());
            continue;
        }

        const std::string name = tp_model->Name(i);
        const double scale = std::pow(10.0, tp_model->LogScale(i));
            // ### Use MSL for fast multiplication!

        if (name=="IP")
        {
            QPolygonF points_tp(num_plotted_pts);
            for (int k=ip_start+ip_step_off, ct=0; ct<num_plotted_pts; k+=step, ++ct)
                points_tp[ct] = QPointF((_pastIPSampsCt+k)*_parserMgr.ModelStep(), _ip[k]*scale);
            curv->setSamples(points_tp);
            continue;
        }
        int didx = _differentials->ShortKeyIndex(name);
        if (didx != -1)
        {
            QPolygonF points_tp(num_plotted_pts);
            for (int k=dv_start+dv_step_off, ct=0; ct<num_plotted_pts; k+=step, ++ct)
                points_tp[ct] = QPointF( (_pastDVSampsCt+k)*_parserMgr.ModelStep(), _diffPts.at(didx).at(k)*scale);
            curv->setSamples(points_tp);
            continue;
        }
        int vidx = _variables->KeyIndex(name);
        if (vidx != -1)
        {
            QPolygonF points_tp(num_plotted_pts);
            for (int k=dv_start+dv_step_off, ct=0; ct<num_plotted_pts; k+=step, ++ct)
                points_tp[ct] = QPointF( (_pastDVSampsCt+k)*_parserMgr.ModelStep(), _varPts.at(vidx).at(k)*scale);
            curv->setSamples(points_tp);
        }
    }

    //Get axis limits
    double y_tp_min(std::numeric_limits<double>::max()),
            y_tp_max(std::numeric_limits<double>::min());
    for (int i=0; i<num_all_tplots; ++i)
        if (tp_model->IsEnabled(i))
        {
            const QwtPlotCurve* curv = _tpCurves[i];
            if (curv->maxYValue() > y_tp_max) y_tp_max = curv->maxYValue();
            if (curv->minYValue() < y_tp_min) y_tp_min = curv->minYValue();
        }

    ViewRect tp_lims( (_pastDVSampsCt+dv_start)*_parserMgr.ModelStep(), (_pastDVSampsCt+dv_end)*_parserMgr.ModelStep(), y_tp_min, y_tp_max );

    if (replot_now)
    {
        assert(std::this_thread::get_id()==_tid && "MainWindow::DrawTimePlot Only replot from main thread!");
        Replot(ViewRect(), tp_lims);
    }

    return tp_lims;
}
void MainWindow::DrawVectorField()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::DrawVectorField", std::this_thread::get_id());
#endif

    if (_needClearVF)
    {
        emit DoAttachVF(false);
        std::unique_lock<std::mutex> lock(_mutex);
        _condVar.wait(lock, [&]{ return _vfPlotItems.empty(); });
        _needUpdateVF = _needClearVF = false;
        return;
    }

    if ((_playState==DRAWING && _plotMode==VECTOR_FIELD) || _needInitialize)
    {
        emit DoInitParserMgr();
        std::unique_lock<std::mutex> lock(_mutex);
        _condVar.wait(lock, [&]{ return !_needInitialize; });
    }

    while ((_playState==DRAWING && _plotMode==VECTOR_FIELD) || _needUpdateVF)
    {
        auto loop_begin = std::chrono::system_clock::now();

        const int num_diffs = (int)_differentials->NumPars(),
                num_vars = (int)_variables->NumPars();
        const double* diffs = _parserMgr.ConstData(_differentials),
                * vars = _parserMgr.ConstData(_variables);
        const int xidx = ui->cmbDiffX->currentIndex(),
                yidx = ui->cmbDiffY->currentIndex();

        const int vf_resolution = ui->spnVFResolution->value();
        const double xmin = _parserMgr.Minimum(_initConds, xidx),
                xmax = _parserMgr.Maximum(_initConds, xidx),
                ymin = _parserMgr.Minimum(_initConds, yidx),
                ymax = _parserMgr.Maximum(_initConds, yidx);
        const double xinc = (xmax - xmin) / (double)(vf_resolution-1),
                yinc = (ymax - ymin) / (double)(vf_resolution-1),
                xpix_inc = (double)ui->qwtPhasePlot->width() / (double)(vf_resolution-1),
                ypix_inc = (double)ui->qwtPhasePlot->height() / (double)(vf_resolution-1);
        ArrowHead::SetConversions(xinc, yinc, xpix_inc, ypix_inc);

        _isVFAttached = true;
        emit DoAttachVF(false);
        std::unique_lock<std::mutex> lock(_mutex);
        _condVar.wait(lock, [&]{ return !_isVFAttached; });

        _vfPlotItems.reserve(vf_resolution*vf_resolution*3);

        std::vector<double> dvals_orig(num_diffs),
                vars_orig(num_vars);
        for (int i=0; i<num_diffs; ++i)
            dvals_orig[i] = diffs[i];
        for (int i=0; i<num_diffs; ++i)
            vars_orig[i] = vars[i];
        try
        {
            _parserMgr.ResetVarInitVals();
            for (int i=0; i<vf_resolution; ++i)
                for (int j=0; j<vf_resolution; ++j)
                {
                    QwtSymbol *symbol1 = new QwtSymbol( QwtSymbol::Ellipse,
                        QBrush(Qt::red), QPen(Qt::red, 2), QSize(2, 2) );
                    QwtPlotMarker* marker1 = new QwtPlotMarker();
                    marker1->setSymbol(symbol1);
                    QwtPlotCurve* arrow = new QwtPlotCurve();
                    arrow->setPen(Qt::darkYellow, 1);
                    arrow->setRenderHint( QwtPlotItem::RenderAntialiased, true );

                    QwtPlotCurve* curv = new QwtPlotCurve();
                    curv->setPen(Qt::blue, 1);
                    curv->setRenderHint( QwtPlotItem::RenderAntialiased, true );

                    _vfPlotItems.push_back(marker1);
                    _vfPlotItems.push_back(curv);
                    _vfPlotItems.push_back(arrow);

                    QPolygonF pts(_vfTailLen+1);
                    const double x = i*xinc + xmin,
                                y = j*yinc + ymin;
                    pts[0] = QPointF(x, y);
                    marker1->setXValue(x);
                    marker1->setYValue(y);
                    marker1->setZ(-1);

                    if (num_diffs>2)
                        _parserMgr.ResetValues();
                    _parserMgr.SetData(_differentials, xidx, x);
                    _parserMgr.SetData(_differentials, yidx, y);
                    for (int k=1; k<=_vfTailLen; ++k)
                    {
                        _parserMgr.ParserEval(false);
                        pts[k] = QPointF(diffs[xidx], diffs[yidx]);
                    }

                    const ArrowHead arrow_head(pts[_vfTailLen], pts[_vfTailLen-1]);
                    arrow->setSamples(arrow_head.Points());
                    arrow->setZ(0);

                    curv->setSamples(pts);
                    curv->setZ(-0.5);
                }
            if (_playState==DRAWING && _plotMode==VECTOR_FIELD) _vfTailLen += _vfStepsSec;

            for (int i=0; i<num_diffs; ++i)
                _parserMgr.SetData(_differentials, i, dvals_orig.at(i));
            for (int i=0; i<num_vars; ++i)
                _parserMgr.SetData(_variables, i, vars_orig.at(i));
        }
        catch (std::exception& e)
        {
            _log->AddExcept("MainWindow::DrawVectorField: " + std::string(e.what()));
            throw (e);
        }
        lock.unlock();

        emit DoAttachVF(true);
        std::unique_lock<std::mutex> lock2(_mutex);
        _condVar.wait(lock2, [&]{ return _isVFAttached; });
        lock2.unlock();

        _needUpdateVF = false;
        if (_plotMode==VECTOR_FIELD)
        {
            ViewRect pp_data(xmin, xmax, ymin, ymax);

            _finishedReplot = false;
            emit DoReplot(pp_data,  ViewRect());
            std::unique_lock<std::mutex> lock(_mutex);
            _condVar.wait(lock, [&]{ return _finishedReplot; });
            lock.unlock();

            auto loop_dur = std::chrono::system_clock::now() - loop_begin;
            int dur_ms = std::chrono::duration_cast<std::chrono::milliseconds>(loop_dur).count(),
                    sleep_time = std::max(10, VF_SLEEP_MS-dur_ms);
            std::this_thread::sleep_for( std::chrono::milliseconds(sleep_time) );
        }
    }

//    if (_plotMode==VECTOR_FIELD) _playState = STOPPED;
}

void MainWindow::InitBuffers()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::InitBuffers", _tid);
#endif
    const int num_diffs = (int)_differentials->NumPars(),
            num_vars = (int)_variables->NumPars();
    _diffPts = std::vector< std::deque<double> >(num_diffs);
    _varPts = std::vector< std::deque<double> >(num_vars);
    _ip.clear();
    _pastDVSampsCt = _pastIPSampsCt = 0;
}
void MainWindow::InitDefaultModel()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::InitDefaultModel", _tid);
#endif
    InitModels();

    _parameters->AddParameter("a", "4");
    _parameters->AddParameter("b", "10");

    _variables->AddParameter("q", Input::NORM_RAND_STR);
    _variables->AddParameter("r", "u*v");

    _differentials->AddParameter("v'", "(u + r + a)/b");
    _differentials->AddParameter("u'", "q*(b - v)");

    _initConds->AddParameter("v(0)", "1");
    _initConds->AddParameter("u(0)", "0");
    _initConds->SetRange(0, -30, 30);
    _initConds->SetRange(1, -30, 30);

    VecStr vs;
    vs.push_back("v = 1"); vs.push_back("u = 2");
    _conditions->AddCondition("v>30", vs);
    ui->lsConditions->setModel(_conditions);
    ui->lsConditions->setModelColumn(0);
    ResetResultsList(0);

    _numTPSamples = (int)( ui->edNumTPSamples->text().toInt() / _parserMgr.ModelStep() );

    UpdateLists();

    SaveModel(ds::TEMP_MODEL_FILE);
}
void MainWindow::InitDraw()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::InitDraw", _tid);
#endif
    InitPlots();
    InitBuffers();
    std::thread t( std::bind(&MainWindow::Draw, this) );
    t.detach();
}
void MainWindow::InitModels(const std::vector<ParamModelBase*>* models, ConditionModel* conditions)
{
#ifdef DEBUG_MW_FUNC
    ScopeTracker st("MainWindow::InitModels", std::this_thread::get_id());
#endif
    _parserMgr.ClearModels();

    if (_parameters) delete _parameters;
    _parameters = (models) ? (*models)[ds::INPUTS] : new ParamModel(this, ds::Model(ds::INPUTS));
    ui->tblParameters->setModel(_parameters);
    ui->tblParameters->setColumnHidden(ParamModelBase::FREEZE,true);
    ui->tblParameters->horizontalHeader()->setStretchLastSection(true);
    _parserMgr.AddModel(_parameters);

    if (_variables) delete _variables;
    _variables = (models) ? (*models)[ds::VARIABLES] : new VariableModel(this, ds::Model(ds::VARIABLES));
    ui->tblVariables->setModel(_variables);
    ui->tblVariables->setColumnHidden(ParamModelBase::MIN,true);
    ui->tblVariables->setColumnHidden(ParamModelBase::MAX,true);
    ui->tblVariables->setColumnWidth(ParamModelBase::FREEZE,25);
    ui->tblVariables->horizontalHeader()->setStretchLastSection(true);
    CheckBoxDelegate* cbbd_v = new CheckBoxDelegate(std::vector<QColor>(), this);
    ui->tblVariables->setItemDelegateForColumn(ParamModelBase::FREEZE, cbbd_v);
    VecStr vstr;
    vstr.push_back(Input::GAMMA_RAND_STR);
    vstr.push_back(Input::NORM_RAND_STR);
    vstr.push_back(Input::UNI_RAND_STR);
    ComboBoxDelegate* cmbd = new ComboBoxDelegate(vstr);
    ui->tblVariables->setItemDelegateForColumn(ParamModelBase::VALUE, cmbd);
    connect(cmbd, SIGNAL(ComboBoxChanged(size_t)), this, SLOT(ComboBoxChanged(size_t)));
    _parserMgr.AddModel(_variables);

    if (_differentials) delete _differentials;
    _differentials =(models) ? (*models)[ds::DIFFERENTIALS] :  new DifferentialModel(this, ds::Model(ds::DIFFERENTIALS));
    ui->tblDifferentials->setModel(_differentials);
    ui->tblDifferentials->horizontalHeader()->setStretchLastSection(true);
    ui->tblDifferentials->setColumnHidden(ParamModelBase::MIN,true);
    ui->tblDifferentials->setColumnHidden(ParamModelBase::MAX,true);
    ui->tblDifferentials->setColumnWidth(ParamModelBase::FREEZE,25);
    CheckBoxDelegate* cbbd_d = new CheckBoxDelegate(std::vector<QColor>(), this);
    ui->tblDifferentials->setItemDelegateForColumn(ParamModelBase::FREEZE, cbbd_d);
    _parserMgr.AddModel(_differentials);

    if (_initConds) delete _initConds;
    _initConds = (models) ? (*models)[ds::INIT_CONDS] : new InitialCondModel(this, ds::Model(ds::INIT_CONDS));
    ui->tblInitConds->setModel(_initConds);
    ui->tblInitConds->setColumnHidden(ParamModelBase::FREEZE,true);
    ui->tblInitConds->horizontalHeader()->setStretchLastSection(true);
    _parserMgr.AddModel(_initConds);

    if (_conditions) delete _conditions;
    _conditions = (conditions) ? conditions : new ConditionModel(this);
    ui->lsConditions->setModel(_conditions);
    ui->lsConditions->setModelColumn(0);
    ResetResultsList(-1);
    _parserMgr.SetCondModel(_conditions);
}
void MainWindow::InitPlots()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::InitPlots", _tid);
#endif
    switch (_plotMode)
    {
        case SINGLE:
        {
            //The phase portrait
            AttachPhasePlot(false);

            //The point indicating current value
            QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
                QBrush( Qt::yellow ), QPen( Qt::red, 2 ), QSize( 8, 8 ) );
            _marker = new QwtPlotMarker();
            _marker->setSymbol(symbol);
            _marker->setZ(1);
            _ppPlotItems.push_back(_marker);

            //The 'tail' of the plot
            _curve = new QwtPlotCurve();
            _curve->setPen( Qt::black, 1 );
            _curve->setRenderHint( QwtPlotItem::RenderAntialiased, true );
            _ppPlotItems.push_back(_curve);
            AttachPhasePlot(true);

            //The time plot
            AttachTimePlot(false);

            const int num_diffs = (int)_differentials->NumPars(),
                    num_vars = (int)_variables->NumPars();
            const int num_all_tplots = 1 + num_diffs + num_vars,
                    num_colors = (int)_tpColors.size();
                // +1 for inner product.  The strategy is to attach all possible curves
                //but only the enabled ones have non-empty samples.
            _tpCurves.resize(num_all_tplots);
            for (int i=0; i<num_all_tplots; ++i)
            {
                QwtPlotCurve* curv = new QwtPlotCurve();
                curv->setPen( _tpColors.at(i%num_colors), 1 );
                curv->setRenderHint( QwtPlotItem::RenderAntialiased, true );
                _tpCurves[i] = curv;
            }
            AttachTimePlot(true);

            break;
        }
        case VECTOR_FIELD:
        {
            AttachVectorField(false);
            _needClearVF = false;
            break;
        }
    }
}

const std::vector<QColor> MainWindow::InitTPColors() const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::InitTPColors", _tid);
#endif
    std::vector<QColor> vc;
    vc.push_back(Qt::black);
    vc.push_back(Qt::blue);
    vc.push_back(Qt::red);
    vc.push_back(Qt::green);
    vc.push_back(Qt::gray);
    vc.push_back(Qt::cyan);
    vc.push_back(Qt::magenta);
    vc.push_back(Qt::yellow);
    vc.push_back(Qt::darkBlue);
    vc.push_back(Qt::darkRed);
    vc.push_back(Qt::darkGreen);
    vc.push_back(Qt::darkGray);
    vc.push_back(Qt::darkCyan);
    vc.push_back(Qt::darkMagenta);
    vc.push_back(Qt::darkYellow);
    return vc;
}

void MainWindow::AttachPhasePlot(bool attach) //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::AttachPhasePlot", _tid);
    assert(_tid == std::this_thread::get_id() && "AttachPhasePlot called from worker thread!");
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    if (attach)
        for (auto it : _ppPlotItems)
            it->attach(ui->qwtPhasePlot);
    else
    {
        for (auto it : _ppPlotItems)
        {
            it->detach();
            delete it;
        }
        _ppPlotItems.clear();
    }
}
void MainWindow::AttachTimePlot(bool attach) //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::AttachTimePlot", _tid);
    assert(_tid == std::this_thread::get_id() && "AttachTimePlot called from worker thread!");
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    if (attach)
        for (auto it : _tpCurves)
            it->attach(ui->qwtTimePlot);
    else
    {
        for (auto it : _tpCurves)
        {
            it->detach();
            delete it;
        }
        _tpCurves.clear();
    }
}
void MainWindow::AttachVectorField(bool attach) //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::AttachVectorField", _tid);
    assert(_tid == std::this_thread::get_id() && "AttachVectorField called from worker thread!");
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    if (attach)
    {
        for (auto it : _vfPlotItems)
            it->attach(ui->qwtPhasePlot);
        _isVFAttached = true;
    }
    else
    {
        for (auto it : _vfPlotItems)
        {
            it->detach();
            delete it;
        }
        _vfPlotItems.clear();
        _isVFAttached = false;
    }
    _condVar.notify_one();
}
void MainWindow::ComboBoxChanged(size_t row) //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ComboBoxChanged", _tid);
#endif
    try
    {
        std::string text = _variables->Value(row);
        _parserMgr.AssignInput(_variables, row, text);
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::ComboBoxChanged: " + std::string(e.what()));
    }
}
void MainWindow::ExprnChanged(QModelIndex, QModelIndex) //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ExprnChanged", _tid);
#endif
//    _parserMgr.ResetVarInitVals();
    _needUpdateExprns = true;
    if (IsVFPresent()) UpdateVectorField();
    if (ui->cboxNullclines->isChecked()) UpdateNullclines();
}
void MainWindow::InitParserMgr() //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::InitParserMgr", _tid);
    assert(_tid == std::this_thread::get_id() && "InitParserMgr called from worker thread!");
#endif
    std::lock_guard<std::mutex> lock(_mutex);
    try
    {
        _parserMgr.InitModels();
        _parserMgr.InitParsers();
        _parserMgr.SetExpressions();
        _parserMgr.SetConditions();
        _needInitialize = false;
        _condVar.notify_one();
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::InitParserMgr: " + std::string(e.what()));
    }
}
void MainWindow::ParamChanged(QModelIndex topLeft, QModelIndex) //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ParamChanged", _tid);
#endif
    int idx = topLeft.row();
    std::string exprn = _parameters->Expression(idx);
    if (_parserMgr.AreModelsInitialized())
        _parserMgr.QuickEval(exprn);
    if (IsVFPresent()) UpdateVectorField();
    if (ui->cboxNullclines->isChecked()) UpdateNullclines();
}
void MainWindow::ResultsChanged(QModelIndex, QModelIndex) //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ResultsChanged", _tid);
#endif
    int cond_row = ui->lsConditions->currentIndex().row();
    if (cond_row==-1) return;
    UpdateResultsModel(cond_row);
}
void MainWindow::Replot(const ViewRect& pp_data, const ViewRect& tp_data) //slot
{
#ifdef DEBUG_FUNC
    assert(std::this_thread::get_id()==_tid && "Thread error: MainWindow::Replot");
#endif
    std::unique_lock<std::mutex> lock(_mutex);

    try
    {
        if (pp_data != ViewRect())
        {
            ui->qwtPhasePlot->setAxisScale( QwtPlot::xBottom, pp_data.xmin, pp_data.xmax );
            ui->qwtPhasePlot->setAxisScale( QwtPlot::yLeft, pp_data.ymin, pp_data.ymax );
            ui->qwtPhasePlot->replot();
        }

        if (tp_data != ViewRect())
        {
            ui->qwtTimePlot->setAxisScale( QwtPlot::xBottom, tp_data.xmin, tp_data.xmax );
            ui->qwtTimePlot->setAxisScale( QwtPlot::yLeft, tp_data.ymin, tp_data.ymax );
            ui->qwtTimePlot->replot();
        }
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::Replot: " + std::string(e.what()));
        if (_playState==DRAWING) on_btnStart_clicked();
    }
    _finishedReplot = true;
    _condVar.notify_one();

//    lock.unlock();
//    if (_plotMode==VECTOR_FIELD)
//        ui->spnTailLength->setValue(_vfTailLen);
}
void MainWindow::UpdateParams() //slot
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateParams", _tid);
#endif
    _parameters->SetPar((int)_pulseParIdx, _pulseResetValue);
    if (ui->cboxVectorField->isChecked()) _needUpdateVF = true;
}
bool MainWindow::IsVFPresent() const
{
//#ifdef DEBUG_FUNC
//    ScopeTracker st("MainWindow::IsVFPresent", _tid);
//#endif
    return ui->cboxVectorField->isChecked() || _plotMode==VECTOR_FIELD;
}
void MainWindow::LoadModel(const std::string& file_name)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::LoadModel", _tid);
#endif
    try
    {
        ClearPlots();

        SysFileIn in(file_name);
        std::vector<ParamModelBase*> models;
        ConditionModel* conditions = new ConditionModel(this);
        std::string model_step;
        Notes* notes = _notesGui->GetNotes();
        in.Load(models, model_step, conditions, notes);

        _parserMgr.ClearExpressions();

        //Have to do this before models are deleted!
        InitModels(&models, conditions);
        ConnectModels();

        ui->edModelStep->setText( model_step.c_str() );
        _numTPSamples = (int)( ui->edNumTPSamples->text().toInt() / _parserMgr.ModelStep() );

        ResetPhasePlotAxes();
        if (file_name != ds::TEMP_MODEL_FILE) UpdateLists();
        UpdateNotes();
        UpdateParamEditor();

        setWindowTitle(("DynaSys " + ds::
                        VERSION_STR + " - " + _fileName).c_str());
    }
    catch (std::exception& e)
    {
        _log->AddExcept("MainWindow::LoadModel: " + std::string(e.what()));
    }
}
void MainWindow::ResetPhasePlotAxes()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ResetPhasePlotAxes(", _tid);
#endif
    QStringList qdiffs = ds::VecStrToQSList( _differentials->ShortKeys() );

    ui->cmbDiffX->clear();
    ui->cmbDiffX->insertItems(0, qdiffs);
    ui->cmbDiffX->setCurrentIndex(0);

    const int yidx = _differentials->NumPars() > 1 ? 1 : 0;
    ui->cmbDiffY->clear();
    ui->cmbDiffY->insertItems(0, qdiffs);
    ui->cmbDiffY->setCurrentIndex(yidx);
}
void MainWindow::ResetResultsList(int cond_row)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ResetResultsList", _tid);
#endif
    delete ui->lsResults->model();
    if (cond_row==-1) return;
    QStringList qexprns = ds::VecStrToQSList( _conditions->Expressions(cond_row) );
    QStringListModel* model = new QStringListModel(qexprns);
    ui->lsResults->setModel(model);
    connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ResultsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
}
void MainWindow::ResumeDraw()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::ResumeDraw", _tid);
#endif
    std::thread t( std::bind(&MainWindow::Draw, this) );
    t.detach();
}
void MainWindow::SaveFigure(QwtPlot* fig, const QString& name, const QSizeF& size) const
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::SaveFigure", _tid);
#endif
    QString suff = name;
    suff.replace(' ', '_');
    std::string base_name = _fileName.substr(0, _fileName.find_last_of('.'));
    std::string file_name = QFileDialog::getSaveFileName(nullptr,
                                                     "Save " + name + " figure",
                                                     QString(base_name.c_str())
                                                         + "_" + suff + ".pdf").toStdString();
    if (file_name.empty()) return;
    QwtPlotRenderer renderer;
    renderer.renderDocument(fig, file_name.c_str(), "pdf", size);
}
void MainWindow::SaveModel(const std::string& file_name)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::SaveModel", _tid);
#endif
    std::vector<const ParamModelBase*> models;
    models.push_back(_parameters);
    models.push_back(_variables);
    models.push_back(_differentials);
    models.push_back(_initConds);
    double model_step = _parserMgr.ModelStep();
    const Notes* notes = _notesGui->GetNotes();
    SysFileOut out(file_name);
    out.Save(models, model_step, _conditions, notes);
}
void MainWindow::SetButtonsEnabled(bool is_enabled)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::SetButtonsEnabled", _tid);
#endif
    ui->btnAddCondition->setEnabled(is_enabled);
    ui->btnAddDiff->setEnabled(is_enabled);
    ui->btnAddExpression->setEnabled(is_enabled);
    ui->btnAddParameter->setEnabled(is_enabled);
    ui->btnAddVariable->setEnabled(is_enabled);
    ui->btnRemoveCondition->setEnabled(is_enabled);
    ui->btnRemoveDiff->setEnabled(is_enabled);
    ui->btnRemoveExpression->setEnabled(is_enabled);
    ui->btnRemoveParameter->setEnabled(is_enabled);
    ui->btnRemoveVariable->setEnabled(is_enabled);

    ui->actionClear->setEnabled(is_enabled);
    ui->actionLoad->setEnabled(is_enabled);
    ui->actionParameters->setEnabled(is_enabled);
    ui->actionReload_Current->setEnabled(is_enabled);
    ui->actionSave_Data->setEnabled(is_enabled);
    ui->actionSave_Model_As->setEnabled(is_enabled);
    ui->actionSave_Phase_Plot->setEnabled(is_enabled);
    ui->actionSave_Time_Plot->setEnabled(is_enabled);
    ui->actionSave_Vector_Field->setEnabled(is_enabled);

    ui->cmbPlotMode->setEnabled(is_enabled);
}
void MainWindow::SetParamsEnabled(bool is_enabled)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::SetParamsEnabled", _tid);
#endif
    ui->tblDifferentials->setEnabled(is_enabled);
    ui->tblInitConds->setEnabled(is_enabled);
    ui->tblParameters->setEnabled(is_enabled);
    ui->tblVariables->setEnabled(is_enabled);
    ui->lsConditions->setEnabled(is_enabled);
    ui->lsResults->setEnabled(is_enabled);
}
void MainWindow::UpdateLists()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateList", _tid);
#endif
    UpdatePulseVList();
    UpdateSliderPList();
    UpdateTimePlotTable();
}
void MainWindow::UpdateNotes()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateNotes", _tid);
#endif
    _notesGui->SetFileName(_fileName);
    _notesGui->UpdateNotes();
}
void MainWindow::UpdateNullclines()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateNullclines", _tid);
#endif
    if (_playState==DRAWING)
        _needUpdateNullclines = true;
    else
        DrawNullclines();
}
void MainWindow::UpdateParamEditor()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateParamEditor", _tid);
#endif
    SaveModel(ds::TEMP_MODEL_FILE);
    _paramEditor->SetFileName(_fileName);
    _paramEditor->UpdateParameters();
}
void MainWindow::UpdatePulseVList()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdatePulseVList", _tid);
#endif
    ui->cmbPulsePars->clear();
    VecStr keys = _parameters->Keys();
    for (size_t i=0; i<keys.size(); ++i)
        ui->cmbPulsePars->insertItem((int)i, keys.at(i).c_str());
}
void MainWindow::UpdateSliderPList()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateSliderPList", _tid);
#endif
    ui->cmbSlidePars->clear();
    VecStr keys = _parameters->Keys();
    for (size_t i=0; i<keys.size(); ++i)
        ui->cmbSlidePars->insertItem((int)i, keys.at(i).c_str());
}
void MainWindow::UpdateResultsModel(int cond_row)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateResultsModel", _tid);
#endif
    QStringList items = qobject_cast<QStringListModel*>(ui->lsResults->model())->stringList();
    VecStr exprns;
    for (const auto& it : items)
        exprns.push_back(it.toStdString());
    _conditions->SetExpressions(cond_row, exprns);
}
void MainWindow::UpdateTimePlotTable()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateTimePlotTable", _tid);
#endif
    delete ui->tblTimePlot->model();
    VecStr vs,
            dkeys = _differentials->ShortKeys(),
            vkeys = _variables->Keys();
    vs.push_back("IP");
    vs.insert(vs.end(), dkeys.cbegin(), dkeys.cend());
    vs.insert(vs.end(), vkeys.cbegin(), vkeys.cend());

    TPVTableModel* model = new TPVTableModel(vs, this);
    ui->tblTimePlot->setModel(model);

    CheckBoxDelegate* cbd = new CheckBoxDelegate(_tpColors, this);
    connect(cbd, SIGNAL(MouseReleased()), this, SLOT(UpdateTimePlot()), Qt::DirectConnection);
    ui->tblTimePlot->setItemDelegateForColumn(0, cbd);
    ui->tblTimePlot->setColumnWidth(TPVTableModel::SHOW,50);
    model->setHeaderData(TPVTableModel::SHOW,Qt::Horizontal,"Show",Qt::DisplayRole);

    DSpinBoxDelegate* dsbd = new DSpinBoxDelegate(this);
    connect(dsbd, SIGNAL(DataChanged()), this, SLOT(UpdateTimePlot()), Qt::DirectConnection);
    ui->tblTimePlot->setItemDelegateForColumn(1, dsbd);
    ui->tblTimePlot->setColumnWidth(TPVTableModel::SCALE,50);
    model->setHeaderData(TPVTableModel::SCALE,Qt::Horizontal,"Scale",Qt::DisplayRole);
}
void MainWindow::UpdateVectorField()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("MainWindow::UpdateVectorField", _tid);
#endif
    if (_needUpdateVF) return;
    _needUpdateVF = true;
    if (_playState==STOPPED) InitDraw();
}
