#pragma once

// We use a teplate for the ExecObj instead of inheritance because execution objects can have many different forms.
template<typename ExecObjT, typename ReturnTypeT>
class ExecBackend {
    public:
        virtual int loadObj(ExecObjT _execObj) = 0;
        virtual int execute(ReturnTypeT &_result) = 0;

        using exec_obj_type = ExecObjT;
        using return_type = ReturnTypeT;
};
