#ifndef MRUNMEXWM_H
#define MRUNMEXWM_H

#include "mrunmex.h"

class MRunMEXWM : public MRunMEX
{
    public:
        MRunMEXWM(const std::string& name);

        virtual void Make(std::ofstream &out) const override;
    protected:
};

#endif // MRUNMEXWM_H
