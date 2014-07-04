#ifndef MODELMGR_H
#define MODELMGR_H

#include "../globals/globals.h"
#include "../globals/scopetracker.h"
#include "../memrep/input.h"
#include "../memrep/notes.h"
#include "../models/conditionmodel.h"
#include "../models/numericmodelbase.h"

//#define DEBUG_MM_FUNC

class ModelMgr
{
    public:
        static ModelMgr* Instance();

        ~ModelMgr();

        void AddModel(ParamModelBase* model);
        void AddCondParameter(const std::string& test, const VecStr& results);
        void AddCondResult(int row, const std::string& result);
        void AddParameter(ds::PMODEL mi, const std::string& key, const std::string& value = "");
        void AssignInput(size_t i, double* data,
                         const std::string& type_str, bool do_lock);
            //Assign the source of the data for the variables
        void ClearModels();

        void SetCondValue(size_t row, const VecStr& results);
        void SetMaximum(ds::PMODEL mi, size_t idx, double val);
        void SetMinimum(ds::PMODEL mi, size_t idx, double val);
        void SetModel(ds::PMODEL mi, ParamModelBase* model);
        void SetModelStep(double step) { _modelStep = step; }
        void SetNotes(Notes *notes);
        void SetNotes(const std::string& text);
        void SetRange(ds::PMODEL mi, size_t idx, double min, double max);
        void SetValue(ds::PMODEL mi, size_t idx, const std::string& value);

        //Do not use ModelMgr as a general pass through function, for the models,
        //that is what Model(ds::PMODEL mi) is for.
        //Just create getters where a test or actual processing is necessary,
        //OR for symmetry, when there is a corresponding setter
        bool AreModelsInitialized() const;
        VecStr CondResults(size_t row) const;
        const Notes* GetNotes() const { return _notes; }
        double Maximum(ds::PMODEL mi, size_t idx) const;
        double Minimum(ds::PMODEL mi, size_t idx) const;
        inline const ParamModelBase* Model(ds::PMODEL mi) const { return _models.at(mi); }
        inline double ModelStep() const { return _modelStep; }
        double Range(ds::PMODEL mi, size_t idx) const;
        std::string Value(ds::PMODEL mi, size_t idx) const;

    private:
        ModelMgr();
#ifdef __GNUG__
        ModelMgr(const ModelMgr&) = delete;
        ModelMgr& operator=(const ModelMgr&) = delete;
        ModelMgr* operator*(ModelMgr*) = delete;
        const ModelMgr* operator*(const ModelMgr*) = delete;
#endif
        static ModelMgr* _instance;

        inline ConditionModel* CondModel();
        inline const ConditionModel* CondModel() const;
        std::vector<ParamModelBase*> MakeModelVec() const;

        std::vector<Input> _inputs;
        int _inputsPerUnitTime; // ### Needs to be done separately for each input
        Log* const _log;
        std::vector<ParamModelBase*> _models;
        double _modelStep;
        mutable std::mutex _mutex;
        Notes* _notes;
        int _stepCt;
        double* _varInitVals; //For temporary model resets
};

#endif // MODELMGR_H
