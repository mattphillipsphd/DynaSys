#ifndef SHAREDOBJ_H
#define SHAREDOBJ_H

#include "../script/cfileso.h"
#include "compilablebase.h"

class SharedObj : public CompilableBase
{
    public:
        SharedObj(const std::string& name);

        virtual void Compile() override;

    private:
        const std::string _nameO;
};

#endif // SHAREDOBJ_H
