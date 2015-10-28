#include "statevarmodel.h"
#include "../memrep/modelmgr.h"

StateVarModel::StateVarModel(QObject *parent, const std::string& name) :
    NumericModelBase(parent, name)
{
}

std::string StateVarModel::ShortKey(size_t idx) const
{
    return ParamModelBase::Key(idx).substr( 0, Key(idx).size()-1 );
}

int StateVarModel::ShortKeyIndex(const std::string& par_name) const
{
    std::string key = par_name + "'";
    return KeyIndex(key);
}

std::string StateVarModel::TempExpression(size_t idx) const
{
    const std::string model_step = std::to_string( ModelMgr::Instance()->ModelStep() );
    return TempExpression(idx, model_step);
}

std::string StateVarModel::TempExprnForCFile(size_t idx) const
{
    return TempExpression(idx, "tau");
}

std::string StateVarModel::ExprnInsert(const std::string& in, const std::string& exprn,
                        const std::string& token) const
{
    std::string out(exprn);

    size_t pos = 0,
            in_len = in.length(),
            tok_len = token.length();
    while((pos = out.find(token, pos)) != std::string::npos)
    {
        if ( (pos>0 && std::isalnum(out.at(pos-1)))
             || (pos<in_len && std::isalnum(out.at(pos+tok_len))) )
        {
            pos += tok_len;
            continue;
        }
        out.replace(pos, tok_len, in);
        pos += in_len;
    }

    return out;
}

std::string StateVarModel::TempExpression(size_t idx, const std::string& model_step) const
{
    const std::string& temp = TempKey(idx),
            & key = ShortKey(idx),
            & value = Value(idx);
    std::string out;
    switch (ModelMgr::Instance()->DiffMethod())
    {
        case ModelMgr::EULER:
            out = temp + " = " + key + " + " + model_step + "*(" + value + ")";
            break;
        case ModelMgr::EULER2:
        {
            const std::string k1 = "__k1 = " + key + " + " + model_step + "*(" + value + ")";
            out = k1 + ", ";
            out += temp + " = " + key + " + " + model_step
                    + "*0.5*(" + value + " + __k1)";
            break;
        }
        case ModelMgr::RUNGE_KUTTA:
        {
            const std::string k1 = "__k1 = " + value,
                    k2 = "__k2 = " + ExprnInsert("("+key+"+("+model_step+"/2)*__k1)", value, key),
                    k3 = "__k3 = " + ExprnInsert("("+key+"+("+model_step+"/2)*__k2)", value, key),
                    k4 = "__k4 = " + ExprnInsert("("+key+"+"+model_step+"*__k3)", value, key);
            out = k1 + ", " + k2 + ", " + k3 + ", " + k4 + ", ";
            out += temp + " = " + key
                    + " + (" + model_step + "/6)*(__k1 + 2*__k2 + 2*__k3 + __k4)";
            break;
        }
        case ModelMgr::UNKNOWN:
            throw std::runtime_error("StateVarModel::TempExpression: Bad DIFF_METHOD type");
    }
    return out;
}
