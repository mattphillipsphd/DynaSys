#include "parameditor.h"
#include "ui_parameditor.h"

const int ParamEditor::NUM_EDITORS = ds::NUM_MODELS + 1;

ParamEditor::ParamEditor(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ParamEditor),
    _buffer(std::pair<int,std::string>(std::numeric_limits<int>::max(), "")),
    _editors(std::vector<PTextEdit*>(NUM_EDITORS)),
    _log(Log::Instance()), _tid(std::this_thread::get_id())
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamEditor::ParamEditor", _tid);
#endif
    ui->setupUi(this);
    _editors[0] = ui->edAllPars;
    connect(_editors[0], SIGNAL(KeyRelease()), this, SLOT(TabTextChanged()));
    for (int i=1; i<NUM_EDITORS; ++i)
    {
        const ds::PMODEL mi = (ds::PMODEL)(i-1);
        PTextEdit* ed = new PTextEdit();
        QFont font = ed->font();
        font.setPointSize(12);
        ed->setFont(font);
        _editors[i] = ed;
        ed->setObjectName(ds::Model(mi).c_str());
        connect(ed, SIGNAL(KeyRelease()), this, SLOT(TabTextChanged()));
        ui->tbwParameters->addTab(ed, ds::Model(mi).c_str());
    }

    connect(ui->btnCancel, SIGNAL(clicked()), this, SLOT(close()));
}

ParamEditor::~ParamEditor()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamEditor::~ParamEditor", _tid);
#endif
    delete ui;
    for (auto it : _editors) delete it;
}

void ParamEditor::UpdateParameters()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamEditor::UpdateParameters", _tid);
#endif
    setWindowTitle( ("Parameters for " + _fileName).c_str() );
    LoadModel();
}

void ParamEditor::on_btnSave_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamEditor::on_btnSave_clicked", _tid);
#endif
/*    int current_idx = ui->tbwParameters->currentIndex();
    for (int i=1; i<ds::NUM_MODELS; ++i)
    {
        UpdateBuffer(i);
        WriteBuffer();
    }
    UpdateBuffer(current_idx);
*/
    VecStr models(ds::NUM_MODELS);
    for (int i=0; i<ds::NUM_MODELS; ++i)
    {
        std::string text = _models.at(i);
        text.erase(0, text.find_first_of('\n') + 1);
        TrimNewlines(text);
        if (text.size()<3) text = "";
        else if ( *(text.end()-2)=='\n' ) text.erase(text.end()-1);
        const ds::PMODEL mi = (ds::PMODEL)i;
        int num_pars = std::count(text.cbegin(), text.cend(), '\n');
        text = ds::Model(mi) + "\t" + std::to_string(num_pars) + "\n" + text + "\n";
        models[i] = text;
    }

    emit ModelChanged(&models);
}
void ParamEditor::on_btnSave_Close_clicked()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamEditor::on_btnSave_Close_clicked", _tid);
#endif
    on_btnSave_clicked();
    close();
}
void ParamEditor::on_tbwParameters_currentChanged(int)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamEditor::on_tbwParameters_currentChanged", _tid);
#endif
    UpdateEditors();
}

void ParamEditor::closeEvent(QCloseEvent* event)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamEditor::closeEvent", _tid);
#endif
    emit CloseEditor();
    QWidget::closeEvent(event);
}
void ParamEditor::showEvent(QShowEvent* event)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamEditor::showEvent", _tid);
#endif
    UpdateParameters();
    QWidget::showEvent(event);
}

void ParamEditor::LoadModel()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamEditor::LoadModel", _tid);
#endif
    SysFileIn in(ds::TEMP_MODEL_FILE);
    _models.clear();
    in.Load(_models);
    UpdateEditors();
}

void ParamEditor::TabTextChanged()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamEditor::TabTextChanged", _tid);
#endif
    int idx = ui->tbwParameters->currentIndex();
    UpdateBuffer(idx);
    WriteBuffer();
}

void ParamEditor::TrimNewlines(std::string& text)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamEditor::TrimNewlines", _tid);
#endif
    size_t pos = std::find_if(text.begin(), text.end(), static_cast<int(*)(int)>(&std::isalnum))
            - text.begin();
    text.erase(0,pos);
    if (!text.empty())
        while (*(text.end()-1) == '\n')
            text.erase( text.end()-1 );
    text += "\n\n";
}
void ParamEditor::UpdateBuffer(int idx)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamEditor::UpdateBuffer", _tid);
#endif
    std::string text = _editors.at(idx)->document()->toPlainText().toStdString();
    TrimNewlines(text);
    _buffer = std::make_pair(idx,text);
}
void ParamEditor::UpdateEditors()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamEditor::UpdateEditors", _tid);
#endif
    for (auto it : _editors) it->clear();
    for (int i=1; i<NUM_EDITORS; ++i)
    {
        int mi = i-1;
        std::string text = _models.at(mi);
        if ((ds::PMODEL)(mi)==ds::FUNC || (ds::PMODEL)(mi)==ds::STATE)
        {
            std::stringstream ss(text);
            std::string line, text_temp;
            std::getline(ss, line);
            while (!line.empty())
            {
                size_t pos1 = line.find_first_of('\t'),
                        pos2 = line.find_first_of('\t', pos1+1);
                if (pos2 != std::string::npos)
                    line.erase(pos2);
                line += "\n";
                text_temp += line;
                std::getline(ss, line);
            }
            text = text_temp;
        }

        _editors[0]->appendPlainText(text.c_str());

        text.erase(0, text.find_first_of('\n') + 1);
        _editors[i]->appendPlainText(text.c_str());
    }
}
void ParamEditor::WriteBuffer()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("ParamEditor::WriteBuffer", _tid);
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
//                throw std::runtime_error("ParamEditor::WriteBuffer: Bad Parameter file!  Changes not recorded.");
            _models = temp_models;
        }
        else
        {
            ds::PMODEL mi = (ds::PMODEL)(idx-1);
            _models[idx-1] = "#" + ds::Model(mi) + "\n" + text;
        }
    }
    catch (std::exception& e)
    {
        _log->AddExcept("ParamEditor::WriteBuffer, " + std::string(e.what()));
    }
}
