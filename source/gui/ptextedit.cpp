#include "ptextedit.h"

PTextEdit::PTextEdit(QWidget *parent) :
    QPlainTextEdit(parent)
{
    setFocusPolicy(Qt::StrongFocus);
}

void PTextEdit::focusOutEvent(QFocusEvent *e)
{
    emit FocusOut();
    QPlainTextEdit::focusOutEvent(e);
}

void PTextEdit::keyPressEvent(QKeyEvent *e)
{
    emit KeyPress();
    QPlainTextEdit::keyPressEvent(e);
}

void PTextEdit::keyReleaseEvent(QKeyEvent *e)
{
    emit KeyRelease();
    QPlainTextEdit::keyReleaseEvent(e);
}
