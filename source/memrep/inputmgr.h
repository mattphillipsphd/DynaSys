#ifndef INPUTMGR_H
#define INPUTMGR_H

#include "input.h"
#include "modelmgr.h"
#include "../globals/globals.h"
#include "../globals/scopetracker.h"

class InputMgr
{
    public:
        static InputMgr* Instance();

        ~InputMgr();

        int AssignInput(double* listener, const std::string& type_str, int idx = -1);
            //Assign the source of the data for the variables
        void ClearInputs();
        void InputEval();
            //To be called on every iteration of the model
        void JumpToSample(int n);
        void RemoveListener(int idx, double* listener);

        size_t NumInputs() const { return _inputs.size(); }
        Input::TYPE Type(size_t i) const;
//        double* Value(size_t i) const;

    private:
        InputMgr();
#ifdef __GNUG__
        InputMgr(const ModelMgr&) = delete;
        InputMgr& operator=(const ModelMgr&) = delete;
        InputMgr* operator*(ModelMgr*) = delete;
        const InputMgr* operator*(const ModelMgr*) = delete;
#endif
        static InputMgr* _instance;

        int EmplaceInput(double* data, int idx);

        std::vector<Input> _inputs;
        ModelMgr* const _modelMgr;
        std::vector<int> _stepCts;
};

#endif // INPUTMGR_H
