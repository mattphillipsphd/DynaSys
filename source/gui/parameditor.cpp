#include "parameditor.h"
#include "ui_parameditor.h"

const int ParamEditor::NUM_EDITORS = ds::NUM_MODELS + 1;

ParamEditor::ParamEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ParamEditor),
    _buffer(std::pair<int,std::string>(std::numeric_limits<int>::max(), "")),
    _editors(std::vector<PTextEdit*>(NUM_EDITORS))
{
    ui->setupUi(this);
    _editors[0] = ui->edAllPars;
    connect(_editors[0], SIGNAL(KeyRelease()), this, SLOT(TabTextChanged()));
    for (int i=1; i<NUM_EDITORS; ++i)
    {
        const ds::PMODEL mi = (ds::PMODEL)(i-1);
        PTextEdit* ed = new PTextEdit();
        _editors[i] = ed;
        ed->setObjectName(ds::Model(mi).c_str());
        connect(ed, SIGNAL(KeyRelease()), this, SLOT(TabTextChanged()));
        ui->tbwParameters->addTab(ed, ds::Model(mi).c_str());
    }

    connect(ui->btnCancel, SIGNAL(clicked()), this, SLOT(close()));
}

ParamEditor::~ParamEditor()
{
    delete ui;
    for (auto it : _editors) delete it;
}

void ParamEditor::UpdateParameters()
{
    setWindowTitle( ("Parameters for " + _fileName).c_str() );
    LoadModel();
}

void ParamEditor::on_btnKeep_Changes_clicked()
{
#ifdef DEBUG_FUNC
    qDebug() << "ParamEditor::on_btnKeep_Changes_clicked";
#endif
    VecStr models(ds::NUM_MODELS);
    for (int i=0; i<ds::NUM_MODELS; ++i)
    {
        std::string text = _models.at(i);
        text.erase(0, text.find_first_of('\n') + 1);
        TrimNewlines(text);
        if ( *(text.end()-2)=='\n' ) text.erase(text.end()-1);
        const ds::PMODEL mi = (ds::PMODEL)i;
        int num_pars = std::count(text.cbegin(), text.cend(), '\n');
        if (mi==ds::CONDITIONS)
        {
            const size_t len = text.length();
            if (len<=2)
                num_pars = 0;
            else
                for (size_t j=1; j<len; ++j)
                    if (text.at(j)=='\t' && text.at(j-1)=='\n') --num_pars;
        }
        text = ds::Model(mi) + "\t" + std::to_string(num_pars) + "\n" + text + "\n";
        models[i] = text;
    }

    emit ModelChanged(&models);
    close();
}
void ParamEditor::on_tbwParameters_currentChanged(int)
{
#ifdef DEBUG_FUNC
    qDebug() << "ParamEditor::on_tbwParameters_currentChanged";
#endif
    UpdateEditors();
}

void ParamEditor::closeEvent(QCloseEvent* event)
{
#ifdef DEBUG_FUNC
    qDebug() << "ParamEditor::closeEvent";
#endif
    emit CloseEditor();
    QWidget::closeEvent(event);
}
void ParamEditor::showEvent(QShowEvent* event)
{
#ifdef DEBUG_FUNC
    qDebug() << "ParamEditor::showEvent";
#endif
    UpdateParameters();
    QWidget::showEvent(event);
}

void ParamEditor::LoadModel()
{
#ifdef DEBUG_FUNC
    qDebug() << "ParamEditor::LoadModel";
#endif
    SysFileIn in(ds::TEMP_MODEL_FILE);
    _models.clear();
    in.Load(_models);
    UpdateEditors();
}

void ParamEditor::TabTextChanged()
{
#ifdef DEBUG_FUNC
    qDebug() << "ParamEditor::TabTextChanged";
#endif
    int idx = ui->tbwParameters->currentIndex();
    std::string text = _editors.at(idx)->document()->toPlainText().toStdString();
    TrimNewlines(text);
    _buffer = std::make_pair(idx,text);

    WriteBuffer();
}

void ParamEditor::TrimNewlines(std::string& text)
{
    while (*(text.end()-1) == '\n')
        text.erase( text.end()-1 );
    text += "\n\n";
}
void ParamEditor::UpdateEditors()
{
#ifdef DEBUG_FUNC
    qDebug() << "ParamEditor::UpdateEditors";
#endif
    for (auto it : _editors) it->clear();
    for (int i=1; i<NUM_EDITORS; ++i)
    {
        std::string text = _models.at(i-1);

         _editors[0]->appendPlainText(text.c_str());

        text.erase(0, text.find_first_of('\n') + 1);
        _editors[i]->appendPlainText(text.c_str());
    }
}
void ParamEditor::WriteBuffer()
{
#ifdef DEBUG_FUNC
    qDebug() << "ParamEditor::WriteBuffer";
#endif
    try
    {
        const int idx = _buffer.first;
        if (idx==std::numeric_limits<int>::max()) return;
        const std::string text = _buffer.second;
        if (idx==0)
        {
            size_t pos1 = text.find_first_of('#');
            int ct = 0;
            VecStr temp_models(ds::NUM_MODELS);
            while (pos1!=std::string::npos)
            {
                const size_t pos2 = text.find_first_of('#', pos1+1);
                std::string ptext = text.substr(pos1, pos2-pos1);
                TrimNewlines(ptext);
                temp_models[ct++] = ptext;
                if (ct==ds::NUM_MODELS) break;
                pos1 = pos2;
            }
            if (ct != ds::NUM_MODELS)
                return;
//                throw ("Bad Parameter file!  Changes not recorded.");
            _models = temp_models;
        }
        else
        {
            ds::PMODEL mi = (ds::PMODEL)(idx-1);
            _models[idx-1] = "#" + ds::Model(mi) + "\n" + text; //ed->toPlainText().toStdString();
        }
    }
    catch (std::exception& e)
    {
        qDebug() << e.what();
    }
}
