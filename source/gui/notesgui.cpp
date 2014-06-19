#include "notesgui.h"
#include "ui_notesgui.h"

NotesGui::NotesGui(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::NotesGui), _notes(new Notes())
{
    ui->setupUi(this);
}

NotesGui::~NotesGui()
{
    delete ui;
}

void NotesGui::UpdateNotes()
{
    ui->txtNotes->document()->setPlainText( _notes->Text().c_str() );
}

void NotesGui::showEvent(QShowEvent* event)
{
    setWindowTitle( ("Notes for " + _fileName).c_str() );
    UpdateNotes();
    QWidget::showEvent(event);
}

void NotesGui::on_btnSave_clicked()
{
    _notes->SetText(
                ui->txtNotes->document()->toPlainText().toStdString() );
    emit SaveNotes();
}

void NotesGui::on_btnSaveClose_clicked()
{
    _notes->SetText(
                ui->txtNotes->document()->toPlainText().toStdString() );
    emit SaveNotes();
    close();
}
