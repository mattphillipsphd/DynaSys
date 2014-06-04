#include "cfilebase.h"

CFileBase::CFileBase(const std::string& name) : _log(Log::Instance()), _name(MakeName(name))
{
}

void CFileBase::Make(const ParserMgr& parser_mgr)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("Executable::Make", std::this_thread::get_id());
#endif
    const ParamModelBase* inputs = parser_mgr.Model(ds::INPUTS),
            * variables = parser_mgr.Model(ds::VARIABLES),
            * diffs = parser_mgr.Model(ds::DIFFERENTIALS),
            * init_conds = parser_mgr.Model(ds::INIT_CONDS);

    std::ofstream out;
    out.open(_name);

    WriteIncludes(out);
    WriteGlobalConst(out, parser_mgr);
    WriteVarDecls(out, parser_mgr);
    WriteFuncs(out, variables);
    WriteFuncs(out, diffs);

    //******************Write entry to main function******************
    WriteMainBegin(out);

    //Initialize variables from arguments, e.g. from argv
    WriteInitArgs(out, inputs, init_conds);

    //Load all input files
    if (static_cast<const VariableModel*>(variables)->TypeCount(Input::INPUT_FILE)>0)
        WriteLoadInput(out, variables);

    //Write header information to output destination
    WriteOutputHeader(out, variables, diffs);

    //Initialize variables and differentials
    WriteInitVarsDiffs(out, variables, diffs);

    //***********Begin main model loop
    out <<
           "    for (int i=0; i<num_iters; ++i)\n"
           "    {\n";

        //  Execute variable and differential functions
        WriteExecVarsDiffs(out, variables, diffs);

    //  *****Begin check whether to save data
    out <<
           "    \n"
           "        if (i%save_mod_n==0)\n"
           "        {\n";

            WriteSaveBlockBegin(out);
            WriteDataOut(out, variables);
            WriteDataOut(out, diffs);
            WriteSaveBlockEnd(out);

    out << "        }\n";
    //  *****End check whether to save data

        //  Check whether any conditions apply
        if (parser_mgr.Conditions()->NumPars()>0)
            WriteConditions(out, parser_mgr.Conditions());

    out << "    }\n"
           "    \n";
    //***********End main model loop

    WriteMainEnd(out);

    out.close();
}

std::string CFileBase::PreprocessExprn(const std::string& exprn) const
{
//#ifdef DEBUG_FUNC
//    ScopeTracker st("CFileBase::PreprocessExprn", std::this_thread::get_id());
//#endif
    std::string temp = exprn; //Gets progressively processed until it's proper C.

    ds::RemoveWhitespace(temp);

    const std::string OPS = "<>?:!%&|~=+-*/";
    size_t posc = temp.find_first_of('^');
    while (posc != std::string::npos)
    {
        const size_t len = temp.length();
        std::string base = temp.substr(0, posc);
        size_t base_begin = 0;
        int paren = 0;
        for (size_t i=posc-1; i<std::numeric_limits<size_t>::max(); --i)
        {
            char c = temp.at(i);
            if (paren==0 && OPS.find_first_of(c)!=std::string::npos)
            {
                base = temp.substr(i+1,posc-i-1);
                base_begin = i+1;
                break;
            }
            else if (c==')')
                ++paren;
            else if (c=='(')
            {
                --paren;
                if (paren==0)
                {
                    base = temp.substr(i,posc-i);
                    base_begin = i;
                    break;
                }
            }
        }

        std::string expt = temp.substr(posc+1);
        size_t expt_end = len;
        paren = 0;
        for (size_t i=posc+1; i<len; ++i)
        {
            char c = temp.at(i);
            if (paren==0 && OPS.find_first_of(c)!=std::string::npos)
            {
                if (c=='-' && i==posc+1) continue; //unary minus
                expt = temp.substr(posc+1,i-posc);
                expt_end = i+1;
                break;
            }
            else if (c=='(')
                ++paren;
            else if (c==')')
            {
                --paren;
                if (paren==0)
                {
                    expt = temp.substr(posc+1,i-posc);
                    expt_end = i+1;
                    break;
                }
            }
        }

        temp.replace(base_begin, expt_end-base_begin, "exp(" + base + "," + expt + ")");
        posc = temp.find_first_of('^');
    }
    return temp;
}

void CFileBase::WriteConditions(std::ofstream& out, const ConditionModel* conditions)
{
    out << "//Begin WriteConditions\n";
    const size_t num_conds = conditions->NumPars();
    for (size_t i=0; i<num_conds; ++i)
    {
        out <<
               "        if (" + conditions->Condition((int)i) + ")\n"
               "        {\n";

        const VecStr expressions = conditions->Expressions((int)i);
        for (const auto& it : expressions)
            out << "            " + it + ";\n";

        out << "        }\n";
    }
    out << "//End WriteConditions\n";
}

void CFileBase::WriteExecVarsDiffs(std::ofstream& out, const ParamModelBase* variables,
                                    const ParamModelBase* diffs)
{
    out << "//Begin WriteExecVarsDiffs\n";
    const size_t num_vars = variables->NumPars(),
            num_diffs = diffs->NumPars();
    for (size_t i=0; i<num_vars; ++i)
    {
        if (variables->IsFreeze(i)) continue;
        std::string value = variables->Value(i);
        if (Input::Type(value)==Input::USER)
            out << "        " + variables->ShortKey(i) + "_func();\n";
        else
        {
            std::string var = variables->ShortKey(i),
                    inputv = "input_" + var,
                    spsv = "sps_" + var, //samples per step
                    samps_ct = "ct_" + var;
            out <<
                   "        if (i % " + spsv + " == 0)\n"
                   "            " + var + " = " + inputv + "[++" + samps_ct + "];\n"
                   "        \n";
        }
    }
    for (size_t i=0; i<num_vars; ++i)
        if (Input::Type(variables->Value(i))==Input::USER && !variables->IsFreeze(i))
            out << "        " + variables->ShortKey(i) + " = " + variables->TempKey(i) + ";\n";
    out << "\n";
    for (size_t i=0; i<num_diffs; ++i)
        if (!diffs->IsFreeze(i))
            out << "        " + diffs->ShortKey(i) + "_func();\n";
    for (size_t i=0; i<num_diffs; ++i)
        if (!diffs->IsFreeze(i))
            out << "        " + diffs->ShortKey(i) + " = " + diffs->TempKey(i) + ";\n";
    out << "//End WriteExecVarsDiffs\n";
}

void CFileBase::WriteFuncs(std::ofstream& out, const ParamModelBase* model)
{
    out << "//Begin WriteFuncs\n";
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
    {
        std::string value = model->Value(i);
        if (Input::Type(value)==Input::INPUT_FILE) continue;
        out <<
               "inline void " + model->ShortKey(i) + "_func()\n"
               "{\n"
               "    " + PreprocessExprn( model->TempExpression(i) ) + ";\n"
               "}\n";
    }
    out << "//End WriteFuncs\n";
    out << "\n";
}

void CFileBase::WriteGlobalConst(std::ofstream& out, const ParserMgr& parser_mgr)
{
    out << "//Begin WriteGlobalConst\n";
    out << "const int INPUT_SIZE = " << Input::INPUT_SIZE << ";\n";
    out << "const double tau = " << parser_mgr.ModelStep() << ";\n";
    out << "//End WriteGlobalConst\n";
    out << "\n";
}

void CFileBase::WriteInitVarsDiffs(std::ofstream& out, const ParamModelBase* variables,
                        const ParamModelBase* diffs)
{
    out << "//Begin WriteInitVarsDiffs\n";
    const size_t num_vars = variables->NumPars(),
            num_diffs = diffs->NumPars();
    for (size_t i=0; i<num_vars; ++i)
    {
        std::string value = variables->Value(i);
        if (Input::Type(value)==Input::USER || variables->IsFreeze(i))
            out << "    " + variables->ShortKey(i) + " = 0;\n";
        else
        {
            std::string inputv = "input_" + variables->ShortKey(i);
            out << "    " + variables->ShortKey(i) + " = " + inputv + "[0];\n";
        }
    }
    for (size_t i=0; i<num_diffs; ++i)
        out << "    " + diffs->ShortKey(i) + " = " + diffs->ShortKey(i) + "0;\n";
    out << "//End WriteInitVarsDiffs\n";
    out << "\n";
}

void CFileBase::WriteLoadInput(std::ofstream& out, const ParamModelBase* variables)
{
    out << "//Begin WriteLoadInput\n";
    out << "    size_t br;\n";
    out << "    int num_elts;\n";
    const size_t num_vars = variables->NumPars();
    for (size_t i=0; i<num_vars; ++i)
    {
        if (variables->IsFreeze(i)) continue;
        const std::string& value = variables->Value(i);
        if (Input::Type(value) != Input::INPUT_FILE) continue;
        QFileInfo f( ds::StripQuotes(value).c_str() );
        const std::string value_abs = f.canonicalFilePath().toStdString();
        if (value_abs.empty())
            throw std::runtime_error("CFileBase::WriteLoadInput: Bad file name.");

        const std::string var = variables->Key(i),
                fpv = "fp_" + var,
                inputv = "input_" + var,
                spsv = "sps_" + var, //samples per step
                samps_ct = "ct_" + var;
        if (i!=0) out << "\n";
        out <<
               "    FILE* " + fpv + " = fopen(\"" + value_abs + "\", \"rb\");\n"
               "    if (" + fpv + "==0) return 1;\n"
               "    int vnum_" + var + ";\n"
               "    br = fread(&vnum_" + var + ", sizeof(int), 1, " + fpv + ");\n"
               "    int " + spsv + ", " + samps_ct + " = 0;\n"
               "    br = fread(&" + spsv + ", sizeof(int), 1, " + fpv + ");\n"
               "    " + spsv + " = (int)(1.0/(tau * (double)" + spsv + ") + 0.5);\n"
               "    br = fread(&num_elts, sizeof(int), 1, " + fpv + ");\n"
               "    double* " + inputv + " = (double*)malloc(INPUT_SIZE*sizeof(double));\n"
#ifdef Q_OS_WIN
               "    br = fread(" + inputv
                        + ", sizeof(double), (int)std::min(INPUT_SIZE, num_elts), " + fpv + ");\n"
#else
               "    br = fread(" + inputv
                        + ", sizeof(double), (int)fmin(INPUT_SIZE, num_elts), " + fpv + ");\n"
#endif
               "    fclose(" + fpv + ");\n";
    }
    out << "//End WriteLoadInput\n";
    out << "\n";
}

void CFileBase::WriteVarDecls(std::ofstream& out, const ParserMgr& parser_mgr)
{
    out << "//Begin WriteVarDecls\n";
    const ParamModelBase* init_conds = parser_mgr.Model(ds::INIT_CONDS);
    const size_t num_ics = init_conds->NumPars();
    for (size_t i=0; i<num_ics; ++i)
        out << "double " + init_conds->ShortKey(i) + "0;\n";
    out << "\n";

    const ParamModelBase* inputs = parser_mgr.Model(ds::INPUTS);
    const size_t num_inputs = inputs->NumPars();
    for (size_t i=0; i<num_inputs; ++i)
        out << "double " + inputs->ShortKey(i) + ";\n";
    out << "\n";

    const ParamModelBase* variables = parser_mgr.Model(ds::VARIABLES);
    const size_t num_vars = variables->NumPars();
    for (size_t i=0; i<num_vars; ++i)
    {
        out << "double " + variables->ShortKey(i) + ";\n";
        if ( Input::Type(variables->Value(i)) == Input::USER && !variables->IsFreeze(i) )
            out << "double " + variables->TempKey(i) + ";\n";
    }
    out << "\n";

    const ParamModelBase* diffs = parser_mgr.Model(ds::DIFFERENTIALS);
    const size_t num_diffs = diffs->NumPars();
    for (size_t i=0; i<num_diffs; ++i)
    {
        out << "double " + diffs->ShortKey(i) + ";\n";
        if (!diffs->IsFreeze(i))
            out << "double " + diffs->TempKey(i) + ";\n";
    }
    out << "//End WriteVarDecls\n";
    out << "\n";
}

std::string CFileBase::MakeName(const std::string& name) const
{
    std::string out(name);
    const int len = out.length();
#ifdef Q_OS_WIN
    if (out.substr(len-2) != ".cpp") out += ".cpp";
#else
    if (out.substr(len-2) != ".c") out += ".c";
#endif
    return out;
}
