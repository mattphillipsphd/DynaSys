#include "cfilebase.h"

CFileBase::CFileBase(const std::string& name, const std::string& ext)
    : _log(Log::Instance()), _modelMgr(ModelMgr::Instance()), 
      _nameBase(MakeName(name)), _nameExtension(ext)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("CFileBase::CFileBase", std::this_thread::get_id());
#endif
}

void CFileBase::Make()
{
#ifdef DEBUG_FUNC
    ScopeTracker st("CFileBase::Make", std::this_thread::get_id());
#endif
    std::ofstream out;
    out.open(Name());

    WriteIncludes(out);
    WriteGlobalConst(out);
    WriteVarDecls(out);
    WriteFuncs(out, ds::VAR);
    WriteFuncs(out, ds::DIFF);

    WriteExtraFuncs(out);

    //******************Write entry to main function******************
    WriteMainBegin(out);

    //Initialize variables from arguments, e.g. from argv
    WriteInitArgs(out);

    //Load all input files
    if (static_cast<const VariableModel*>(_modelMgr->Model(ds::VAR))->TypeCount(Input::INPUT_FILE)>0)
        WriteLoadInput(out);

    //Write header information to output destination
    WriteOutputHeader(out);

    //Initialize variables and differentials
    WriteInitVarsDiffs(out);

    //***********Begin main model loop
    WriteModelLoopBegin(out);

        //  Execute variable and differential functions
        WriteExecVarsDiffs(out);

    //  *****Save data if we're at the right point
        WriteSave(out);

        //  Check whether any conditions apply
        if (_modelMgr->Model(ds::COND)->NumPars()>0)
            WriteConditions(out);

    WriteModelLoopEnd(out);
    //***********End main model loop

    WriteMainEnd(out);
    //***********************End main function***********************

    out.close();

    MakeHFile();
}

const std::string CFileBase::Name() const
{
    return _nameBase + Suffix() + _nameExtension;
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
        for (int i=(int)posc-1; i!=-1; --i)
        {
            char c = temp.at(i);
            if (paren==0
                    && (OPS.find_first_of(c)!=std::string::npos || c=='('))
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
            if (paren==0
                    && (OPS.find_first_of(c)!=std::string::npos || c==')'))
            {
                if (c=='-' && i==posc+1) continue; //unary minus
                expt = temp.substr(posc+1,i-posc-1);
                expt_end = i;
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

        temp.replace(base_begin, expt_end-base_begin, "pow(" + base + "," + expt + ")");
        posc = temp.find_first_of('^');
    }
    return temp;
}

void CFileBase::WriteConditions(std::ofstream& out)
{
    out << "//Begin CFileBase::WriteConditions\n";
    const ConditionModel* conditions
            = static_cast<const ConditionModel*>( _modelMgr->Model(ds::COND) );
    const size_t num_conds = conditions->NumPars();
    for (size_t i=0; i<num_conds; ++i)
    {
        out <<
               "        if (" + conditions->Condition((int)i) + ")\n"
               "        {\n";

        const VecStr expressions = conditions->Results((int)i);
        for (const auto& it : expressions)
            out << "            " + it + ";\n";

        out << "        }\n";
    }
    out << "//End CFileBase::WriteConditions\n";
}

void CFileBase::WriteExecVarsDiffs(std::ofstream& out)
{
    out << "//Begin CFileBase::WriteExecVarsDiffs\n";
    const ParamModelBase* variables = _modelMgr->Model(ds::VAR),
            * diffs = _modelMgr->Model(ds::DIFF);
    const size_t num_vars = variables->NumPars(),
            num_diffs = diffs->NumPars();
    for (size_t i=0; i<num_vars; ++i)
    {
        if (variables->IsFreeze(i)) continue;
        std::string value = variables->Value(i);
        if (Input::Type(value)==Input::USER)
            out << "        " + variables->ShortKey(i) + "_func(" + FuncArgs(ds::VAR, i) + ");\n";
        else
        {
            std::string var = variables->ShortKey(i),
                    inputv = "input_" + var,
                    sputv = "sput_" + var, //samples per unit time
                    samps_ct = "ct_" + var;
            out <<
                   "        if (i % " + sputv + " == 0)\n"
                   "            " + var + " = " + inputv + "[" + samps_ct + "++];\n"
                   "        \n";
        }
    }
    for (size_t i=0; i<num_vars; ++i)
        if (Input::Type(variables->Value(i))==Input::USER && !variables->IsFreeze(i))
            out << "        " + variables->ShortKey(i) + " = " + variables->TempKey(i) + ";\n";
    out << "\n";
    for (size_t i=0; i<num_diffs; ++i)
        if (!diffs->IsFreeze(i))
            out << "        " + diffs->ShortKey(i) + "_func(" + FuncArgs(ds::VAR, i) + ");\n";
    for (size_t i=0; i<num_diffs; ++i)
        if (!diffs->IsFreeze(i))
            out << "        " + diffs->ShortKey(i) + " = " + diffs->TempKey(i) + ";\n";
    out << "//End CFileBase::WriteExecVarsDiffs\n";
    out << "\n";
}

void CFileBase::WriteFuncs(std::ofstream& out, ds::PMODEL mi)
{
    out << "//Begin CFileBase::WriteFuncs\n";
    const ParamModelBase* model = _modelMgr->Model(mi);
    const size_t num_pars = model->NumPars();
    for (size_t i=0; i<num_pars; ++i)
    {
        std::string value = model->Value(i);
        if (Input::Type(value)==Input::INPUT_FILE) continue;
        out <<
               "inline void " + model->ShortKey(i) + "_func()\n"
               "{\n"
               "    " + PreprocessExprn( model->TempExprnForCFile(i) ) + ";\n"
               "}\n";
    }
    out << "//End CFileBase::WriteFuncs\n";
    out << "\n";
}

void CFileBase::WriteGlobalConst(std::ofstream& out)
{
    out << "//Begin CFileBase::WriteGlobalConst\n";
    out << "const int INPUT_SIZE = " << Input::INPUT_SIZE << ";\n";
    out << "const double tau = " << _modelMgr->ModelStep() << ";\n";
    out << "//End CFileBase::WriteGlobalConst\n";
    out << "\n";
}

void CFileBase::WriteInitVarsDiffs(std::ofstream& out)
{
    out << "//Begin CFileBase::WriteInitVarsDiffs\n";
    const ParamModelBase* variables = _modelMgr->Model(ds::VAR),
            * diffs = _modelMgr->Model(ds::DIFF);
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
    out << "//End CFileBase::WriteInitVarsDiffs\n";
    out << "\n";
}

void CFileBase::WriteModelLoopBegin(std::ofstream& out)
{
    out <<
           "    for (int i=0; i<num_iters; ++i)\n"
           "    {\n";
}

void CFileBase::WriteModelLoopEnd(std::ofstream& out)
{
    out << "    }\n"
           "    \n";
}

void CFileBase::WriteLoadInput(std::ofstream& out)
{
    out << "//Begin CFileBase::WriteLoadInput\n";
    out << "    size_t br;\n";
    out << "    int num_elts;\n";
    const ParamModelBase* variables = _modelMgr->Model(ds::VAR);
    const size_t num_vars = variables->NumPars();
    for (size_t i=0, ct=0; i<num_vars; ++i)
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
                sputv = "sput_" + var, //samples per step
                samps_ct = "ct_" + var;
        if (ct++!=0) out << "\n";
        out <<
               "    FILE* " + fpv + " = fopen(\"" + value_abs + "\", \"rb\");\n"
               "    if (" + fpv + "==0) return 1;\n"
               "    int vnum_" + var + ";\n"
               "    br = fread(&vnum_" + var + ", sizeof(int), 1, " + fpv + ");\n"
               "    int " + sputv + ", " + samps_ct + " = 0;\n"
               "    br = fread(&" + sputv + ", sizeof(int), 1, " + fpv + ");\n"
               "    " + sputv + " = (int)(1.0/(tau * (double)" + sputv + ") + 0.5);\n"
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
    out << "//End CFileBase::WriteLoadInput\n";
    out << "\n";
}

void CFileBase::WriteSave(std::ofstream& out)
{
    out <<
           "    \n"
           "        if (i%save_mod_n==0)\n"
           "        {\n";

    WriteSaveBlockBegin(out);
    WriteDataOut(out, ds::VAR);
    WriteDataOut(out, ds::DIFF);
    WriteSaveBlockEnd(out);

    out <<
           "        }\n"
           "\n";
}

void CFileBase::WriteVarDecls(std::ofstream& out)
{
    out << "//Begin CFileBase::WriteVarDecls\n";
    const ParamModelBase* init_conds = _modelMgr->Model(ds::INIT);
    const size_t num_ics = init_conds->NumPars();
    for (size_t i=0; i<num_ics; ++i)
        out << "double " + init_conds->ShortKey(i) + "0;\n";
    out << "\n";

    const ParamModelBase* inputs = _modelMgr->Model(ds::INP);
    const size_t num_inputs = inputs->NumPars();
    for (size_t i=0; i<num_inputs; ++i)
        out << "double " + inputs->ShortKey(i) + ";\n";
    out << "\n";

    const ParamModelBase* variables = _modelMgr->Model(ds::VAR);
    const size_t num_vars = variables->NumPars();
    for (size_t i=0; i<num_vars; ++i)
    {
        out << "double " + variables->ShortKey(i) + ";\n";
        if ( Input::Type(variables->Value(i)) == Input::USER && !variables->IsFreeze(i) )
            out << "double " + variables->TempKey(i) + ";\n";
    }
    out << "\n";

    const ParamModelBase* diffs = _modelMgr->Model(ds::DIFF);
    const size_t num_diffs = diffs->NumPars();
    for (size_t i=0; i<num_diffs; ++i)
    {
        out << "double " + diffs->ShortKey(i) + ";\n";
        if (!diffs->IsFreeze(i))
            out << "double " + diffs->TempKey(i) + ";\n";
    }
    out << "//End CFileBase::WriteVarDecls\n";
    out << "\n";
}

std::string CFileBase::MakeName(const std::string& name) const
{
    std::string out(name);
    const size_t pos = out.find_last_of('.');
    if (pos!=std::string::npos)
        out.erase(pos);
    return out;
}
