use std::iter;
use std::slice::Iter;

use lela::bloom::BloomSet;
use lela::literal::Literal;
use lela::term::Term;

#[derive(PartialEq, Eq, Hash, Clone, Debug)]
pub struct Clause<'a> {
    bloom: BloomSet,
    lits: Vec<Literal<'a>>,
}

impl<'a> Clause<'a> {
    pub fn new(mut lits: Vec<Literal<'a>>) -> Self {
        lits.sort();
        lits.dedup();
        lits.retain(|a| !a.valid());
        let mut b = BloomSet::new();
        for a in lits.iter() {
            b.add(a.lhs());
        }
        Clause {
            bloom: b,
            lits: lits,
        }
    }

    pub fn empty(&self) -> bool {
        self.len() == 0
    }

    pub fn unit(&self) -> bool {
        self.len() == 1
    }

    pub fn len(&self) -> usize {
        self.lits.len()
    }

    pub fn valid(&self) -> bool {
        self.literals().any(|a| a.valid())
    }

    pub fn invalid(&self) -> bool {
        self.literals().all(|a| a.invalid())
    }

    pub fn ground(&self) -> bool {
        self.literals().all(|a| a.ground())
    }

    pub fn primitive(&self) -> bool {
        self.literals().all(|a| a.primitive())
    }

    pub fn quasiprimitive(&self) -> bool {
        self.literals().all(|a| a.quasiprimitive())
    }

    pub fn subsumes(&self, other: &Self) -> bool {
        debug_assert!(self.primitive());
        debug_assert!(other.primitive());
        other.bloom.possibly_includes(&self.bloom) &&
        self.literals().all(|a| other.literals().any(|b| a.subsumes(b)))
    }

    pub fn propagate_in_place(&mut self, a: &Literal) -> bool {
        debug_assert!(self.primitive());
        debug_assert!(a.primitive());
        if self.bloom.possibly_contains(a.lhs()) {
            let n = self.len();
            self.lits.retain(|b| !Literal::complementary(a, b));
            self.len() != n
        } else {
            false
        }
    }

    pub fn propagate(&self, a: &Literal) -> Option<Clause> {
        let mut c = self.clone();
        if c.propagate_in_place(a) {
            Some(c)
        } else {
            None
        }
    }

    pub fn literals(&self) -> Iter<Literal<'a>> {
        self.lits.iter()
    }

    pub fn sub_terms<'b>(&'b self) -> impl Iterator<Item = &'b Term<'a>> + 'b {
        self.literals().flat_map(|a| a.sub_terms())
    }

    pub fn lhs_terms<'b>(&'b self) -> impl Iterator<Item = &'b Term<'a>> + 'b {
        self.literals().map(|a| a.lhs())
    }
}
