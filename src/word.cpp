#include "word.hpp"

#include <algorithm>
#include <vector>

#include "emitter.hpp"

void emit_add(uni::Emitter& emitter);
void emit_sub(uni::Emitter& emitter);
void emit_mult(uni::Emitter& emitter);
void emit_div(uni::Emitter& emitter);
void emit_printi(uni::Emitter& emitter);
void emit_printf(uni::Emitter& emitter);
void emit_prints(uni::Emitter& emitter);
void emit_eq(uni::Emitter& emitter);
void emit_neq(uni::Emitter& emitter);
void emit_lt(uni::Emitter& emitter);
void emit_gt(uni::Emitter& emitter);
void emit_le(uni::Emitter& emitter);
void emit_ge(uni::Emitter& emitter);
void emit_dup(uni::Emitter& emitter);
void emit_drop(uni::Emitter& emitter);
void emit_swap(uni::Emitter& emitter);
void emit_over(uni::Emitter& emitter);

static uni::Type INT1[] = { UNI_TYPE_INT };
static uni::Type INT2[] = { UNI_TYPE_INT, UNI_TYPE_INT };

static uni::Type FLOAT1[] = { UNI_TYPE_FLOAT };

static uni::Type NUM_A[] = { UNI_TYPE_NUM(0) };
static uni::Type NUM_AA[] = { UNI_TYPE_NUM(0), UNI_TYPE_NUM(0) };

static uni::Type STR1[] = { UNI_TYPE_STRING };

static uni::Type VAR_A[] = { UNI_TYPE_VAR(0) };
static uni::Type VAR_AA[] = { UNI_TYPE_VAR(0), UNI_TYPE_VAR(0) };
static uni::Type VAR_AB[] = { UNI_TYPE_VAR(0), UNI_TYPE_VAR(1) };
static uni::Type VAR_BA[] = { UNI_TYPE_VAR(1), UNI_TYPE_VAR(0) };
static uni::Type VAR_ABA[] = { UNI_TYPE_VAR(0), UNI_TYPE_VAR(1), UNI_TYPE_VAR(0) };

#define ARRAY_VEC(arr) std::begin(arr), std::end(arr)

static std::vector<uni::Word> word_table({
    uni::Word{"+",      {ARRAY_VEC(NUM_AA)},    {ARRAY_VEC(NUM_A)},     emit_add,       nullptr},
    uni::Word{"-",      {ARRAY_VEC(NUM_AA)},    {ARRAY_VEC(NUM_A)},     emit_sub,       nullptr},
    uni::Word{"*",      {ARRAY_VEC(NUM_AA)},    {ARRAY_VEC(NUM_A)},     emit_mult,      nullptr},
    uni::Word{"/",      {ARRAY_VEC(NUM_AA)},    {ARRAY_VEC(NUM_A)},     emit_div,       nullptr},
    uni::Word{"printi", {ARRAY_VEC(INT1)},      {},                     emit_printi,    nullptr},
    uni::Word{"printf", {ARRAY_VEC(FLOAT1)},    {},                     emit_printf,    nullptr},
    uni::Word{"prints", {ARRAY_VEC(STR1)},      {},                     emit_prints,    nullptr},
    uni::Word{"==",     {ARRAY_VEC(NUM_AA)},    {ARRAY_VEC(INT1)},      emit_eq,        nullptr},
    uni::Word{"!=",     {ARRAY_VEC(NUM_AA)},    {ARRAY_VEC(INT1)},      emit_neq,       nullptr},
    uni::Word{"<",      {ARRAY_VEC(NUM_AA)},    {ARRAY_VEC(INT1)},      emit_lt,        nullptr},
    uni::Word{">",      {ARRAY_VEC(NUM_AA)},    {ARRAY_VEC(INT1)},      emit_gt,        nullptr},
    uni::Word{"<=",     {ARRAY_VEC(NUM_AA)},    {ARRAY_VEC(INT1)},      emit_le,        nullptr},
    uni::Word{">=",     {ARRAY_VEC(NUM_AA)},    {ARRAY_VEC(INT1)},      emit_ge,        nullptr},
    uni::Word{"dup",    {ARRAY_VEC(VAR_A)},     {ARRAY_VEC(VAR_AA)},    emit_dup,       nullptr},
    uni::Word{"drop",   {ARRAY_VEC(VAR_A)},     {},                     emit_drop,      nullptr},
    uni::Word{"swap",   {ARRAY_VEC(VAR_AB)},    {ARRAY_VEC(VAR_BA)},    emit_swap,      nullptr},
    uni::Word{"over",   {ARRAY_VEC(VAR_AB)},    {ARRAY_VEC(VAR_ABA)},   emit_over,      nullptr},
});

namespace uni {
    void registerWord(Word word) {
        word_table.push_back(word);
    }

    Word* lookupWord(std::string_view name) {
        auto result = std::find_if(
            word_table.begin(), word_table.end(),
            [name](const Word& word) {
                return word.name == name;
            }
        );

        return (result != word_table.end())? &*result : nullptr;
    }
}

#define RESOLVE_FLOAT(emitter, a, b, fexp, iexp)                    \
    if(                                                             \
        a->getType()->isDoubleTy() ||                               \
        b->getType()->isDoubleTy()                                  \
    ) {                                                             \
        if(!a->getType()->isDoubleTy()) {                           \
            a = emitter.builder.CreateSIToFP(                       \
                a, llvm::Type::getDoubleTy(emitter.ctx), "itof"     \
            );                                                      \
        }                                                           \
        if(!b->getType()->isDoubleTy()) {                           \
            b = emitter.builder.CreateSIToFP(                       \
                b, llvm::Type::getDoubleTy(emitter.ctx), "itof"     \
            );                                                      \
        }                                                           \
        fexp;                                                       \
    } else {                                                        \
        iexp;                                                       \
    }                                                               \

void emit_add(uni::Emitter& emitter) {
    llvm::Value* b = emitter.pop();
    llvm::Value* a = emitter.pop();

    llvm::Value* r;
    RESOLVE_FLOAT(
        emitter,
        a, b,
        r = emitter.builder.CreateFAdd(a, b, "fadd"),
        r = emitter.builder.CreateAdd(a, b, "add")
    );

    emitter.push(r);
}

void emit_sub(uni::Emitter& emitter) {
    llvm::Value* b = emitter.pop();
    llvm::Value* a = emitter.pop();

    llvm::Value* r;
    RESOLVE_FLOAT(
        emitter,
        a, b,
        r = emitter.builder.CreateFSub(a, b, "fsub"),
        r = emitter.builder.CreateSub(a, b, "sub")
    );

    emitter.push(r);
}

void emit_mult(uni::Emitter& emitter) {
    llvm::Value* b = emitter.pop();
    llvm::Value* a = emitter.pop();

    llvm::Value* r;
    RESOLVE_FLOAT(
        emitter,
        a, b,
        r = emitter.builder.CreateFMul(a, b, "fmul"),
        r = emitter.builder.CreateMul(a, b, "mul")
    );

    emitter.push(r);
}

void emit_div(uni::Emitter& emitter) {
    llvm::Value* b = emitter.pop();
    llvm::Value* a = emitter.pop();

    llvm::Value* r;
    RESOLVE_FLOAT(
        emitter,
        a, b,
        r = emitter.builder.CreateFDiv(a, b, "fdiv"),
        r = emitter.builder.CreateSDiv(a, b, "div")
    );

    emitter.push(r);
}

void emit_printi(uni::Emitter& emitter) {
    llvm::Value* a = emitter.pop();
    emitter.builder.CreateCall(emitter.printf_fn, {emitter.fmt_int, a}, "printi");
}

void emit_printf(uni::Emitter& emitter) {
    llvm::Value* a = emitter.pop();
    emitter.builder.CreateCall(emitter.printf_fn, {emitter.fmt_flt, a}, "printf");
}

void emit_prints(uni::Emitter& emitter) {
    llvm::Value* a = emitter.pop();
    emitter.builder.CreateCall(emitter.printf_fn, {emitter.fmt_str, a}, "printf");
}

static void emit_icmp(
    uni::Emitter& emitter, llvm::ICmpInst::Predicate pred, 
    llvm::Value* a, llvm::Value* b
) {
    llvm::Value* boolean = emitter.builder.CreateICmp(pred, a, b, "icmp");
    llvm::Value* r = emitter.builder.CreateZExt(
        boolean, llvm::Type::getInt64Ty(emitter.ctx), "ext"
    );

    emitter.push(r);
}
static void emit_fcmp(
    uni::Emitter& emitter, llvm::FCmpInst::Predicate pred,
    llvm::Value* a, llvm::Value* b
) {
    llvm::Value* boolean = emitter.builder.CreateFCmp(pred, a, b, "fcmp");
    llvm::Value* r = emitter.builder.CreateZExt(
        boolean, llvm::Type::getInt64Ty(emitter.ctx), "ext"
    );

    emitter.push(r);
}

void emit_eq(uni::Emitter& emitter) {
    llvm::Value* b = emitter.pop();
    llvm::Value* a = emitter.pop();

    RESOLVE_FLOAT(
        emitter,
        a, b,
        emit_fcmp(emitter, llvm::FCmpInst::Predicate::FCMP_OEQ, a, b),
        emit_icmp(emitter, llvm::ICmpInst::Predicate::ICMP_EQ, a, b)
    );
}

void emit_neq(uni::Emitter& emitter) {
    llvm::Value* b = emitter.pop();
    llvm::Value* a = emitter.pop();

    RESOLVE_FLOAT(
        emitter,
        a, b,
        emit_fcmp(emitter, llvm::FCmpInst::Predicate::FCMP_ONE, a, b),
        emit_icmp(emitter, llvm::ICmpInst::Predicate::ICMP_NE, a, b)
    );
}

void emit_lt(uni::Emitter& emitter) {
    llvm::Value* b = emitter.pop();
    llvm::Value* a = emitter.pop();

    RESOLVE_FLOAT(
        emitter,
        a, b,
        emit_fcmp(emitter, llvm::FCmpInst::Predicate::FCMP_OLT, a, b),
        emit_icmp(emitter, llvm::ICmpInst::Predicate::ICMP_SLT, a, b)
    );
}

void emit_gt(uni::Emitter& emitter) {
    llvm::Value* b = emitter.pop();
    llvm::Value* a = emitter.pop();

    RESOLVE_FLOAT(
        emitter,
        a, b,
        emit_fcmp(emitter, llvm::FCmpInst::FCMP_OGT, a, b),
        emit_icmp(emitter, llvm::ICmpInst::ICMP_SGT, a, b)
    );
}

void emit_le(uni::Emitter& emitter) {
    llvm::Value* b = emitter.pop();
    llvm::Value* a = emitter.pop();

    RESOLVE_FLOAT(
        emitter,
        a, b,
        emit_fcmp(emitter, llvm::FCmpInst::FCMP_OLE, a, b),
        emit_icmp(emitter, llvm::ICmpInst::ICMP_SLE, a, b)
    );
}

void emit_ge(uni::Emitter& emitter) {
    llvm::Value* b = emitter.pop();
    llvm::Value* a = emitter.pop();

    RESOLVE_FLOAT(
        emitter,
        a, b,
        emit_fcmp(emitter, llvm::FCmpInst::FCMP_OGE, a, b),
        emit_icmp(emitter, llvm::ICmpInst::ICMP_SGE, a, b)
    );
}

void emit_dup(uni::Emitter& emitter) {
    llvm::Value* a = emitter.pop();

    emitter.push(a);
    emitter.push(a);
}

void emit_drop(uni::Emitter& emitter) {
    emitter.pop();
}

void emit_swap(uni::Emitter& emitter) {
    llvm::Value* b = emitter.pop();
    llvm::Value* a = emitter.pop();

    emitter.push(b);
    emitter.push(a);
}

void emit_over(uni::Emitter& emitter) {
    llvm::Value* b = emitter.pop();
    llvm::Value* a = emitter.pop();

    emitter.push(a);
    emitter.push(b);
    emitter.push(a);
}

