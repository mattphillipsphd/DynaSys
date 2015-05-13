#ifndef PTEXTEDIT_H
#define PTEXTEDIT_H

#include <QPlainTextEdit>

class PTextEdit : public QPlainTextEdit
{
    Q_OBJECT
    public:
        explicit PTextEdit(QWidget *parent = 0);

    signals:
        void FocusOut();
        void KeyPress();
        void KeyRelease();

    public slots:

    protected slots:
        virtual void focusOutEvent(QFocusEvent *e) override;
        virtual void keyPressEvent(QKeyEvent *e) override;
        virtual void keyReleaseEvent(QKeyEvent *e) override;
};

#endif // PTEXTEDIT_H
