#ifndef SHAREDOBJ_H
#define SHAREDOBJ_H

#include "cfileso.h"
#include "compilablebase.h"

class SharedObj : public CompilableBase
{
    public:
        SharedObj(const std::string& name);

        virtual void Compile(const ParserMgr &parser_mgr) override;

    private:
        const std::string _nameO;
};

#endif // SHAREDOBJ_H
