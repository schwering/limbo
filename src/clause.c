// vim:filetype=c:textwidth=80:shiftwidth=4:softtabstop=4:expandtab
#include "clause.h"

SET_IMPL(clause, literal_t *, literal_cmp);

static void generate_varmaps(
        setup_t *setup,
        varmap_t *varmap,
        const univ_clause_t *univ_clause,
        const stdvecset_t *zs,
        const stdset_t *ns,
        int i)
{
    if (i < varset_size(&univ_clause->vars)) {
        const var_t var = varset_get(&univ_clause->vars, i);
        for (int j = 0; j < stdset_size(ns); ++j) {
            const stdname_t n = stdset_get(ns, j);
            varmap_add_replace(varmap, var, n);
            generate_varmaps(setup, varmap, univ_clause, zs, ns, i+1);
        }
    } else {
        const clause_t *c = univ_clause->univ_clause(varmap);
        if (c != NULL) {
            setup_add(setup, c);
        }
    }
}

void ground_clauses(setup_t *setup,
        const univ_clause_t *univ_clauses[], int n_univ_clauses,
        const stdvecset_t *zs, const stdset_t *ns)
{
    setup_init(setup);
    // first ground the universal quantifiers
    // this is a bit complicated because we need to compute all possible
    // assignment of variables (elements from univ_clauses[i]->vars) and
    // standard names (elements of ns) for all i
    for (int i = 0; i < n_univ_clauses; ++i) {
        varmap_t varmap;
        varmap_init_with_size(&varmap, varset_size(&univ_clauses[i]->vars));
        generate_varmaps(
                setup,
                &varmap,
                univ_clauses[i],
                zs, ns, 0);
    }
    // now add all prefixes of all elements from ns
}

