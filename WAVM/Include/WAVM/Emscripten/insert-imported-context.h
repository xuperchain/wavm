#pragma once

typedef CodeValidationProxyStream<OperatorEncoderStream> CodeStream;

template<typename InnerStream> struct OperatorStreamProxy
{
    typedef void Result;
    InnerStream* innerStream;
    OperatorStreamProxy(InnerStream* inInnerStream) : innerStream(inInnerStream) {}

#define VISIT_OPCODE(_, name, _2, Imm, ...)                                                \
    Result name(Imm imm = {}) { innerStream->name(imm); }
    ENUM_OPERATORS(VISIT_OPCODE)
#undef VISIT_OPCODE
};

struct ImportFunctionInsertVisitor : OperatorStreamProxy<CodeStream>
{
    ImportFunctionInsertVisitor(IR::Module& irModule, std::string name) :
        OperatorStreamProxy<CodeStream>(nullptr), module(irModule), exportName(name) {}

    ~ImportFunctionInsertVisitor() {}
    IR::Module& module;
    std::string exportName;
    Uptr insertedIndex;

    Result unknown(Opcode) {}
    Result block(ControlStructureImm imm)
    {
        innerStream->block(imm);
        pushControlStack(ControlContext::Type::block, "");
    }

    Result loop(ControlStructureImm imm)
    {
        innerStream->loop(imm);
        pushControlStack(ControlContext::Type::loop, "");
    }

    Result if_(ControlStructureImm imm)
    {
        innerStream->if_(imm);
        pushControlStack(ControlContext::Type::ifThen, "");
    }

    Result else_(NoImm imm)
    {
        innerStream->else_(imm);
        controlStack.back().type = ControlContext::Type::ifElse;
    }

    Result end(NoImm imm)
    {
        innerStream->end(imm);
        controlStack.pop_back();
    }

    Result try_(ControlStructureImm imm)
    {
        innerStream->try_(imm);
        pushControlStack(ControlContext::Type::try_, "");
    }

    Result catch_(ExceptionTypeImm imm)
    {
        innerStream->catch_(imm);
        controlStack.back().type = ControlContext::Type::catch_;
    }

    Result catch_all(NoImm imm)
    {
        innerStream->catch_all(imm);
        controlStack.back().type = ControlContext::Type::catch_;
    }

    Result call(FunctionImm imm)
    {
        if (imm.functionIndex >= insertedIndex)
            imm.functionIndex += 1;
        innerStream->call(imm);
    }

    Result ref_func(FunctionImm imm)
    {
        if (imm.functionIndex >= insertedIndex) { ++imm.functionIndex; }
        innerStream->ref_func(imm);
    }

    void AddImportedFunc();

private:
    struct ControlContext
    {
        enum class Type : U8
        {
            function,
            block,
            ifThen,
            ifElse,
            loop,
            try_,
            catch_,
        };
        Type type;
        std::string lableId;
    };
    std::vector<ControlContext> controlStack;
    void pushControlStack(ControlContext::Type type, std::string lableId)
    {
        controlStack.push_back({type, lableId});
    }
};

/*
 *  add imported function add the tail of the function.imports
    - IR::Module::exports
    - IR::Module::elemSegments
    - IR::Module::startFunctionIndex
    - IR::FunctionImm in the byte code for call instructions
*/
void ImportFunctionInsertVisitor::AddImportedFunc()
{
    insertedIndex = module.functions.imports.size();
    //insert types
    module.types.push_back(FunctionType{{}, ValueType::i64});

    //right shift all the index in elemSegments
    for (auto& seg : module.elemSegments)
    {
        for (auto& elem : seg.elems)
        {
            if (elem.index >= insertedIndex)
            {
                elem.index += 1;
            }
        }
    }

    //update start function
    if (module.startFunctionIndex != UINTPTR_MAX
            && module.startFunctionIndex >=  module.functions.imports.size()) {
        module.startFunctionIndex += 1;
    }

    //update exports
    for (auto& export_ : module.exports) {
        export_.index += 1;
    }

    // insert imported function
    module.functions.imports.push_back(
            {{module.types.size() - 1}, std::move("env"), std::move(exportName)});

    for (int i = 0; i < module.functions.defs.size(); ++ i) {
        //update FunctionImm
        FunctionDef& functionDef = module.functions.defs[i];
        Serialization::ArrayOutputStream functionCodes;
        OperatorEncoderStream  encoder(functionCodes);
        innerStream = new CodeValidationProxyStream<OperatorEncoderStream>(
                module, functionDef, encoder);

        OperatorDecoderStream decoder(functionDef.code);
        pushControlStack(
                ControlContext::Type::function, "");
        while (decoder && controlStack.size()) { decoder.decodeOp(*this); }
        innerStream->finishValidation();
        functionDef.code = functionCodes.getBytes();
        delete innerStream;
        innerStream = nullptr;
    }
}
