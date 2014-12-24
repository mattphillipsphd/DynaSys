#include "cudakernelwithmeasure.h"

const std::string CudaKernelWithMeasure::MAX_OBJ_VEC_LEN = "16";

CudaKernelWithMeasure::CudaKernelWithMeasure(const std::string& name, const std::string& obj_fun)
    : CudaKernel( AddSuffix(name) ), _hFileName( MakeHName(name) ), _objectiveFun(obj_fun)
{
}

std::string CudaKernelWithMeasure::ObjFunName() const
{
    std::string obj_fun = ds::StripPath(_objectiveFun);
    const size_t pos = obj_fun.find_last_of('.');
    obj_fun.erase(pos);
    return obj_fun;
}

void CudaKernelWithMeasure::MakeHFile()
{
    std::ofstream hout;
    hout.open(_hFileName);

    const ParamModelBase* variables = _modelMgr->Model(ds::VAR),
            * diffs = _modelMgr->Model(ds::DIFF);
    const size_t num_vars = variables->NumPars(),
            num_diffs = diffs->NumPars();

    hout << "//Auto-generated with DynaSys " + ds::VERSION_STR + "\n\n";

    for (size_t i=0; i<num_vars; ++i)
    {
        const std::string key = variables->ShortKey(i) + "_OUTIDX_";
        std::string idx;
        std::transform(key.cbegin(), key.cend(), std::back_inserter(idx), ::toupper);
        hout << "#define " + idx + " " + std::to_string(i) + "\n";
    }
    for (size_t i=0; i<num_diffs; ++i)
    {
        const std::string key = diffs->ShortKey(i) + "_OUTIDX_";
        std::string idx;
        std::transform(key.cbegin(), key.cend(), std::back_inserter(idx), ::toupper);
        hout << "#define " + idx + " " + std::to_string(i+num_vars) + "\n";
    }
}
std::string CudaKernelWithMeasure::NameMDefs() const
{
    std::string out(Name());
    size_t pos = out.find_last_of('.');
    if (pos!=std::string::npos) out.erase(pos);
    out.erase( out.find_last_of('_') ); //Get rid of _m here
    out += "_defs.m";
    return out;
}
std::string CudaKernelWithMeasure::NameMRun() const
{
    std::string out(Name());
    size_t pos = out.find_last_of('.');
    if (pos!=std::string::npos) out.erase(pos);
    out += "_cmrun.m";
    return out;
}

void CudaKernelWithMeasure::WriteCuCall(std::ofstream& out)
{
    out <<
           "num_intervals = fis(1).NumIntervals;\n"
           "out_mat = zeros(num_tests,1);\n"
           "input_data = zeros(length(fis(1).Data), num_fis);\n"
           "sput = zeros(num_fis,1);\n"
           "for i=1:num_fis\n"
           "    input_data(:,i) = fis(i).Data;\n"
           "    sput(i) = fis(i).Sput;\n"
           "end\n"
           "num_iters = num_records / tau;\n"
           "mipars = int32( zeros(6+length(user_mipars), 1) );\n"
           "mipars(1) = num_iters;\n"
           "mipars(2) = num_intervals;\n"
           "mipars(3) = num_iters / num_intervals;\n"
           "mipars(4) = 1.0 / tau;\n"
           "mipars(5) = tau * num_iters / num_intervals;\n"
           "mipars(6) = length(target);\n"
           "for i=1:length(user_mipars), mipars(6+i) = user_mipars(i); end\n"
           "mdpars = user_mdpars;\n"
           "input_len = length(fis(1).Data);\n"
           "%keyboard;\n"
           "data = gather( feval(k, ...\n"
           "    input_data, input_len, sput, ...\n"
           "    input_mat, num_inputs, num_tests, ...\n"
           "    target, fis(1).IntervalLen*ones(num_intervals,1), num_intervals, ...\n"
           "    mipars, mdpars, out_mat) );\n"
           "\n";
}


void CudaKernelWithMeasure::WriteDataOut(std::ofstream& out, ds::PMODEL mi)
{
    out << "//Begin CudaKernelWithMeasure::WriteDataOut\n";
    const ParamModelBase* model = _modelMgr->Model(mi);
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
        out << "            out[ col++ ] = " + model->ShortKey(i) + ";\n";
    out << "//End CudaKernelWithMeasure::WriteDataOut\n";
    out << "    \n";
}

void CudaKernelWithMeasure::WriteExtraFuncs(std::ofstream& out)
{
    std::ifstream ofun;
    ofun.open(_objectiveFun);

    out << "//Begin CudaKernelWithMeasure::WriteExtraFuncs\n";
    std::string line;
    std::getline(ofun, line);
    while (!ofun.eof())
    {
        if (!line.empty())
            out << line << "\n";
        std::getline(ofun, line);
    }
    out << "//End CudaKernelWithMeasure::WriteExtraFuncs\n";
    out << "\n";
}

void CudaKernelWithMeasure::WriteIncludes(std::ofstream &out)
{
    out << "//Begin CudaKernelWithMeasure::WriteIncludes\n";
    out <<
           "#include \"math.h\"\n"
           "#include \"" + _hFileName + "\"\n";
    out << "//End CudaKernelWithMeasure::WriteIncludes\n";
    out << "\n";
}

void CudaKernelWithMeasure::WriteMainBegin(std::ofstream& out)
{
    out <<
           "__global__ void Entry(\n"
           "        const double* input, const int input_len, const int* sput, \n"
           "        const double* par_mat, const int num_pars, const int num_tests,\n"
           "        const double* target, const int* int_lens, const int num_intervals,\n"
           "        const int* mipars,  const double* mdpars, double* out_mat)\n"
           "{\n";
}

void CudaKernelWithMeasure::WriteMainEnd(std::ofstream& out)
{
    out << "//Begin CudaKernelWithMeasure::WriteMainEnd\n";
    out <<
           "    out_mat[idx] =  MeasureRMSE(target, mipars, mdpars, &mstate); //RMSE\n"
           "    Delete" + ObjFunName() + "(&mstate);\n"
           "}\n";
    out << "//End CudaKernelWithMeasure::WriteMainEnd\n";
}

void CudaKernelWithMeasure::WriteMDefsCall(std::ofstream& out)
{
    std::string name_defs = ds::StripPath( NameMDefs() );
    name_defs.erase(name_defs.find_last_of('.'));
    out <<
           "[~, inputs, ~, is_par, ~, tau] = " + name_defs + ";\n"
           "\n";
}

void CudaKernelWithMeasure::WriteModelLoopBegin(std::ofstream& out)
{
    out << "//Begin CudaKernelWithMeasure::WriteModelLoopBegin\n";

    out <<
           "    MState mstate;\n"
           "    Init" + ObjFunName() + "(&mstate, mipars, mdpars);\n";

    out << "//End CudaKernelWithMeasure::WriteModelLoopBegin\n";
    out << "\n";
    CFileBase::WriteModelLoopBegin(out);
}

void CudaKernelWithMeasure::WriteMRunArgCheck(std::ofstream& out)
{
    CudaKernel::WriteMRunArgCheck(out);
    out <<
            "if ~exist('user_mipars', 'var'), user_mipars = []; end\n"
            "if ~exist('user_mdpars', 'var'), user_mdpars = []; end\n"
            "\n";
}

void CudaKernelWithMeasure::WriteMRunHeader(std::ofstream& out)
{
    std::string name_run = ds::StripPath( NameMRun() );
    name_run.erase(name_run.find_last_of('.'));
    out << "function [data, k] = " + name_run
           + "(num_records, save_mod_n, par_mat, ...\n"
           "            is_test, fis, target, user_mipars, user_mdpars, k)\n";
}

void CudaKernelWithMeasure::WriteOutputHeader(std::ofstream& out)
{
    out << "//Begin CudaKernelWithMeasure::WriteOutputHeader\n";
    const size_t num_vars = _modelMgr->Model(ds::VAR)->NumPars(),
                num_diffs = _modelMgr->Model(ds::DIFF)->NumPars();
    std::string num_out = std::to_string(num_vars+num_diffs);
    out << "    double out[" + num_out + "];\n";
    out << "//End CudaKernel::WriteOutputHeader\n";
    out << "\n";
}

void CudaKernelWithMeasure::WriteSaveBlockEnd(std::ofstream& out)
{
    out << "//Begin CudaKernelWithMeasure::WriteSaveBlockEnd\n";

    out << "            " + ObjFunName()
           + "(i, out, mipars, mdpars, &mstate);\n";
    out << "//End CudaKernelWithMeasure::WriteSaveBlockEnd\n";
}

std::string CudaKernelWithMeasure::AddSuffix(const std::string& name) const
{
    std::string out = name;
    size_t pos = out.find_last_of('.');
    if (pos==std::string::npos) out += "_m";
    else out.insert(pos, "_m");
    return out;
}
std::string CudaKernelWithMeasure::MakeHName(const std::string& name) const
{
    std::string out = name;
    size_t pos = out.find_last_of('.');
    if (pos!=std::string::npos) out.erase(pos);
    out += ".h";
    return out;
}
std::string CudaKernelWithMeasure::MakeObjFunName(const std::string& obj_fun) const
{
    std::string out(obj_fun);
    const size_t pos = out.find_last_of('.');
    if (pos==std::string::npos || out.substr(pos)!=".cu")
        throw std::runtime_error("CudaKernelWithMeasure::MakeObjFunName: Objective function must have .cu extension.");
    return out;
}
