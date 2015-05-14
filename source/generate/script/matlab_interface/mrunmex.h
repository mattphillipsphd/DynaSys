#ifndef MRUNMEX_H
#define MRUNMEX_H

#include "mrunbase.h"

class MRunMEX : public MRunBase
{
    public:
        MRunMEX(const std::string& name);

    protected:
        virtual void Make(std::ofstream& out) const override;

};

#endif // MRUNMEX_H
