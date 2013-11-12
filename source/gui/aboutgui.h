#ifndef ABOUTGUI_H
#define ABOUTGUI_H

#include <QWidget>

namespace Ui {
class AboutGui;
}

class AboutGui : public QWidget
{
    Q_OBJECT

    public:
        explicit AboutGui(QWidget *parent = 0);
        ~AboutGui();

    private:
        Ui::AboutGui *ui;
};

#endif // ABOUTGUI_H
