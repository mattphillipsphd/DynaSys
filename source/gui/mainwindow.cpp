#include "mainwindow.h"
#include "ui_mainwindow.h"

const int MainWindow::MAX_BUF_SIZE = 1024 * 1024;
const int MainWindow::SLEEP_MS = 10;
const int MainWindow::IP_SAMPLES_SHOWN = 100;
const std::string MainWindow::TEMP_FILE = ".temp.txt";


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), _isDrawing(false), _needGetParams(true), _thread(nullptr)
{
    ui->setupUi(this);

    setWindowTitle("DynaSys");

    _parameters = new ParamModel(this, "Parameters");
    _parameters->AddParameter("a", "4");
    _parameters->AddParameter("b", "10");
    ui->tblParameters->setModel(_parameters);
    ui->tblParameters->horizontalHeader()->setStretchLastSection(true);
    connect(_parameters, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ParamsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    _parserMgr.AddModel(_parameters);

    _variables = new VariableModel(this, "Variables");
    _variables->AddParameter("q", Input::NORM_RAND_STR);
    _variables->AddParameter("r", "u*v");
    ui->tblVariables->setModel(_variables);
    AddVarDelegate(0);
    AddVarDelegate(1);
    ui->tblVariables->horizontalHeader()->setStretchLastSection(true);
    connect(_variables, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ParamsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    _parserMgr.AddModel(_variables);

    _differentials = new DifferentialModel(this, "Differentials");
    _differentials->AddParameter("v'", "0.1*(u + a)/b");
    _differentials->AddParameter("u'", "0.1*(b - v)");
    ui->tblDifferentials->setModel(_differentials);
    ui->tblDifferentials->horizontalHeader()->setStretchLastSection(true);
    connect(_differentials, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ParamsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    _parserMgr.AddModel(_differentials);

    _initConds = new InitialCondModel(this, "InitialConds");
    _initConds->AddParameter("v(0)", "1");
    _initConds->AddParameter("u(0)", "0");
    ui->tblInitConds->setModel(_initConds);
    ui->tblInitConds->horizontalHeader()->setStretchLastSection(true);
    connect(_initConds, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ParamsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    _parserMgr.AddModel(_initConds);

    _conditions = new ConditionModel(this);
    VecStr vs;
    vs.push_back("v = 1"); vs.push_back("u = 2");
    _conditions->AddCondition("v>30", vs);
    ui->lsConditions->setModel(_conditions);
    ui->lsConditions->setModelColumn(0);
    ui->lsResults->setModel(_conditions);
    ui->lsResults->setModelColumn(1);
    connect(_conditions, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(ParamsChanged(QModelIndex,QModelIndex)), Qt::QueuedConnection);
    _parserMgr.SetCondModel(_conditions);

    connect(this, SIGNAL(DoReplot()), this, SLOT(Replot()), Qt::QueuedConnection);

    _aboutGui = new AboutGui();
    _aboutGui->setWindowModality(Qt::ApplicationModal);
}

MainWindow::~MainWindow()
{
    delete ui;
    if (_thread) _thread->detach();
    QFile temp_file(TEMP_FILE.c_str());
    if (temp_file.exists()) temp_file.remove();
}

void MainWindow::on_actionAbout_triggered()
{
    _aboutGui->show();
}

void MainWindow::on_actionLoad_triggered()
{
    try
    {
    std::lock_guard<std::mutex> lock(_mutex);

    std::string file_name = QFileDialog::getOpenFileName(nullptr,
                                                         "Load dynamical system",
                                                         "").toStdString();
    if (file_name.empty()) return;

    std::vector<ParamModel*> models;
    ConditionModel* conditions = new ConditionModel(this);
    SysFileIn in(file_name, models, conditions);
    in.Load();

    delete _parameters;     _parameters = models[0];    ui->tblParameters->setModel(_parameters);
    delete _variables;      _variables = models[1];     ui->tblVariables->setModel(_variables);
    delete _differentials;  _differentials = models[2]; ui->tblDifferentials->setModel(_differentials);
    delete _initConds;      _initConds = models[3];     ui->tblInitConds->setModel(_initConds);
    delete _conditions;     _conditions = conditions;   ui->lsConditions->setModel(_conditions);

    for (auto it : _cmbDelegates)
        delete it;
    _cmbDelegates.clear();
    const size_t num_vars = _variables->NumPars();
    for (size_t i=0; i<num_vars; ++i)
        AddVarDelegate((int)i, _variables->Value(i));
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}
void MainWindow::on_actionSave_Data_triggered()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if ( !QFile(TEMP_FILE.c_str()).exists() ) return;
    QString file_name = QFileDialog::getSaveFileName(nullptr,
                                                         "Save generated data",
                                                         "");
    if (file_name.isEmpty()) return;
    QFile::rename(TEMP_FILE.c_str(), file_name);
}
void MainWindow::on_actionSave_Model_triggered()
{
    std::lock_guard<std::mutex> lock(_mutex);

    std::string file_name = QFileDialog::getSaveFileName(nullptr,
                                                         "Save dynamical system",
                                                         "").toStdString();
    if (file_name.empty()) return;

    std::vector<const ParamModel*> models;
    models.push_back(_parameters);
    models.push_back(_variables);
    models.push_back(_differentials);
    models.push_back(_initConds);
    SysFileOut out(file_name, models, _conditions);
    out.Save();
}

void MainWindow::on_btnAddCondition_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::string cond = QInputDialog::getText(this, "New Condition",
                                                 "Condition (evaluates to true/false):",
                                                 QLineEdit::Normal).toStdString();
    if (!cond.empty())
        _conditions->AddCondition(cond, VecStr());
}
void MainWindow::on_btnAddDiff_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::string diff = QInputDialog::getText(this, "New Differential",
                                                 "Differential Name:",
                                                 QLineEdit::Normal).toStdString();
    if (!diff.empty())
    {
        _differentials->AddParameter(diff + "'", "0");
        _initConds->AddParameter(diff + "(0)", "0");
    }
}
void MainWindow::on_btnAddExpression_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);

    QModelIndex index = ui->lsConditions->currentIndex();
    if (index.parent().isValid()) return;

    std::string expr = QInputDialog::getText(this, "New Condition Result",
                                                 "Statement evaluated when condition satisfied",
                                                 QLineEdit::Normal).toStdString();
    if (expr.empty()) return;

    _conditions->AddExpression(index.row(), expr);
}
void MainWindow::on_btnAddParameter_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::string par = QInputDialog::getText(this, "New Parameter",
                                                 "Parameter Name:",
                                                 QLineEdit::Normal).toStdString();
    if (!par.empty())
        _parameters->AddParameter(par, "0");
}
void MainWindow::on_btnAddVariable_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::string var = QInputDialog::getText(this, "New Variable",
                                                 "Variable Name:",
                                                 QLineEdit::Normal).toStdString();
    if (!var.empty())
    {
        _variables->AddParameter(var, "0");
        AddVarDelegate((int)_variables->NumPars()-1);
    }
}
void MainWindow::on_btnRemoveCondition_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->lsConditions->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->lsConditions->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
}
void MainWindow::on_btnRemoveDiff_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->tblDifferentials->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->tblDifferentials->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    ui->tblInitConds->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
}
void MainWindow::on_btnRemoveExpression_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->lsResults->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->lsResults->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
}
void MainWindow::on_btnRemoveParameter_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->tblParameters->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->tblParameters->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
    for (auto it : rows)
    {
        delete _cmbDelegates[ it.row() ];
        _cmbDelegates[ it.row() ] = nullptr;
    }
    std::remove(_cmbDelegates.begin(), _cmbDelegates.end(), nullptr);
}
void MainWindow::on_btnRemoveVariable_clicked()
{
    std::lock_guard<std::mutex> lock(_mutex);
    QModelIndexList rows = ui->tblVariables->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    ui->tblVariables->model()->removeRows(rows.at(0).row(), rows.size(), QModelIndex());
}
void MainWindow::on_btnStart_clicked()
{
    if (ui->btnStart->text()=="Start")
    {
        _isDrawing = _needGetParams = true;
        ui->btnAddDiff->setEnabled(false);
        ui->btnRemoveDiff->setEnabled(false);
        _thread = new std::thread( std::bind(&MainWindow::Draw, this) );
        ui->btnStart->setText("Stop");
    }
    else // "Stop"
    {
        if (_thread)
        {
            _isDrawing = false;
            _thread->detach();
            _thread = nullptr;
            ui->btnAddDiff->setEnabled(true);
            ui->btnRemoveDiff->setEnabled(true);
        }
        ui->btnStart->setText("Start");
    }
}
void MainWindow::AddVarDelegate(int row, const std::string& type)
{
    AddVarDelegate(row);//, Input::Type(type));
}
void MainWindow::AddVarDelegate(int row)
{
    VecStr vstr;// = {"unirand", "normrand"};
    vstr.push_back(Input::GAMMA_RAND_STR);
    vstr.push_back(Input::NORM_RAND_STR);
    vstr.push_back(Input::UNI_RAND_STR);
    ComboBoxDelegate* cbd = new ComboBoxDelegate(vstr);
    ui->tblVariables->setItemDelegateForRow(row, cbd);
    connect(cbd, SIGNAL(ComboBoxChanged(const QString&)), this, SLOT(ComboBoxChanged(const QString&)));
    _cmbDelegates.push_back(cbd);
}
void MainWindow::Draw()
{
    ui->qwtPlot->detachItems();
    ui->qwtInnerProduct->detachItems();

    //The point indicating current value
    QwtSymbol *symbol = new QwtSymbol( QwtSymbol::Ellipse,
        QBrush( Qt::yellow ), QPen( Qt::red, 2 ), QSize( 8, 8 ) );
    QwtPlotMarker* marker = new QwtPlotMarker();
    marker->setSymbol(symbol);
    marker->attach(ui->qwtPlot);

    //The 'tail' of the plot
    QwtPlotCurve* curve = new QwtPlotCurve();
    curve->setPen( Qt::black, 1 ),
    curve->setRenderHint( QwtPlotItem::RenderAntialiased, true );
    curve->attach(ui->qwtPlot);

    //The inner-product plot
    QwtPlotCurve* curve_ip = new QwtPlotCurve();
    curve_ip->setPen( Qt::black, 1 ),
    curve_ip->setRenderHint( QwtPlotItem::RenderAntialiased, true );
    curve_ip->attach(ui->qwtInnerProduct);

    //Get all of the information from the parameter fields, introducing new variables as needed.
    int num_pars = (int)_parameters->NumPars(),
            num_vars = (int)_variables->NumPars(),
            num_diffs = (int)_differentials->NumPars();//,
//            num_conds = _conditions->rowCount();
//    _parserMgr.AddParsers(num_conds);
    std::unique_ptr<double[]> pars( new double[num_pars] ),
                                vars( new double[num_vars] ), //This is purely for muParser
                                diffs( new double[num_diffs] );
//    std::vector<std::string> expressions, initializations;
        //variables, differential equations, and initial conditions, all of which can invoke named
        //values
    std::vector< std::deque<double> > pts(num_diffs);
    std::deque<double> ip;

    QFile temp(".temp.txt");
    temp.open(QFile::WriteOnly | QFile::Text);
/*
 *This function should be restricted entirely to pulling out the parameter info.  Doing the
 *parameter evaluation, and drawing the curves, should be done in other classes.  In particular
 *no instance of mu::Parser should appear here.
 */

//    std::vector< std::vector<double> > inputs;
//    int input_ct = 0;
    bool is_initialized = false;
    while (_isDrawing)
    {
        static auto last_step = std::chrono::system_clock::now();
        auto step_diff = std::chrono::system_clock::now() - last_step;
        auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(step_diff);
        bool need_new_step = diff_ms.count() > 1000.0 / (double)ui->spnStepsPerSec->value();
        if (!need_new_step)
            goto label;
        last_step = std::chrono::system_clock::now();
        {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_needGetParams)
        {
            //Initialize
//            num_pars = (int)_parameters->NumPars();
            num_vars = (int)_variables->NumPars();
            num_diffs = (int)_differentials->NumPars();
//            pars.reset( new double[num_pars] );
//            vars.reset( new double[num_vars] );
//            diffs.reset( new double[num_diffs] );
//            expressions.clear();

//            try
//            {
                _parserMgr.InitVars();
//                _parserMgr.AssignInputs();
                _parserMgr.InitParsers();


                //Differentials
/*                for (int i=0; i<num_diffs; ++i)
                {
                    const std::string& key = _differentials->Key(i).substr(0,1),
                            & value = _differentials->Value(i),
                            & init_value = _initConds->Value(i);

                    if (!is_initialized)
                        initializations.push_back(key + " = " + init_value);
                    expressions.push_back(key + " = " + key + " + " + value);
                }

                //Parameters
                for (int i=0; i<num_pars; ++i)
                {
                    const std::string& key = _parameters->Key(i),
                            & value = _parameters->Value(i);
                    pars[i] = atof(value.c_str());
                    _parser.DefineVar(key, &pars[i]);
                    for (auto& it : _parserConds)
                        it.DefineVar(key, &pars[i]);
                }
*/
                //Variables (includes input signals)
/*                for (int i=0; i<num_vars; ++i)
                {
                    const std::string& key = _variables->Key(i),
                            & value = _variables->Value(i);
                    _parser.DefineVar(key, &vars[i]); //So the expression which assigns a value
                        //to this variable must get called before the variable is used!
                    for (auto& it : _parserConds)
                        it.DefineVar(key, &vars[i]);

                    //Create input vector(s)
                    const ComboBoxDelegate* cbd = _cmbDelegates.at(i);
                    std::mt19937_64 mte;
                    switch (cbd->Type())
                    {
                        case ComboBoxDelegate::UNKNOWN:
                            throw("Unknown Variable Type");
                        case ComboBoxDelegate::UNI_RAND:
                        {
                            inputs.push_back( std::vector<double>(MAX_BUF_SIZE) );
                            std::uniform_real_distribution<double> uni_rand;
                            for (int k=0; k<MAX_BUF_SIZE; ++k)
                                inputs[i][k] = uni_rand(mte);
                            std::cerr << "UNI_RAND" << std::endl;
                            break;
                        }
                        case ComboBoxDelegate::GAMMA_RAND:
                        {
                            inputs.push_back( std::vector<double>(MAX_BUF_SIZE) );
                            std::gamma_distribution<double> gamma_rand;
                            for (int k=0; k<MAX_BUF_SIZE; ++k)
                                inputs[i][k] = gamma_rand(mte);
                            std::cerr << "GAMMA_RAND" << std::endl;
                            break;
                        }
                        case ComboBoxDelegate::NORM_RAND:
                        {
                            inputs.push_back( std::vector<double>(MAX_BUF_SIZE) );
                            std::normal_distribution<double> norm_rand;
                            for (int k=0; k<MAX_BUF_SIZE; ++k)
                                inputs[i][k] = norm_rand(mte);
                            std::cerr << "NORM_RAND" << std::endl;
                            break;
                        }
                        case ComboBoxDelegate::USER:
                            inputs.push_back( std::vector<double>() );
                            expressions.push_back(key + " = " + value);
                            std::cerr << "USER, " << value << std::endl;
                            break;
                    }
                }
*/
                //Differentials
/*                for (int i=0; i<num_diffs; ++i)
                {
                    const std::string& key = _differentials->Key(i).substr(0,1),
                            & value = _differentials->Value(i),
                            & init_value = _initConds->Value(i);
                    _parser.DefineVar(key, &diffs[i]); //So the expression which assigns a value
                        //to this variable must get called before the variable is used!
                    for (auto& it : _parserConds)
                        it.DefineVar(key, &diffs[i]);

                    if (!is_initialized) initializations.push_back(key + " = " + init_value);
                    expressions.push_back(key + " = " + key + " + " + value);
                }
            }
*//*            catch (mu::ParserError& e)
            {
                std::cout << e.GetMsg() << std::endl;
            }
*/        }

        //Update the state vector with the value of the differentials.
            //Number of iterations to calculate in this refresh
        int num_steps = (double)ui->spnStepsPerSec->value() / (double)SLEEP_MS + 0.5;
        if (num_steps==0) num_steps = 1;

            //Shrink the buffer if need be
        while (pts.at(0).size() + num_steps > MAX_BUF_SIZE)
            for (int i=0; i<num_diffs; ++i)
                pts[i].pop_front();
        while (ip.size() > IP_SAMPLES_SHOWN)
            ip.pop_front();

            //Go through each expression and evaluate them
        try
        {
            if (!is_initialized)
            {
                _parserMgr.InitParsers();
/*                for (const auto& it : initializations)
                {
                    _parserMgr.SetExpression(it);
                    _parserMgr.ParserEval();
//                    _parser.SetExpr(it);
//                    _parser.Eval();
                }
*/                is_initialized = true;
            }

            if (_needGetParams)
            {
//                std::string expr = expressions.front();
//                std::for_each(expressions.cbegin()+1, expressions.cend(),
//                              [&](const std::string& it)
//                {
//                    expr += ", " + it;
//                });
//                _parser.SetExpr(expr);
                _parserMgr.SetExpressions();//(expressions);
                _parserMgr.SetConditions();

//                for (int k=0; k<num_conds; ++k)
//                {
//                    std::string expr = _conditions->Condition(k);
//                    _parserConds[k].SetExpr(expr);
//                }
            }
            _needGetParams = false;

            std::string output;
            for (int k=0; k<num_steps; ++k)
            {
//                for (int i=0; i<num_vars; ++i)
//                    if (!inputs.at(i).empty()) //It's a variable
//                        vars[i] = inputs.at(i).at(input_ct);
//                ++input_ct;
//                input_ct %= MAX_BUF_SIZE;

                _parserMgr.ParserEval();
                _parserMgr.ParserCondEval();

                //Check conditions and update as need be
/*                for (int k=0; k<num_conds; ++k)
                {
                    if (_parserConds.at(k).Eval())
                    {
                        VecStr cond_exprns = _conditions->Expressions(k);
                        for (const auto& it : cond_exprns)
                        {
                            _parserConds[k].SetExpr(it);
                            _parserConds[k].Eval();
                        }
                        _parserConds[k].SetExpr( _conditions->Condition(k) );
                    }
                }
*/
                //Record updated variables for 2d graph, inner product, and output file
                double ip_k = 0;
                for (int i=0; i<num_diffs; ++i)
                {
                    pts[i].push_back(diffs[i]);
                    output += std::to_string(diffs[i]) + "\t";

                    ip_k += diffs[i] * diffs[i];
                }
                output += "\n";
                temp.write(output.c_str());
                temp.flush();

                ip.push_back(ip_k);
            }

        }
        catch (mu::ParserError& e)
        {
            std::cout << e.GetMsg() << std::endl;
            break;
        }

        //A blowup will crash QwtPlot
//        if (std::isinf(diffs[0]) || std::isinf(diffs[1]) || std::isnan(diffs[0]) || std::isnan(diffs[1]))
        const double DMAX = std::numeric_limits<double>::max();
        if (diffs[0]>DMAX || diffs[1]>DMAX)
        {
            on_btnStart_clicked();
            return;
        }

        //Plot the current state vector
        marker->setValue(diffs[0], diffs[1]);
        if (num_steps<100)
            std::cout << diffs[0] << ", " << diffs[1] << ", " << ip.back() << std::endl;
        int num_saved_pts = (int)pts[0].size(),
            tail_len = std::min(num_saved_pts, ui->spnTailLength->text().toInt());
        if (tail_len==-1) tail_len = num_saved_pts;
        QPolygonF points(tail_len);
        int ct_begin = std::max(0,num_saved_pts-tail_len);
        for (int k=0, ct=ct_begin; k<tail_len; ++k, ++ct)
            points[k] = QPointF(pts[0][ct], pts[1][ct]);
        curve->setSamples(points);

        //Plot points for the inner-product graph
        const int NUM_IP_POINTS = (int)ip.size();
        QPolygonF points_ip(NUM_IP_POINTS);
        for (int k=0; k<NUM_IP_POINTS; ++k)
            points_ip[k] = QPointF(k, ip[k]);
        curve_ip->setSamples(points_ip);

            //Get axis limits for inner-product plot
        auto xlims_ip = std::make_pair((const double)0, (const double)(ip.size()-1));
        auto ylims_ip = std::minmax_element(ip.cbegin(), ip.cend());
        ui->qwtInnerProduct->setAxisScale( QwtPlot::xBottom, xlims_ip.first, xlims_ip.second );
        ui->qwtInnerProduct->setAxisScale( QwtPlot::yLeft, *ylims_ip.first, *ylims_ip.second );

            //Get axis limits
        auto xlims = std::minmax_element(pts[0].cbegin(), pts[0].cend()),
                ylims = std::minmax_element(pts[1].cbegin(), pts[1].cend());
        ui->qwtPlot->setAxisScale( QwtPlot::xBottom, *xlims.first, *xlims.second );
        ui->qwtPlot->setAxisScale( QwtPlot::yLeft, *ylims.first, *ylims.second );

        emit DoReplot();
        }
        label:
        std::this_thread::sleep_for( std::chrono::milliseconds(SLEEP_MS) );
    }

    temp.close();
    _isDrawing = false;
}

void MainWindow::ComboBoxChanged(const QString& text)
{
    ComboBoxDelegate* cbd = qobject_cast<ComboBoxDelegate*>(sender());
    size_t row = std::find(_cmbDelegates.cbegin(), _cmbDelegates.cend(), cbd) - _cmbDelegates.cbegin();
    _parserMgr.AssignInput(_variables, row, text.toStdString());
}
void MainWindow::ParamsChanged(QModelIndex, QModelIndex) //slot
{
    _needGetParams = true;
}
void MainWindow::Replot() //slot
{
    ui->qwtPlot->replot();
    ui->qwtInnerProduct->replot();
}
