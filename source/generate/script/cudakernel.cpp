#include "cudakernel.h"

const std::string CudaKernel::IDX_SUF = "_IDX_";
const std::string CudaKernel::STATE_ARR = "ds_spec";

CudaKernel::CudaKernel(const std::string& name, MatlabBase::FILE_TYPE file_type)
    : MatlabBase(name, ".cu", file_type)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("CudaKernel::CudaKernel", std::this_thread::get_id());
#endif
}

std::string CudaKernel::ArrayEltReplace(const std::string& exprn) const
{
    VecStr all_elts = _allElts;
    std::sort(all_elts.begin(), all_elts.end(), [](const std::string& s1, const std::string& s2)
    {
        return s1.size() > s2.size();
    });
        //Sort the strings by size in descending order, so that no partial string
        //replacements will occur
    std::string raw_out, out = exprn;
    const size_t num_all_elts = all_elts.size();

    //First replace elements with temporary values that aren't possible parameter/variable
    //etc names
    for (size_t i=0; i<num_all_elts; ++i)
    {
        const std::string elt = all_elts.at(i);
        size_t pos=0,
                match_pos = out.find(elt, pos);
        while (match_pos != std::string::npos)
        {
            //Another step to avoid partial string replacements, arising from
            //the possible presence of tau in the string
            if ((match_pos>0 && std::isalnum(out[match_pos-1]))
                        || (match_pos<out.length()-elt.size()
                            && std::isalnum(out[match_pos+elt.size()])))
            {
                raw_out += out.substr(pos, match_pos-pos + elt.size());
                pos = match_pos + elt.size();
                match_pos = out.find(elt, pos);
                continue;
            }

            raw_out += out.substr(pos, match_pos-pos);
            raw_out += "@" + std::to_string(i) + "@";
            pos = match_pos + elt.size();
            match_pos = out.find(elt, pos);
        }
        raw_out += out.substr(pos);
        out = raw_out;
        raw_out.clear();
    }

    //Then replace the temporary values with the array element replacements.
    for (size_t i=0; i<num_all_elts; ++i)
    {
        const std::string elt = "@" + std::to_string(i) + "@";
        size_t pos=0, match_pos;
        while ((match_pos = out.find(elt, pos)) != std::string::npos)
        {
            raw_out += out.substr(pos, match_pos-pos);
            raw_out += ToArrayElt( all_elts.at(i) );
            pos = match_pos + elt.size();
        }
        raw_out += out.substr(pos);
        out = raw_out;
        raw_out.clear();
    }

    return out;
}

std::string CudaKernel::FuncArgs(ds::PMODEL, size_t) const
{
    return STATE_ARR;
}

std::string CudaKernel::ObjectiveFunc() const
{
    return "";
}

std::string CudaKernel::Suffix() const
{
    return "_c";
}

std::string CudaKernel::ToArrayElt(const std::string& var) const
{
    return STATE_ARR + "[" + var + IDX_SUF + "]";
}

void CudaKernel::WriteDataOut(std::ofstream& out, ds::PMODEL mi)
{
    out << "//Begin CudaKernel::WriteDataOut\n";
    const ParamModelBase* model = _modelMgr->Model(mi);
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
        out << "            out[ col++*num_records + row_ct ] = " + model->ShortKey(i) + ";\n";
    out << "//End CudaKernel::WriteDataOut\n";
    out << "    \n";
}
void CudaKernel::WriteFuncs(std::ofstream& out, ds::PMODEL mi)
{
    out << "//Begin CudaKernel::WriteFuncs\n";
    const ParamModelBase* model = _modelMgr->Model(mi);
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
    {
        std::string value = model->Value(i);
        if (Input::Type(value)==Input::INPUT_FILE) continue;
        out <<
               "__device__ void " + model->ShortKey(i) + "_func(double " + STATE_ARR + "[])\n"
               "{\n"
               "    " + ArrayEltReplace( PreprocessExprn( model->TempExprnForCFile(i) ) ) + ";\n"
               "}\n";
    }
    out << "//End CudaKernel::WriteFuncs\n";
    out << "\n";
}
void CudaKernel::WriteGlobalConst(std::ofstream& out)
{
    out << "//Begin CudaKernel::WriteGlobalConst\n";
    out << "__device__ const int INPUT_SIZE = " << Input::INPUT_SIZE << ";\n";
    out << "__device__ const double tau = " << _modelMgr->ModelStep() << ";\n";
    out << "//End CudaKernel::WriteGlobalConst\n";
    out << "\n";
}
void CudaKernel::WriteIncludes(std::ofstream& out)
{
    out << "//Begin CudaKernel::WriteIncludes\n";
    out << "#include \"math.h\"\n";
    out << "#include <cuda.h>\n";
    out << "#include <cuda_runtime.h>\n";
    out << "#include <stdio.h>\n";
    out << "//End CudaKernel::WriteIncludes\n";
    out << "\n";
}
void CudaKernel::WriteInitArgs(std::ofstream& out)
{
    out << "//Begin CudaKernel::WriteInitArgs\n";
    const ParamModelBase* inputs = _modelMgr->Model(ds::INP),
            * init_conds = _modelMgr->Model(ds::INIT);
    const size_t num_inputs = _modelMgr->Model(ds::INP)->NumPars(),
            num_ics = _modelMgr->Model(ds::INIT)->NumPars();

    out <<
           "    const int blockId = blockIdx.x + blockIdx.y * gridDim.x,\n"
           "            idx = blockId * (blockDim.x * blockDim.y) + (threadIdx.y * blockDim.x) + threadIdx.x;\n"
           "    if (idx>=num_tests) return;\n"
           "    const double* pars = par_mat + idx*num_pars;\n"
           "\n"
           "    double " + STATE_ARR + "[STATE_ARR_SIZE];\n"
           "    const int num_iters = (int)( pars[0] / tau + 0.5),\n"
           "            save_mod_n = (int)(pars[1]);\n"
           "\n";

    for (size_t i=0; i<num_inputs; ++i)
        out << "    " + ToArrayElt(inputs->Key(i)) + " = pars["
               + std::to_string(i+MFileBase::NUM_AUTO_ARGS) + "];\n";
    out << "\n";
    for (size_t i=0; i<num_ics; ++i)
        out << "    " + ToArrayElt(init_conds->ShortKey(i) + "0") +
               " = pars[" + std::to_string(i+num_inputs+MFileBase::NUM_AUTO_ARGS) + "];\n";
    out << "\n";

    for (const auto& it : _allElts)
        out << "    double& " + it + " = " + STATE_ARR + "[" + it + IDX_SUF + "];\n";

    out << "//End CudaKernel::WriteInitArgs\n";
    out << "\n";
}
void CudaKernel::WriteLoadInput(std::ofstream& out)
{
    out << "//Begin CudaKernel::WriteLoadInput\n";
    const ParamModelBase* variables = _modelMgr->Model(ds::VAR);
    const size_t num_vars = variables->NumPars();
    for (size_t i=0, ct=0; i<num_vars; ++i)
    {
        if (variables->IsFreeze(i)) continue;
        const std::string& value = variables->Value(i);
        if (Input::Type(value) != Input::INPUT_FILE) continue;

        const std::string var = variables->Key(i),
                inputv = "input_" + var,
                sputv = "sput_" + var, //samples per unit time
                samps_ct = "ct_" + var;
        if (ct!=0) out << "\n";
        const std::string sct = std::to_string(ct);
        out << "    const double* " + inputv + " = input + " + sct + "*input_len;\n";
        out << "    const int " + sputv + " = (int)(1.0/(tau * (double)sput[" + sct + "]) + 0.5);\n";
        out << "    int " + samps_ct + " = 0;\n";
        ++ct;
    }
    out << "//End CudaKernel::WriteLoadInput\n";
    out << "\n";
}
void CudaKernel::WriteMainBegin(std::ofstream& out)
{
    out <<
           "__global__ void Entry(\n"
           "        const double* input, const int input_len, const int* sput, \n"
           "        const double* par_mat, const int num_pars, \n"
           "        const int num_tests, double* out_mat)\n"
           "{\n";
}
void CudaKernel::WriteMainEnd(std::ofstream& out)
{
    out << "}\n";
}
void CudaKernel::WriteOutputHeader(std::ofstream& out)
{
    out << "//Begin CudaKernel::WriteOutputHeader\n";
    const size_t num_vars = _modelMgr->Model(ds::VAR)->NumPars(),
                num_diffs = _modelMgr->Model(ds::DIFF)->NumPars();
    std::string num_out = std::to_string(num_vars+num_diffs);
    out <<
           "    const int num_records = num_iters / save_mod_n;\n"
           "    int row_ct = 0;\n"
           "    double* out = out_mat + " + num_out + "*num_records*idx;\n";
    out << "//End CudaKernel::WriteOutputHeader\n";
    out << "\n";
}
void CudaKernel::WriteSaveBlockBegin(std::ofstream& out)
{
    out << "            int col = 0;\n";
    out << "\n";
}
void CudaKernel::WriteSaveBlockEnd(std::ofstream& out)
{
    out << "            ++row_ct;\n";
    out << "\n";
}
void CudaKernel::WriteVarDecls(std::ofstream& out)
{
    out << "//Begin CudaKernel::WriteVarDecls\n";
    int ct = 0;
    _allElts.clear();

    const ParamModelBase* init_conds = _modelMgr->Model(ds::INIT);
    const size_t num_ics = init_conds->NumPars();
    for (size_t i=0; i<num_ics; ++i)
    {
        out << "#define " + init_conds->ShortKey(i) + "0" + IDX_SUF
               + " " + std::to_string(ct++) + "\n";
        _allElts.push_back(init_conds->ShortKey(i) + "0");
    }
    out << "\n";

    const ParamModelBase* inputs = _modelMgr->Model(ds::INP);
    const size_t num_inputs = inputs->NumPars();
    for (size_t i=0; i<num_inputs; ++i)
    {
        out << "#define " + inputs->ShortKey(i) + IDX_SUF
               + " " + std::to_string(ct++) + "\n";
        _allElts.push_back(inputs->ShortKey(i));
    }
    out << "\n";

    const ParamModelBase* variables = _modelMgr->Model(ds::VAR);
    const size_t num_vars = variables->NumPars();
    for (size_t i=0; i<num_vars; ++i)
    {
        out << "#define " + variables->ShortKey(i) + IDX_SUF
               + " " + std::to_string(ct++) + "\n";
        _allElts.push_back(variables->ShortKey(i));
        if ( Input::Type(variables->Value(i)) == Input::USER && !variables->IsFreeze(i) )
        {
            out << "#define " + variables->TempKey(i) + IDX_SUF
                   + " " + std::to_string(ct++) + "\n";
            _allElts.push_back(variables->TempKey(i));
        }
    }
    out << "\n";

    const ParamModelBase* diffs = _modelMgr->Model(ds::DIFF);
    const size_t num_diffs = diffs->NumPars();
    for (size_t i=0; i<num_diffs; ++i)
    {
        out << "#define " + diffs->ShortKey(i) + IDX_SUF
               + " " + std::to_string(ct++) + "\n";
        _allElts.push_back(diffs->ShortKey(i));
        if (!diffs->IsFreeze(i))
        {
            out << "#define " + diffs->TempKey(i) + IDX_SUF
                   + " " + std::to_string(ct++) + "\n";
            _allElts.push_back(diffs->TempKey(i));
        }
    }
    out <<
           "\n"
           "#define STATE_ARR_SIZE " + std::to_string(ct) + "\n";
    out << "//End CudaKernel::WriteVarDecls\n";
    out << "\n";
}

