#include "inputmgr.h"

InputMgr* InputMgr::_instance = nullptr;

InputMgr* InputMgr::Instance()
{
    if (!_instance) _instance = new InputMgr();
    return _instance;
}

InputMgr::~InputMgr()
{
    delete _instance;
}

int InputMgr::AssignInput(double* data, const std::string& type_str)
{
#ifdef DEBUG_FUNC
    ScopeTracker st("InputMgr::AssignInput", std::this_thread::get_id());
#endif
    int input_idx(-1);
    Input::TYPE type = Input::Type(type_str);
    switch (type)
    {
        case Input::UNI_RAND:
        case Input::GAMMA_RAND:
        case Input::NORM_RAND:
        {
            input_idx = EmplaceInput(data);
            _inputs[input_idx].GenerateInput(type);
            break;
        }
        case Input::INPUT_FILE:
        {
            input_idx = EmplaceInput(data);
            std::string file_name = type_str;
            file_name.erase(
                std::remove_if(file_name.begin(), file_name.end(), [&](std::string::value_type c)
                {
                    return c=='"';
                }),
                    file_name.end());
            _inputs[input_idx].LoadInput(file_name);
            _stepCts[input_idx] = 0;
            break;
        }
        case Input::USER:
            break;
        case Input::UNKNOWN:
            throw std::runtime_error("ModelMgr::AssignInput: Unknown input.");
    }
    return input_idx;
}
void InputMgr::ClearInputs()
{
    _inputs.clear();
    _stepCts.clear();
}
void InputMgr::InputEval()
{
    const size_t num_inputs = _inputs.size();
    for (size_t i=0; i<num_inputs; ++i)
    {
        Input& input = _inputs[i];
        if (++_stepCts[i]
                % (int)(1.0 / (_modelMgr->ModelStep()*input.SamplesPerUnitTime()) + 0.5)
                == 0)
            input.NextInput();
    }
}
void InputMgr::JumpToSample(int n)
{
    const size_t num_inputs = _inputs.size();
    for (size_t i=0; i<num_inputs; ++i)
    {
        const int ct = (int)( (double)n / (_modelMgr->ModelStep()) + 0.5 );
        _inputs[i].SeekTo(ct);
    }
}
Input::TYPE InputMgr::Type(size_t i) const
{
    assert(i<_inputs.size());
    return _inputs.at(i).Type();
}
double* InputMgr::Value(size_t i) const
{
    assert(i<_inputs.size());
    return _inputs.at(i).Value();
}

InputMgr::InputMgr() : _modelMgr(ModelMgr::Instance())
{
}

int InputMgr::EmplaceInput(double* data)
{
    _inputs.emplace_back(data);
    _stepCts.push_back(0);
    return (int)(_inputs.size() - 1);
}
