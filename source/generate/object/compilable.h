#ifndef COMPILABLE_H
#define COMPILABLE_H

#include <QObject>

#include "../script/cfile.h"
#include "compilablebase.h"

class Compilable : public CompilableBase
{
    Q_OBJECT

    public:
        Compilable(const std::string& name);

        virtual void Compile() override;
};

#endif // COMPILABLE_H
