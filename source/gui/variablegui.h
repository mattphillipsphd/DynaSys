#ifndef VARIABLEGUI_H
#define VARIABLEGUI_H

#include <QWidget>

namespace Ui {
class VariableGui;
}

class VariableGui : public QWidget
{
    Q_OBJECT
    
public:
    explicit VariableGui(QWidget *parent = 0);
    ~VariableGui();
    
private:
    Ui::VariableGui *ui;
};

#endif // VARIABLEGUI_H
