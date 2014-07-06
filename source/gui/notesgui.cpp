#include "notesgui.h"
#include "ui_notesgui.h"

NotesGui::NotesGui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NotesGui), _modelMgr(ModelMgr::Instance())
{
    ui->setupUi(this);
}

NotesGui::~NotesGui()
{
    delete ui;
}

void NotesGui::UpdateNotes()
{
    ui->txtNotes->document()->setPlainText( _modelMgr->GetNotes()->Text().c_str() );
}

void NotesGui::showEvent(QShowEvent* event)
{
    setWindowTitle( ("Notes for " + _fileName).c_str() );
    UpdateNotes();
    QWidget::showEvent(event);
}

void NotesGui::on_btnSave_clicked()
{
    _modelMgr->SetNotes( ui->txtNotes->document()->toPlainText().toStdString() );
    emit SaveNotes();
}

void NotesGui::on_btnSaveClose_clicked()
{
    _modelMgr->SetNotes( ui->txtNotes->document()->toPlainText().toStdString() );
    emit SaveNotes();
    close();
}
