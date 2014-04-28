#ifndef PARAMEDITOR_H
#define PARAMEDITOR_H

#include <QPlainTextEdit>
#include <QWidget>

#include "ptextedit.h"
#include "../file/sysfilein.h"
#include "../file/sysfileout.h"
#include "../memrep/parsermgr.h"

namespace Ui {
class ParamEditor;
}

class ParamEditor : public QWidget
{
    Q_OBJECT

    public:
        explicit ParamEditor(QWidget *parent = 0);
        ~ParamEditor();

        void UpdateParameters();

        void SetFileName(const std::string& file_name) { _fileName = file_name; }

    public slots:
        void on_btnKeep_Changes_clicked();
        void on_tbwParameters_currentChanged(int);

    protected slots:
        virtual void closeEvent(QCloseEvent* event) override;
        virtual void showEvent(QShowEvent* event) override;

    signals:
        void CloseEditor();
        void ModelChanged(void*);

    private slots:
        void TabTextChanged();

    private:
        void TrimNewlines(std::string& text);
        void UpdateEditors();
        void WriteBuffer();

        static const int NUM_EDITORS;

        Ui::ParamEditor *ui;
        
        void LoadModel();

        std::pair<int, std::string> _buffer;
        std::vector<PTextEdit*> _editors;
        std::string _fileName;
        VecStr _models;
};

#endif // PARAMEDITOR_H
