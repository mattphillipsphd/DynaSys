#ifndef MRUNCUDAKERNEL_H
#define MRUNCUDAKERNEL_H

#include "mrunbase.h"

class MRunCudaKernel : public MRunBase
{
    public:
        MRunCudaKernel(const std::string& name);

    protected:
        virtual void Make(std::ofstream &out) const override;
        std::string NameCu() const;
        std::string NamePtx() const;
        virtual void WriteArgCheck(std::ofstream& out) const;
        virtual void WriteChunkSize(std::ofstream& out) const;
        virtual void WriteCuCall(std::ofstream& out) const;
        virtual void WriteDefaultPars(std::ofstream& out) const override;
        virtual void WriteDefsCall(std::ofstream& out) const;
        virtual void WriteHeader(std::ofstream& out) const;

    private:
};

#endif // MRUNCUDAKERNEL_H
