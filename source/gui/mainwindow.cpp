#include "mainwindow.h"
#include "ui_mainwindow.h"

const int MainWindow::MAX_BUF_SIZE = 1024 * 1024;
const int MainWindow::SLEEP_MS = 50;
const int MainWindow::SLIDER_INT_LIM = 10000;
const int MainWindow::IP_SAMPLES_SHOWN = 10000;
const int MainWindow::XY_SAMPLES_SHOWN = 16 * 1024;
const int MainWindow::VF_RESOLUTION = 20;
const int MainWindow::VF_SLEEP_MS = 250;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _conditions(nullptr), _differentials(nullptr), _initConds(nullptr),
    _variables(nullptr), _parameters(nullptr), _fileName(""),
    _isDrawing(false),
    _needClearVF(false), _needInitialize(true), _needUpdateExprns(false), _needUpdateVF(false),
    _plotMode(SINGLE), _pulseResetValue("-666"), _pulseStepsRemaining(-1),
    _tpColors(InitTPColors())
{
    ui->setupUi(this);

    QStringList modes;
    modes << "Single" << "Vector field";
    ui->cmbPlotMode->setModel( new QStringListModel(modes) );

    ui->tblTimePlot->horizontalHeader()->setStretchLastSection(true);
    ui->sldParameter->setRange(0, SLIDER_INT_LIM);
    ui->spnVFResolution->setValue(VF_RESOLUTION);

    setWindowTitle("DynaSys " + QString(ds::VERSION_STR.c_str()) + " - default model");

    InitDefaultModel();

    ResetPhasePlotAxes();
    UpdateTimePlotTable();

    ConnectModels();

    connect(this, SIGNAL(DoReplot()), this, SLOT(Replot()), Qt::QueuedConnection);
    connect(this, SIGNAL(DoUpdateParams()), this, SLOT(UpdateParams()), Qt::QueuedConnection);

    _aboutGui = new AboutGui();
    _aboutGui->setWindowModality(Qt::ApplicationModal);
}

MainWindow::~MainWindow()
{
    delete ui;
    QFile temp_file(ds::TEMP_FILE.c_str());
    if (temp_file.exists()) temp_file.remove();
}

void MainWindow::on_actionAbout_triggered()
{
    _aboutGui->show();
}

void MainWindow::on_actionClear_triggered()
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::on_actionClear_triggered";
#endif
    _isDrawing = _needUpdateVF = false;
//    std::lock_guard<std::mutex> lock(_mutex);
    InitModels();
    UpdateLists();
}

void MainWindow::on_actionLoad_triggered()
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::on_actionLoad_triggered";
#endif
//        std::lock_guard<std::mutex> lock(_mutex);

    std::string file_name = QFileDialog::getOpenFileName(nullptr,
                                                             "Load dynamical system",
#ifdef QT_DEBUG
                                                             "../DynaSysFiles").toStdString();
#else
                                                             "../../DynaSysFiles").toStdString();
#endif
    if (file_name.empty()) return;
    _fileName = file_name;
    LoadModel();
}
void MainWindow::on_actionReload_Current_triggered()
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::on_actionReload_Current_triggered";
#endif
    if (_fileName.empty()) InitDefaultModel();
    else LoadModel();
}

void MainWindow::on_actionSave_Data_triggered()
{
//    std::lock_guard<std::mutex> lock(_mutex);

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
//    std::lock_guard<std::mutex> lock(_mutex);

    std::string file_name = QFileDialog::getSaveFileName(nullptr,
                                                         "Save dynamical system",
                                                         "").toStdString();
    if (file_name.empty()) return;

    _fileName = file_name;

    std::vector<const ParamModelBase*> models;
    models.push_back(_parameters);
    models.push_back(_variables);
    models.push_back(_differentials);
    models.push_back(_initConds);
    SysFileOut out(file_name, models, _conditions);
    out.Save();
    setWindowTitle(("DynaSys " + ds::VERSION_STR + " - " + file_name).c_str());
}

void MainWindow::on_btnAddCondition_clicked()
{
//    std::lock_guard<std::mutex> lock(_mutex);
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
    qDebug() << "MainWindow::on_btnPulse_clicked";
#endif
//    ui->btnPulse->setEnabled(false);
    _pulseParIdx = ui->cmbVariables->currentIndex();
    if (_pulseParIdx==-1) _pulseParIdx = 0;
    _pulseResetValue = _parameters->Value(_pulseParIdx);
    _pulseStepsRemaining = ui->edPulseDuration->text().toInt();
    std::string val = ui->edPulseValue->text().toStdString();
    _parameters->SetPar((int)_pulseParIdx, val);

    if (ui->cboxVectorField->isChecked()) _needUpdateVF = true;
}
void MainWindow::on_btnAddDiff_clicked()
{
//    std::lock_guard<std::mutex> lock(_mutex);
    std::string diff = QInputDialog::getText(this, "New Differential",
                                                 "Differential Name:",
                                                 QLineEdit::Normal).toStdString();
    if (!diff.empty())
    {
        _differentials->AddParameter(diff + "'", "0");
        _initConds->AddParameter(diff + "(0)", "0");
        UpdateTimePlotTable();
    }
}
void MainWindow::on_btnAddExpression_clicked()
{
//    std::lock_guard<std::mutex> lock(_mutex);

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
//    std::lock_guard<std::mutex> lock(_mutex);
    std::string par = QInputDialog::getText(this, "New Parameter",
                                                 "Parameter Name:",
                                                 QLineEdit::Normal).toStdString();
    if (!par.empty())
    {
        _parameters->AddParameter(par, "0");
        _parserMgr.InitModels();
    }
    UpdatePulseVList();
    UpdateSliderPList();
}
void MainWindow::on_btnAddVariable_clicked()
{
//    std::lock_guard<std::mutex> lock(_mutex);
    std::string var = QInputDialog::getText(this, "New Variable",
                                                 "Variable Name:",
                                                 QLineEdit::Normal).toStdString();
    if (!var.empty())
    {
        _variables->AddParameter(var, "0");
        AddVarDelegate((int)_variables->NumPars()-1);
        UpdateTimePlotTable();
    }
}
void MainWindow::on_btnRemoveCondition_clicked()
{
//    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->lsConditions->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->lsConditions->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    _parserMgr.SetConditions();
}
void MainWindow::on_btnRemoveDiff_clicked()
{
//    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->tblDifferentials->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->tblDifferentials->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    ui->tblInitConds->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    UpdateTimePlotTable();
}
void MainWindow::on_btnRemoveExpression_clicked()
{
//    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->lsResults->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->lsResults->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());

    int row = ui->lsConditions->currentIndex().row();
    if (row == -1) row = 0;
    UpdateResultsModel(row);
}
void MainWindow::on_btnRemoveParameter_clicked()
{
//    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->tblParameters->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->tblParameters->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    UpdatePulseVList();
    UpdateSliderPList();
}
void MainWindow::on_btnRemoveVariable_clicked()
{
//    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->tblVariables->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    for (auto it : rows)
//    {
//        delete _cmbDelegates[ it.row() ]; //Actually it looks like Qt does this
        _cmbDelegates[ it.row() ] = nullptr;
//    }
    std::remove(_cmbDelegates.begin(), _cmbDelegates.end(), nullptr);
    ui->tblVariables->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    UpdateTimePlotTable();
}
void MainWindow::on_btnStart_clicked()
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::on_btnStart_clicked";
#endif
    if (_isDrawing)
    {
        _isDrawing = false;
        SetButtonsEnabled(true);
        ui->btnStart->setText("Start");
    }
    else // "Stop"
    {
        _isDrawing = _needInitialize = true;
        SetButtonsEnabled(false);
        ui->btnStart->setText("Stop");
        InitDraw();
    }
}
void MainWindow::on_cboxVectorField_stateChanged(int state)
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::on_cboxVectorField_stateChanged";
#endif
    if (state==Qt::Checked)
    {
        _needUpdateVF = true;
        _needClearVF = false;
        if (!_isDrawing) InitDraw();
    }
    else
    {
        if (_plotMode==SINGLE) _needUpdateVF = _needClearVF = true;
    }
}

void MainWindow::on_cmbPlotMode_currentIndexChanged(const QString& text)
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::on_cmbPlotMode_currentIndexChanged";
#endif
    ClearPlots();

    if (text=="Single")
    {
        _plotMode = SINGLE;
        ui->btnPulse->setEnabled(true);
    }
    else if (text=="Vector field")
    {
        _plotMode = VECTOR_FIELD;
        ui->btnPulse->setEnabled(false);
        UpdateVectorField();
    }
}
void MainWindow::on_cmbSlidePars_currentIndexChanged(int index)
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::on_cmbSlidePars_currentIndexChanged";
#endif
    if (index==-1) return;
    const double val = std::stod( _parameters->Value(index) ),
            min = _parserMgr.Minimum(_parameters, index),
            range = _parserMgr.Range(_parameters, index);
    const int scaled_val = ((val-min)/range) * SLIDER_INT_LIM + 0.5;
    qDebug() << "MainWindow::on_cmbSlidePars_currentIndexChanged" << index << val << scaled_val;
    ui->sldParameter->setValue( qBound(0, scaled_val, SLIDER_INT_LIM) );
}

void MainWindow::on_lsConditions_clicked(const QModelIndex& index)
{
    const int row = index.row();
    ResetResultsList(row);
}

void MainWindow::on_sldParameter_valueChanged(int value)
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::on_sldParameter_valueChanged";
#endif
//    std::lock_guard<std::mutex> lock(_mutex);
    const int index = ui->cmbSlidePars->currentIndex();
    const double range = _parserMgr.Range(_parameters, index);
    const double pct = (double)value / (double)SLIDER_INT_LIM,
            dval = pct*range + _parserMgr.Minimum(_parameters, index);
    _parameters->SetPar(index, std::to_string(dval));
    qDebug() << "MainWindow::on_sldParameter_valueChanged" << index << value << dval;
    ui->tblParameters->update();
    if (IsVFPresent()) UpdateVectorField();
}
void MainWindow::on_spnTailLength_valueChanged(int)
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::on_spnTailLength_valueChanged";
#endif
    if (IsVFPresent()) UpdateVectorField();
}
void MainWindow::on_spnVFResolution_valueChanged(int)
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::on_spnVFResolution_valueChanged";
#endif
    if (IsVFPresent()) UpdateVectorField();
}

void MainWindow::AddVarDelegate(int row, const std::string& type)
{
    AddVarDelegate(row);//, Input::Type(type));
}
void MainWindow::AddVarDelegate(int row)
{
    VecStr vstr;
    vstr.push_back(Input::GAMMA_RAND_STR);
    vstr.push_back(Input::NORM_RAND_STR);
    vstr.push_back(Input::UNI_RAND_STR);
    ComboBoxDelegate* cbd = new ComboBoxDelegate(vstr);
    ui->tblVariables->setItemDelegateForRow(row, cbd);
    connect(cbd, SIGNAL(ComboBoxChanged(const QString&)), this, SLOT(ComboBoxChanged(const QString&)));
    _cmbDelegates.push_back(cbd);
}
void MainWindow::ClearPlots()
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::ClearPlots";
#endif
    for (auto it : _ppPlotItems)
        it->detach();
    _ppPlotItems.clear();
    for (auto it : _vfPlotItems)
        it->detach();
    _vfPlotItems.clear();
    ui->qwtPhasePlot->detachItems();
}
void MainWindow::ConnectModels()
{
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
//    std::lock_guard<std::mutex> lock(_mutex);
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::Draw()";
#endif
    if (_mutex.try_lock())
    {
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
            qDebug() << "MainWindow::Draw" << e.what();
        }
        _mutex.unlock();
    }
}
void MainWindow::DrawPhasePortrait()
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::DrawPhasePortrait";
#endif
    for (auto it : _ppPlotItems)
        it->detach();
    ui->qwtTimePlot->detachItems();

    //The point indicating current value
    QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
        QBrush( Qt::yellow ), QPen( Qt::red, 2 ), QSize( 8, 8 ) );
    QwtPlotMarker* marker = new QwtPlotMarker();
    marker->setSymbol(symbol);
    marker->setZ(1);
    marker->attach(ui->qwtPhasePlot);
    _ppPlotItems.push_back(marker);

    //The 'tail' of the plot
    QwtPlotCurve* curve = new QwtPlotCurve();
    curve->setPen( Qt::black, 1 );
    curve->setRenderHint( QwtPlotItem::RenderAntialiased, true );
    curve->attach(ui->qwtPhasePlot);
    _ppPlotItems.push_back(curve);

    //The time plot
    const int num_diffs = (int)_differentials->NumPars(),
            num_vars = (int)_variables->NumPars();
    const int num_all_tplots = 1 + num_diffs + num_vars,
            num_colors = _tpColors.size();
        // +1 for inner product.  The strategy is to attach all possible curves
        //but only the enabled ones have non-empty samples.
    std::vector<QwtPlotCurve*> curve_tp(num_all_tplots);
    for (int i=0; i<num_all_tplots; ++i)
    {
        QwtPlotCurve* curv = new QwtPlotCurve();
        curv->setPen( _tpColors.at(i%num_colors), 1 );
        curv->setRenderHint( QwtPlotItem::RenderAntialiased, true );
        curv->attach(ui->qwtTimePlot);
        curve_tp[i] = curv;
    }

    //Get all of the information from the parameter fields, introducing new variables as needed.
    const double* diffs = _parserMgr.ConstData(_differentials),
            * vars = _parserMgr.ConstData(_variables);
        //variables, differential equations, and initial conditions, all of which can invoke named
        //values
    std::vector< std::deque<double> > diff_pts(num_diffs),
            var_pts(num_vars);
    std::deque<double> ip;

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

    while (_isDrawing)
    {
        if (_needUpdateVF) DrawVectorField();

        static auto last_step = std::chrono::system_clock::now();
        auto step_diff = std::chrono::system_clock::now() - last_step;
        auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(step_diff);
        bool need_new_step = diff_ms.count() > 1000.0 / (double)ui->spnStepsPerSec->value();
        if (!need_new_step)
            goto label;
        last_step = std::chrono::system_clock::now();
        {
//        std::lock_guard<std::mutex> lock(_mutex);

        //Update the state vector with the value of the differentials.
            //Number of iterations to calculate in this refresh
        int num_steps = (double)ui->spnStepsPerSec->value() / (double)SLEEP_MS + 0.5;
        if (num_steps==0) num_steps = 1;

            //Shrink the buffer if need be
        while (diff_pts.at(0).size() + num_steps > MAX_BUF_SIZE)
        {
            for (int i=0; i<num_diffs; ++i)
                diff_pts[i].pop_front();
            for (int i=0; i<num_vars; ++i)
                var_pts[i].pop_front();
        }
        while (ip.size() + num_steps > IP_SAMPLES_SHOWN)
            ip.pop_front();

            //Go through each expression and evaluate them
        try
        {
            if (_needInitialize) InitParserMgr();
            if (_needUpdateExprns)
            {
                _parserMgr.SetExpressions();
                _parserMgr.SetConditions();
                _needUpdateExprns = false;
            }

            for (int k=0; k<num_steps; ++k)
            {
                _parserMgr.ParserEval();
#ifdef QT_DEBUG
                std::string sd;
                for (int i=0; i<num_diffs; ++i)
                    sd += std::to_string(diffs[i]) + ", ";
//                qDebug() << "Diffs: " << sd.c_str();

                std::string sv;
                for (int i=0; i<num_vars; ++i)
                    sv += std::to_string(vars[i]) + ", ";
//                qDebug() << "Vars: " << sv.c_str();
#endif
                _parserMgr.ParserCondEval();

                if (_pulseStepsRemaining>0) --_pulseStepsRemaining;
                if (_pulseStepsRemaining==0)
                {
                    emit DoUpdateParams();
                    ui->tblParameters->update();
                    _pulseStepsRemaining = -1;
                }

                //Record updated variables for 2d graph, inner product, and output file
                double ip_k = 0;
                output.clear();
                for (int i=0; i<num_diffs; ++i)
                {
                    if (is_recording)
                        output += std::to_string(diffs[i]) + "\t";
                    diff_pts[i].push_back( diffs[i] );
                    ip_k += diffs[i] * diffs[i];
                }
                for (int i=0; i<num_vars; ++i)
                    var_pts[i].push_back( vars[i] );
                if (is_recording)
                {
                    for (int i=0; i<num_vars; ++i)
                        output += std::to_string(vars[i]) + "\t";
                    output += "\n";
                    temp.write(output.c_str());
                    temp.flush();
                }
                ip.push_back(ip_k);
            }

        }
        catch (mu::ParserError& e)
        {
            std::cout << e.GetMsg() << std::endl;
            qDebug() << e.GetMsg().c_str();
            break;
        }

        //A blowup will crash QwtPlot
//        if (std::isinf(diffs[0]) || std::isinf(diffs[1]) || std::isnan(diffs[0]) || std::isnan(diffs[1]))
        const double DMAX = std::numeric_limits<double>::max()/1000;
        for (int i=0; i<num_diffs; ++i)
            if (abs(diffs[i])>DMAX)
            {
                _isDrawing = false;
                return;
            }

        //Plot the current state vector
        const int xidx = ui->cmbDiffX->currentIndex(),
                yidx = ui->cmbDiffY->currentIndex();
        marker->setValue(diffs[xidx], diffs[yidx]);

        const int num_saved_pts = (int)diff_pts[0].size();
        int tail_len = std::min(num_saved_pts, ui->spnTailLength->text().toInt());
        if (tail_len==-1) tail_len = num_saved_pts;
        const int inc = tail_len < XY_SAMPLES_SHOWN/2
                ? 1
                : tail_len / (XY_SAMPLES_SHOWN/2);
        const int num_drawn_pts = tail_len / inc;
        QPolygonF points(num_drawn_pts);
//        qDebug() << tail_len << inc;
        int ct_begin = std::max(0,num_saved_pts-tail_len);
        for (int k=0, ct=ct_begin; k<num_drawn_pts; ++k, ct+=inc)
            points[k] = QPointF(diff_pts.at(xidx).at(ct), diff_pts.at(yidx).at(ct));
        curve->setSamples(points);

        //Plot points for the inner-product graph
        const int num_tp_points = (int)ip.size();
        TPVTableModel* tp_model = qobject_cast<TPVTableModel*>( ui->tblTimePlot->model() );
        const int dv_start = std::max(0, (int)diff_pts.at(0).size()-num_tp_points),
                dv_end = dv_start + num_tp_points;
        for (int i=0; i<num_all_tplots; ++i)
        {
            QwtPlotCurve* curv = curve_tp[i];
            if (!tp_model->IsEnabled(i))
            {
                curv->setSamples(QPolygonF());
                continue;
            }
            std::string name = tp_model->Name(i);
            if (name=="IP")
            {
                QPolygonF points_tp(num_tp_points);
                for (int k=0; k<num_tp_points; ++k)
                    points_tp[k] = QPointF(dv_start+k, ip[k]);
                curv->setSamples(points_tp);
                continue;
            }
            int didx = _differentials->ShortKeyIndex(name);
            if (didx != -1)
            {
                QPolygonF points_tp(num_tp_points);
                for (int k=dv_start, ct=0; k<dv_end; ++k, ++ct)
                    points_tp[ct] = QPointF(k, diff_pts.at(didx).at(k));
                curv->setSamples(points_tp);
                continue;
            }
            int vidx = _variables->KeyIndex(name);
            if (vidx != -1)
            {
                QPolygonF points_tp(num_tp_points);
                for (int k=dv_start, ct=0; k<dv_end; ++k, ++ct)
                    points_tp[ct] = QPointF(k, var_pts.at(vidx).at(k));
                curv->setSamples(points_tp);
            }
        }

            //Get axis limits for time plot
        double y_tp_min(std::numeric_limits<double>::max()),
                y_tp_max(std::numeric_limits<double>::min());
        for (int i=0; i<num_all_tplots; ++i)
            if (tp_model->IsEnabled(i))
            {
                QwtPlotCurve* curv = curve_tp[i];
                if (curv->maxYValue() > y_tp_max) y_tp_max = curv->maxYValue();
                if (curv->minYValue() < y_tp_min) y_tp_min = curv->minYValue();
            }
        ui->qwtTimePlot->setAxisScale( QwtPlot::xBottom, dv_start, dv_end );
        ui->qwtTimePlot->setAxisScale( QwtPlot::yLeft, y_tp_min, y_tp_max );

            //Get axis limits
        auto xlims = std::minmax_element(diff_pts.at(xidx).cbegin(), diff_pts.at(xidx).cend()),
                ylims = std::minmax_element(diff_pts.at(yidx).cbegin(), diff_pts.at(yidx).cend());
        ui->qwtPhasePlot->setAxisScale( QwtPlot::xBottom, *xlims.first, *xlims.second );
        ui->qwtPhasePlot->setAxisScale( QwtPlot::yLeft, *ylims.first, *ylims.second );

        emit DoReplot();
        }
        label:
        std::this_thread::sleep_for( std::chrono::milliseconds(SLEEP_MS) );
    }

    temp.close();
    _isDrawing = false;
}
void MainWindow::DrawVectorField()
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::DrawVectorField";
#endif
//    qDebug() << "MainWindow::DrawVectorField 1";
//    std::lock_guard<std::mutex> lock(_mutex);
//    qDebug() << "MainWindow::DrawVectorField 2";

    if (_needClearVF)
    {
        for (auto it : _vfPlotItems)
            it->detach();
        _vfPlotItems.clear();
        _needUpdateVF = _needClearVF = false;
        return;
    }

    if ((_isDrawing && _plotMode==VECTOR_FIELD) || _needInitialize)
    {
        InitParserMgr();
        _needInitialize = false;
    }

    int num_steps = (_plotMode==VECTOR_FIELD && !_isDrawing)
            ? std::max(1, ui->spnTailLength->value())
            : 1;
    while ((_isDrawing && _plotMode==VECTOR_FIELD) || _needUpdateVF)
    {
        const int num_diffs = _differentials->NumPars();
        const double* diffs = _parserMgr.ConstData(_differentials);
        const int xidx = ui->cmbDiffX->currentIndex(),
                yidx = ui->cmbDiffY->currentIndex();

        const int vf_resolution = ui->spnVFResolution->value();
        const double xmin = _parserMgr.Minimum(_initConds, xidx),
                xmax = _parserMgr.Maximum(_initConds, xidx),
                ymin = _parserMgr.Minimum(_initConds, yidx),
                ymax = _parserMgr.Maximum(_initConds, yidx);
        const double xinc = (xmax - xmin) / (double)(vf_resolution-1),
                yinc = (ymax - ymin) / (double)(vf_resolution-1);

        for (auto it : _vfPlotItems)
            it->detach();
        _vfPlotItems.clear();
        _vfPlotItems.reserve(vf_resolution*vf_resolution*3);

        ui->qwtPhasePlot->setAxisScale( QwtPlot::xBottom, xmin, xmax );
        ui->qwtPhasePlot->setAxisScale( QwtPlot::yLeft, ymin, ymax );

    //    qDebug() << "MainWindow::DrawVectorField 3";
    //    qDebug() << "MainWindow::DrawVectorField 4";
        const double dval_x_orig = diffs[xidx],
                dval_y_orig = diffs[yidx];
        try
        {
            for (int i=0; i<vf_resolution; ++i)
                for (int j=0; j<vf_resolution; ++j)
                {
                    QwtSymbol *symbol1 = new QwtSymbol( QwtSymbol::Ellipse,
                        QBrush(Qt::red), QPen(Qt::red, 2), QSize(2, 2) );
                    QwtSymbol *symbol2 = new QwtSymbol( QwtSymbol::Ellipse,
                        QBrush(Qt::yellow), QPen(Qt::green, 2), QSize(2, 2) );
                    QwtPlotMarker* marker1 = new QwtPlotMarker(),
                            * marker2 = new QwtPlotMarker;
                    marker1->setSymbol(symbol1);
                    marker1->attach(ui->qwtPhasePlot);
                    marker2->setSymbol(symbol2);
                    marker2->attach(ui->qwtPhasePlot);

                    QwtPlotCurve* curv = new QwtPlotCurve();
                    curv->setPen(Qt::blue, 1);
                    curv->setRenderHint( QwtPlotItem::RenderAntialiased, true );
                    curv->attach(ui->qwtPhasePlot);

                    _vfPlotItems.push_back(marker1);
                    _vfPlotItems.push_back(marker2);
                    _vfPlotItems.push_back(curv);

                    QPolygonF pts(num_steps+1);
                    const double x = i*xinc + xmin,
                                y = j*yinc + ymin;
                    pts[0] = QPointF(x, y);
                    marker1->setXValue(x);
                    marker1->setYValue(y);
                    marker1->setZ(-1);

                    if (num_diffs>2)
                        _parserMgr.ResetDifferentials();
                    _parserMgr.SetData(_differentials, xidx, x);
                    _parserMgr.SetData(_differentials, yidx, y);
                    for (int k=1; k<=num_steps; ++k)
                    {
                        _parserMgr.ParserEval(false);
                        pts[k] = QPointF(diffs[xidx], diffs[yidx]);
                    }
                    marker2->setXValue(pts[num_steps].x());
                    marker2->setYValue(pts[num_steps].y());
                    marker2->setZ(0);

    //                qDebug() << i << j << pts[0] << pts[1];

                    curv->setSamples(pts);
                    curv->setZ(-0.5);
                }
            ++num_steps;
        }
        catch (std::exception& e)
        {
            qDebug() << "MainWindow::DrawVectorField" << e.what();
        }
        _parserMgr.SetData(_differentials, xidx, dval_x_orig);
        _parserMgr.SetData(_differentials, yidx, dval_y_orig);

    //    qDebug() << "MainWindow::DrawVectorField 5";
        _needUpdateVF = false;
        if (_plotMode==VECTOR_FIELD)
        {
            emit DoReplot();
            std::this_thread::sleep_for( std::chrono::milliseconds(VF_SLEEP_MS) );
        }
        std::this_thread::sleep_for( std::chrono::milliseconds(SLEEP_MS) );
    //    qDebug() << "MainWindow::DrawVectorField 6";
    }

    if (_plotMode==VECTOR_FIELD) _isDrawing = false;
}

void MainWindow::InitDefaultModel()
{
    InitModels();

    _parameters->AddParameter("a", "4");
    _parameters->AddParameter("b", "10");

    _variables->AddParameter("q", Input::NORM_RAND_STR);
    _variables->AddParameter("r", "u*v");
    _variables->AddParameter("tau", "0.1");
    AddVarDelegate(0);
    AddVarDelegate(1);
    AddVarDelegate(2);

    _differentials->AddParameter("v'", "tau*(u + a)/b");
    _differentials->AddParameter("u'", "tau*(b - v)");

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

    UpdateLists();
}
void MainWindow::InitDraw()
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::InitDraw";
#endif
    std::thread t( std::bind(&MainWindow::Draw, this) );
    t.detach();
}
void MainWindow::InitModels(const std::vector<ParamModelBase*>* models, ConditionModel* conditions)
{
    _parserMgr.ClearModels();

    if (_parameters) delete _parameters;
    _parameters = (models) ? (*models)[0] : new ParamModel(this, "Parameters");
    ui->tblParameters->setModel(_parameters);
    ui->tblParameters->horizontalHeader()->setStretchLastSection(true);
    _parserMgr.AddModel(_parameters);

    if (_variables) delete _variables;
    _variables = (models) ? (*models)[1] : new VariableModel(this, "Variables");
    ui->tblVariables->setModel(_variables);
    ui->tblVariables->setColumnHidden(1,true);
    ui->tblVariables->setColumnHidden(2,true);
    ui->tblVariables->horizontalHeader()->setStretchLastSection(true);
    _parserMgr.AddModel(_variables);

    if (_differentials) delete _differentials;
    _differentials =(models) ? (*models)[2] :  new DifferentialModel(this, "Differentials");
    ui->tblDifferentials->setModel(_differentials);
    ui->tblDifferentials->horizontalHeader()->setStretchLastSection(true);
    ui->tblDifferentials->setColumnHidden(1,true);
    ui->tblDifferentials->setColumnHidden(2,true);
    _parserMgr.AddModel(_differentials);

    if (_initConds) delete _initConds;
    _initConds = (models) ? (*models)[3] : new InitialCondModel(this, "InitialConds");
    ui->tblInitConds->setModel(_initConds);
    ui->tblInitConds->horizontalHeader()->setStretchLastSection(true);
    _parserMgr.AddModel(_initConds);

    if (_conditions) delete _conditions;
    _conditions = (conditions) ? conditions : new ConditionModel(this);
    ui->lsConditions->setModel(_conditions);
    ui->lsConditions->setModelColumn(0);
    ResetResultsList(-1);
    _parserMgr.SetCondModel(_conditions);
}
void MainWindow::InitParserMgr()
{
    _parserMgr.InitModels();
    _parserMgr.InitParsers();
    _parserMgr.SetExpressions();
    _parserMgr.SetConditions();
    _needInitialize = false;
}

const std::vector<QColor> MainWindow::InitTPColors() const
{
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

void MainWindow::ComboBoxChanged(const QString& text) //slot
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::ComboBoxChanged";
#endif
//    std::lock_guard<std::mutex> lock(_mutex);
    ComboBoxDelegate* cbd = qobject_cast<ComboBoxDelegate*>(sender());
    size_t row = std::find(_cmbDelegates.cbegin(), _cmbDelegates.cend(), cbd) - _cmbDelegates.cbegin();
    _parserMgr.AssignInput(_variables, row, text.toStdString());
}
void MainWindow::ExprnChanged(QModelIndex, QModelIndex) //slot
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::ExprnChanged";
#endif
    _needUpdateExprns = true;
    if (IsVFPresent()) UpdateVectorField();
/*    const int xidx = ui->cmbDiffX->currentIndex(),
            yidx = ui->cmbDiffY->currentIndex();
    const double xmin = _parserMgr.Minimum(_initConds,xidx),
            xmax = _parserMgr.Maximum(_initConds,xidx),
            ymin = _parserMgr.Minimum(_initConds,yidx),
            ymax = _parserMgr.Maximum(_initConds,yidx);
    ui->qwtPhasePlot->setAxisScale( QwtPlot::xBottom, xmin, xmax );
    ui->qwtPhasePlot->setAxisScale( QwtPlot::yLeft, ymin, ymax );
    ui->qwtPhasePlot->replot();*/
}
void MainWindow::ParamChanged(QModelIndex topLeft, QModelIndex) //slot
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::ParamChanged";
#endif
//    std::lock_guard<std::mutex> lock(_mutex);
    int idx = topLeft.row();
    std::string exprn = _parameters->Expression(idx);
//    ui->sldParameter->setSliderPosition(100*atof(exprn.c_str()));
    if (_parserMgr.AreModelsInitialized())
        _parserMgr.QuickEval(exprn);
    if (IsVFPresent()) UpdateVectorField();
}
void MainWindow::ResultsChanged(QModelIndex, QModelIndex) //slot
{
//    std::lock_guard<std::mutex> lock(_mutex);
    int cond_row = ui->lsConditions->currentIndex().row();
    if (cond_row==-1) return;
    UpdateResultsModel(cond_row);
}
void MainWindow::Replot() //slot
{
//    std::lock_guard<std::mutex> lock(_mutex);
    ui->qwtPhasePlot->replot();
    ui->qwtTimePlot->replot();
}
void MainWindow::UpdateParams() //slot
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::UpdateParams";
#endif
    _parameters->SetPar((int)_pulseParIdx, _pulseResetValue);
    if (ui->cboxVectorField->isChecked()) _needUpdateVF = true;
}
bool MainWindow::IsVFPresent() const
{
    return ui->cboxVectorField->isChecked() || _plotMode==VECTOR_FIELD;
}
void MainWindow::LoadModel()
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::LoadModel";
#endif
    try
    {
        ClearPlots();

        std::vector<ParamModelBase*> models;
        ConditionModel* conditions = new ConditionModel(this);
        SysFileIn in(_fileName, models, conditions);
        in.Load();

        //Have to do this before models are deleted!
        InitModels(&models, conditions);
        ConnectModels();

        for (auto it : _cmbDelegates)
            delete it;
        _cmbDelegates.clear();
        const size_t num_vars = _variables->NumPars();
        for (size_t i=0; i<num_vars; ++i)
            AddVarDelegate((int)i, _variables->Value(i));

        ResetPhasePlotAxes();
        UpdateLists();

        setWindowTitle(("DynaSys " + ds::
                        VERSION_STR + " - " + _fileName).c_str());
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}
void MainWindow::ResetPhasePlotAxes()
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::ResetPhasePlotAxes";
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
    delete ui->lsResults->model();
    if (cond_row==-1) return;
    QStringList qexprns = ds::VecStrToQSList( _conditions->Expressions(cond_row) );
    QStringListModel* model = new QStringListModel(qexprns);
    ui->lsResults->setModel(model);
    connect(model, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ResultsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
}
void MainWindow::SetButtonsEnabled(bool is_enabled)
{
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

    ui->cmbPlotMode->setEnabled(is_enabled);
}
void MainWindow::UpdateLists()
{
    UpdatePulseVList();
    UpdateSliderPList();
    UpdateTimePlotTable();
}
void MainWindow::UpdatePulseVList()
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::UpdatePulseVList";
#endif
    ui->cmbVariables->clear();
    VecStr keys = _parameters->Keys(); // ### rename cmbVariables
    for (size_t i=0; i<keys.size(); ++i)
        ui->cmbVariables->insertItem((int)i, keys.at(i).c_str());
}
void MainWindow::UpdateSliderPList()
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::UpdateSliderPList";
#endif
    ui->cmbSlidePars->clear();
    VecStr keys = _parameters->Keys();
    for (size_t i=0; i<keys.size(); ++i)
        ui->cmbSlidePars->insertItem((int)i, keys.at(i).c_str());
}
void MainWindow::UpdateResultsModel(int cond_row)
{
    QStringList items = qobject_cast<QStringListModel*>(ui->lsResults->model())->stringList();
    VecStr exprns;
    for (const auto& it : items)
        exprns.push_back(it.toStdString());
    _conditions->SetExpressions(cond_row, exprns);
}
void MainWindow::UpdateTimePlotTable()
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::UpdateTimePlotTable";
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

    const size_t num_tp_rows = vs.size(),
            num_colors = _tpColors.size();
    for (size_t i=0; i<num_tp_rows; ++i)
    {
        CheckBoxDelegate* cbd = new CheckBoxDelegate(
                    _tpColors.at(i%num_colors), this);
        ui->tblTimePlot->setItemDelegateForRow((int)i, cbd);
    }
}
void MainWindow::UpdateVectorField()
{
#ifdef DEBUG_FUNC
    qDebug() << "MainWindow::UpdateVectorField";
#endif
    if (_needUpdateVF) return;
    _needUpdateVF = true;
    if (!_isDrawing) InitDraw();
}
