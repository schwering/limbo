// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
/*
 * schwering@kbsg.rwth-aachen.de
 */
#ifndef _UTIL_H_
#define _UTIL_H_

// Action sequence Z. Use Z(a,b,c) to get an stdvec_t with a, b, c.
#define Z(z...) \
    ({\
        const stdname_t _Z_array[] = { z };\
        const size_t _Z_n = sizeof(_Z_array) / sizeof(_Z_array[0]);\
        NEW(stdvec_from_array(_Z_array, (int) _Z_n));\
    })

// Argument sequence A. Use A(a,b,c) to get an stdvec_t with a, b, c.
#define A(a...)     Z(a)

// Set of sensing literals SF. Use SF(l1,l2) to get an splitset_t with l1, l2.
#define SF(l...)\
    ({\
        const literal_t *_SF_array[] = { l };\
        const size_t _SF_n = sizeof(_SF_array) / sizeof(_SF_array[0]);\
        splitset_t *_SF_set = NEW(splitset_init_with_size((int) _SF_n));\
        for (int _SF_i = 0; _SF_i < (int) _SF_n; ++_SF_i) {\
            splitset_add(_SF_set, _SF_array[_SF_i]);\
        }\
        _SF_set;\
    })

// Literal L, positive P and negative N. Use N(Z(a,b,c), p, A(x,y,z)) for as a
// shorthand for a literal we would write like [a,b,c]~P(x,y,z).
#define L(z, sign, p, args) NEW(literal_init(z, sign, p, args))
#define P(z, p, args)       L(z, true, p, args)
#define N(z, p, args)       L(z, false, p, args)

// Clause C. Use C(P(...), N(...)) for a clause with a positive and a negative
// literal.
#define C(c...) ({\
                    const literal_t *_C_array[] = { c };\
                    const int _C_n = sizeof(_C_array) / sizeof(_C_array[0]);\
                    clause_t *_C_c = NEW(clause_init_with_size(_C_n));\
                    for (int _C_i = 0; _C_i < _C_n; ++_C_i) {\
                        clause_add(_C_c, _C_array[_C_i]);\
                    }\
                _C_c;\
                })

// Lookup a variable in varmap.
#define V(varname) \
    ({\
        assert(IS_VARIABLE(varname));\
        assert(varmap_contains(varmap, varname));\
        const stdname_t n = varmap_lookup(varmap, varname);\
        assert(IS_STDNAME(n));\
        n;\
    })

// Declares a clause. To be used in a function; then the check is a nested
// function.
#define DECL_CLAUSE(suffix, cond, clause) \
    bool check_##suffix(const void *check_arg, const varmap_t *varmap) {\
        return cond;\
    }\
    const clause_t *clause_##suffix = clause;

#endif

