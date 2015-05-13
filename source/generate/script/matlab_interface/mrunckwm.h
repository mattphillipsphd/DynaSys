#ifndef MRUNCKWM_H
#define MRUNCKWM_H

#include "mruncudakernel.h"

class MRunCKWM : public MRunCudaKernel
{
    public:
        MRunCKWM(const std::string& name);

    protected:
        virtual void WriteChunkSize(std::ofstream& out) const override;
        virtual void WriteCuCall(std::ofstream& out) const override;
        virtual void WriteDefaultPars(std::ofstream& out) const override;
        virtual void WriteDefsCall(std::ofstream& out) const override;
        virtual void WriteHeader(std::ofstream& out) const override;
};

#endif // MRUNCKWM_H
