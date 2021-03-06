#ifndef NOTESGUI_H
#define NOTESGUI_H

#include <QWidget>

#include "../globals/globals.h"
#include "../memrep/modelmgr.h"
#include "../memrep/notes.h"

namespace Ui {
class NotesGui;
}

class NotesGui : public QWidget
{
    Q_OBJECT

    public:
        explicit NotesGui(QWidget *parent = 0);
        ~NotesGui();

        void UpdateNotes();

        void SetFileName(const std::string& file_name) { _fileName = file_name; }

    protected:
        virtual void showEvent(QShowEvent* event) override;

    signals:
        void SaveNotes();

    private slots:
        void on_btnSave_clicked();
        void on_btnSaveClose_clicked();

    private:
        Ui::NotesGui *ui;

        std::string _fileName;
        ModelMgr* const _modelMgr;
};

#endif // NOTESGUI_H
