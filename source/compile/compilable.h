#ifndef COMPILABLE_H
#define COMPILABLE_H

#include <QObject>

#include "cfile.h"
#include "compilablebase.h"

class Compilable : public CompilableBase
{
    Q_OBJECT

    public:
        Compilable(const std::string& name);

        virtual void Compile(const ParserMgr& parser_mgr) override;
};

#endif // COMPILABLE_H
