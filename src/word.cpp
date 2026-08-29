#include "word.hpp"
#include <algorithm>
#include <vector>

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

void emit_add(uni::Emitter& emitter) {}
void emit_sub(uni::Emitter& emitter) {}
void emit_mult(uni::Emitter& emitter) {}
void emit_div(uni::Emitter& emitter) {}
void emit_printi(uni::Emitter& emitter) {}
void emit_printf(uni::Emitter& emitter) {}
void emit_prints(uni::Emitter& emitter) {}
void emit_eq(uni::Emitter& emitter) {}
void emit_neq(uni::Emitter& emitter) {}
void emit_lt(uni::Emitter& emitter) {}
void emit_gt(uni::Emitter& emitter) {}
void emit_le(uni::Emitter& emitter) {}
void emit_ge(uni::Emitter& emitter) {}
void emit_dup(uni::Emitter& emitter) {}
void emit_drop(uni::Emitter& emitter) {}
void emit_swap(uni::Emitter& emitter) {}
void emit_over(uni::Emitter& emitter) {}

