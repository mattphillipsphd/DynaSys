#ifndef MRUNBASE_H
#define MRUNBASE_H

#include "mfilebase.h"

class MRunBase : public MFileBase
{
    public:
        MRunBase(const std::string& name);

    protected:
        virtual std::string Name() const override final;
        virtual void WriteDefaultPars(std::ofstream& out) const;
};

#endif // MRUNBASE_H
