#include "cudakernel.h"

const int CudaKernel::NUM_AUTO_ARGS = 2;

CudaKernel::CudaKernel(const std::string& name) : CFileBase(name), _inputCt(0)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("CudaKernel::CudaKernel", std::this_thread::get_id());
#endif
    ResetNameSuffix(".cu");
}

void CudaKernel::WriteDataOut(std::ofstream& out, const ParamModelBase* model)
{
    out << "//Begin CudaKernel::WriteDataOut\n";
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
        out << "            out[ col++*num_records + row_ct ] = " + model->ShortKey(i) + ";\n";
    out << "//End CudaKernel::WriteDataOut\n";
    out << "    \n";
}
void CudaKernel::WriteFuncs(std::ofstream& out, const ParamModelBase* model)
{
    out << "//Begin CudaKernel::WriteFuncs\n";
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
    {
        std::string value = model->Value(i);
        if (Input::Type(value)==Input::INPUT_FILE) continue;
        out <<
               "__device__ void " + model->ShortKey(i) + "_func()\n"
               "{\n"
               "    " + PreprocessExprn( model->TempExpression(i) ) + ";\n"
               "}\n";
    }
    out << "//End CudaKernel::WriteFuncs\n";
    out << "\n";
}
void CudaKernel::WriteGlobalConst(std::ofstream& out, const ParserMgr& parser_mgr)
{
    out << "//Begin CudaKernel::WriteGlobalConst\n";
    out << "__device__ const int INPUT_SIZE = " << Input::INPUT_SIZE << ";\n";
    out << "__device__ const double tau = " << parser_mgr.ModelStep() << ";\n";
    out << "//End CudaKernel::WriteGlobalConst\n";
    out << "\n";
}
void CudaKernel::WriteIncludes(std::ofstream& out)
{
    out << "//Begin CudaKernel::WriteIncludes\n";
    out <<
           "#include \"math.h\"\n";
    out << "//End CudaKernel::WriteIncludes\n";
    out << "\n";
}
void CudaKernel::WriteInitArgs(std::ofstream& out, const ParamModelBase* inputs,
                   const ParamModelBase* init_conds)
{
    out << "//Begin CudaKernel::WriteInitArgs\n";
    const size_t num_inputs = inputs->NumPars(),
            num_ics = init_conds->NumPars();

    out <<
           "    const int num_iters = (int)( *pars[0] / tau ),\n"
           "            save_mod_n = (int)(*pars[1]);\n"
           "\n";

    for (size_t i=0; i<num_inputs; ++i)
        out << "    " + inputs->Key(i) + " = *pars["
               + std::to_string(i+NUM_AUTO_ARGS) + "];\n";
    out << "\n";
    for (size_t i=0; i<num_ics; ++i)
        out << "    " + init_conds->ShortKey(i) + "0 = *pars["
               + std::to_string(i+num_inputs+NUM_AUTO_ARGS) + "];\n";
    out << "//End CudaKernel::WriteInitArgs\n";
    out << "\n";

    _inputCt = num_inputs+num_ics+NUM_AUTO_ARGS; // ### Feels like a hack,
        //just make const parser_mgr& a member?
}
void CudaKernel::WriteLoadInput(std::ofstream& out, const ParamModelBase* variables)
{
    out << "//Begin CudaKernel::WriteLoadInput\n";
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
        if (ct++!=0) out << "\n";
        out << "    const double* " + inputv + " = pars[" + std::to_string(_inputCt++) + "];\n";
        out << "    int " + sputv + " = (int)*pars[" + std::to_string(_inputCt++) + "];\n";
        out <<
               "    int " + samps_ct + " = 0;\n"
               "    " + sputv + " = (int)(1.0/(tau * (double)" + sputv + ") + 0.5);\n";
    }
    out << "//End CudaKernel::WriteLoadInput\n";
    out << "\n";
}
void CudaKernel::WriteMainBegin(std::ofstream& out)
{
    out <<
           "__global__ void Entry(const double* pars[], int num_pars, double* out)\n"
           "{\n";
}
void CudaKernel::WriteMainEnd(std::ofstream& out)
{
    out << "}\n";
}
void CudaKernel::WriteOutputHeader(std::ofstream& out, const ParamModelBase* variables,
                                   const ParamModelBase* diffs)
{
    out << "//Begin CudaKernel::WriteOutputHeader\n";
    const size_t num_fields = variables->NumPars()+diffs->NumPars();
    out <<
           "    const int num_records = num_iters / save_mod_n;\n"
           "    int row_ct = 0;\n";
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
void CudaKernel::WriteVarDecls(std::ofstream& out, const ParserMgr& parser_mgr)
{
    out << "//Begin CudaKernel::WriteVarDecls\n";
    const ParamModelBase* init_conds = parser_mgr.Model(ds::INIT_CONDS);
    const size_t num_ics = init_conds->NumPars();
    for (size_t i=0; i<num_ics; ++i)
        out << "__device__ double " + init_conds->ShortKey(i) + "0;\n";
    out << "\n";

    const ParamModelBase* inputs = parser_mgr.Model(ds::INPUTS);
    const size_t num_inputs = inputs->NumPars();
    for (size_t i=0; i<num_inputs; ++i)
        out << "__device__ double " + inputs->ShortKey(i) + ";\n";
    out << "\n";

    const ParamModelBase* variables = parser_mgr.Model(ds::VARIABLES);
    const size_t num_vars = variables->NumPars();
    for (size_t i=0; i<num_vars; ++i)
    {
        out << "__device__ double " + variables->ShortKey(i) + ";\n";
        if ( Input::Type(variables->Value(i)) == Input::USER && !variables->IsFreeze(i) )
            out << "__device__ double " + variables->TempKey(i) + ";\n";
    }
    out << "\n";

    const ParamModelBase* diffs = parser_mgr.Model(ds::DIFFERENTIALS);
    const size_t num_diffs = diffs->NumPars();
    for (size_t i=0; i<num_diffs; ++i)
    {
        out << "__device__ double " + diffs->ShortKey(i) + ";\n";
        if (!diffs->IsFreeze(i))
            out << "__device__ double " + diffs->TempKey(i) + ";\n";
    }
    out << "//End CudaKernel::WriteVarDecls\n";
    out << "\n";
}

/*std::string CudaKernel::MakeName(const std::string& name) const
{
    std::string out(name);
    const size_t len = out.length();
    if (out.substr(len-3) != ".cu") out += ".cu";
    return out;
}
*/
