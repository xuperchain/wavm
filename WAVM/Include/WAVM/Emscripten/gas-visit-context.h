#pragma once
#include <stdint.h>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "WAVM/IR/Module.h"
#include "WAVM/IR/OperatorPrinter.h"
#include "WAVM/IR/Operators.h"
#include "WAVM/IR/Types.h"
#include "WAVM/Inline/Assert.h"
#include "WAVM/Inline/BasicTypes.h"
#include "WAVM/Inline/Errors.h"
#include "WAVM/Logging/Logging.h"

#include "gas-cost-table.h"

using namespace WAVM;
using namespace WAVM::IR;

typedef CodeValidationProxyStream<OperatorEncoderStream> CodeStream;
typedef void OperatorEmitFunc(CodeStream*);

struct GasVisitor {
    typedef void Result;
    GasVisitor(Uptr idx, IR::Module& irModule, IR::FunctionDef& fd)
        : gasCounter(0), addGasFuncIndex(idx), module(irModule), functionDef(fd) {}

    ~GasVisitor() {
        if (encoderStream != nullptr) {
            delete encoderStream;
            encoderStream = nullptr;
        }
    }

    CodeStream *encoderStream;
    I64 gasCounter;  //trace gas used by current block
    Uptr addGasFuncIndex; //gas stat function index
    IR::Module& module;
    IR::FunctionDef& functionDef;

    std::vector<std::function<OperatorEmitFunc>> opEmitters;

    void gas_trap()
    {
        if (opEmitters.size() == 0) {
            //printf("gas=%llu, size=%lu\n",gasCounter, opEmitters.size());
            gasCounter = 0;
            return;
        }
        insert_inst();
        for (auto op : opEmitters)
        {
            op(encoderStream);
        }
        gasCounter = 0;
        opEmitters.clear();
    }

#define VISIT_OP(encoding, name, nameString, Imm, _4, _5)       \
    Result name(Imm imm) {                                      \
        gasCounter += g_gas_cost_table[nameString];                \
        opEmitters.push_back(                                   \
                [imm](CodeStream *codeStream){                  \
                codeStream->name(imm); });                      \
    }
    ENUM_NONCONTROL_NONPARAMETRIC_OPERATORS(VISIT_OP)
#undef VISIT_OP

    Result unknown(Opcode) {}

    void insert_inst()
    {
        encoderStream->i64_const({gasCounter});
        encoderStream->call({addGasFuncIndex});
    }

	Result block(ControlStructureImm imm)
    {
        gas_trap();
        encoderStream->block(imm);
        gasCounter += g_gas_cost_table["block"];
        pushControlStack(ControlContext::Type::block, "");

    }

	Result loop(ControlStructureImm imm)
    {
        gas_trap();
        encoderStream->loop(imm);
        gasCounter += g_gas_cost_table["loop"];
        pushControlStack(ControlContext::Type::loop, "");

    }
	Result if_(ControlStructureImm imm)
    {
        gas_trap();
        encoderStream->if_(imm);
        gasCounter += g_gas_cost_table["if_"];
        pushControlStack(ControlContext::Type::ifThen, "");

    }

	Result else_(NoImm imm)
	{
        gas_trap();
        encoderStream->else_(imm);
        gasCounter += g_gas_cost_table["else_"];
        controlStack.back().type = ControlContext::Type::ifElse;

	}

	Result end(NoImm imm)
	{
        gas_trap();
        encoderStream->end(imm);
        gasCounter += g_gas_cost_table["end"];
        controlStack.pop_back();

	}

	Result try_(ControlStructureImm imm)
    {
        gas_trap();
        encoderStream->try_(imm);
        gasCounter += g_gas_cost_table["try_"];
        pushControlStack(ControlContext::Type::try_, "");

    }

	Result catch_(ExceptionTypeImm imm)
	{
        gas_trap();
        encoderStream->catch_(imm);
        gasCounter += g_gas_cost_table["catch_"];
        controlStack.back().type = ControlContext::Type::catch_;

	}

	Result catch_all(NoImm imm)
	{
        gas_trap();
        encoderStream->catch_all(imm);
        gasCounter += g_gas_cost_table["catch_all"];
        controlStack.back().type = ControlContext::Type::catch_;

	}

    Result unreachable(NoImm imm)
    {
        opEmitters.push_back(
                [imm](CodeStream *codeStream){
                codeStream->unreachable(imm); });

    }

    Result br(BranchImm imm)
    {
        gas_trap();
        encoderStream->br(imm);
        gasCounter += g_gas_cost_table["br"];

    }

    Result br_if(BranchImm imm)
    {
        gas_trap();
        encoderStream->br_if(imm);
        gasCounter += g_gas_cost_table["br_if"];

    }

    Result br_table(BranchTableImm imm)
    {
        gas_trap();
        encoderStream->br_table(imm);
        gasCounter += g_gas_cost_table["br_table"];

    }

    Result return_(NoImm imm)
    {
        opEmitters.push_back(
                [imm](CodeStream *codeStream){
                codeStream->return_(imm); });
        gasCounter += g_gas_cost_table["return_"];
    }

    Result call(FunctionImm imm)
    {
        opEmitters.push_back(
                [imm](CodeStream *codeStream){
                codeStream->call(imm); });
        gasCounter += g_gas_cost_table["call"];
    }

    Result call_indirect(CallIndirectImm imm)
    {
        opEmitters.push_back(
                [imm](CodeStream *codeStream){
                codeStream->call_indirect(imm); });
        gasCounter += g_gas_cost_table["call_indirect"];
    }

    Result drop(NoImm imm)
    {
        opEmitters.push_back(
                [imm](CodeStream *codeStream){
                codeStream->drop(imm); });
        gasCounter += g_gas_cost_table["drop"];
    }

    Result select(NoImm imm)
    {
        opEmitters.push_back(
                [imm](CodeStream *codeStream){
                codeStream->select(imm); });
        gasCounter += g_gas_cost_table["select"];
    }

    Result local_set(GetOrSetVariableImm<false> imm)
    {
        opEmitters.push_back(
                [imm](CodeStream *codeStream){
                codeStream->local_set(imm); });
        gasCounter += g_gas_cost_table["local_set"];
    }

    Result local_get(GetOrSetVariableImm<false> imm)
    {
        opEmitters.push_back(
                [imm](CodeStream *codeStream){
                codeStream->local_get(imm); });
        gasCounter += g_gas_cost_table["local_get"];
    }

    Result local_tee(GetOrSetVariableImm<false> imm)
    {
        opEmitters.push_back(
                [imm](CodeStream *codeStream){
                codeStream->local_tee(imm); });
        gasCounter += g_gas_cost_table["local_tee"];
    }

    Result global_set(GetOrSetVariableImm<true> imm)
    {
        opEmitters.push_back(
                [imm](CodeStream *codeStream){
                codeStream->global_set(imm); });
        gasCounter += g_gas_cost_table["global_set"];
    }

    Result global_get(GetOrSetVariableImm<true> imm)
    {
        opEmitters.push_back(
                [imm](CodeStream *codeStream){
                codeStream->global_get(imm); });
        gasCounter += g_gas_cost_table["global_get"];
    }

    Result table_get(TableImm imm)
    {
        opEmitters.push_back(
                [imm](CodeStream *codeStream){
                codeStream->table_get(imm); });
        gasCounter += g_gas_cost_table["table_get"];
    }

    Result table_set(TableImm imm)
    {
        opEmitters.push_back(
                [imm](CodeStream *codeStream){
                codeStream->table_get(imm); });
        gasCounter += g_gas_cost_table["table_set"];
    }

    Result table_grow(TableImm imm)
    {
        opEmitters.push_back(
                [imm](CodeStream *codeStream){
                codeStream->table_grow(imm); });
        gasCounter += g_gas_cost_table["table_grow"];
    }

    Result table_fill(TableImm imm)
    {
        opEmitters.push_back(
                [imm](CodeStream *codeStream){
                codeStream->table_fill(imm); });
        gasCounter += g_gas_cost_table["table_fill"];
    }

    Result throw_(ExceptionTypeImm imm)
    {
        opEmitters.push_back(
                [imm](CodeStream *codeStream){
                codeStream->throw_(imm); });
        gasCounter += g_gas_cost_table["throw_"];
    }

    Result rethrow(RethrowImm imm)
    {
        opEmitters.push_back(
                [imm](CodeStream *codeStream){
                codeStream->rethrow(imm); });
        gasCounter += g_gas_cost_table["rethrow"];
    }

    void AddGas();

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

void GasVisitor::AddGas()
{
    Serialization::ArrayOutputStream functionCodes;
    OperatorEncoderStream  encoder(functionCodes);
    encoderStream = new CodeValidationProxyStream<OperatorEncoderStream>(
            module, functionDef, encoder);

	OperatorDecoderStream decoder(functionDef.code);
	pushControlStack(
		ControlContext::Type::function, "");
	while(decoder && controlStack.size()){ decoder.decodeOp(*this); }
    encoderStream->finishValidation();
    functionDef.code = functionCodes.getBytes();
}
