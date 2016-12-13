use std::collections::HashMap;
use std::hash::{Hash, Hasher, BuildHasherDefault};

use lela::hash::Fnv1aHasher;
use lela::clause::Clause;
use lela::literal::Literal;
use lela::term::Term;

pub type Index = u32;
type TermMap<'a> = HashMap<Term<'a>, Index, BuildHasherDefault<Fnv1aHasher>>;

#[derive(Debug)]
pub struct Setup<'a: 'b, 'b> {
    parent: Option<&'b Setup<'a, 'b>>,
    first: Index,
    clauses: Vec<Clause<'a>>,
    occurs: TermMap<'a>,
}

impl<'a, 'b> Setup<'a, 'b> {
    pub fn new() -> Self {
        Setup {
            parent: None,
            first: 0,
            clauses: Vec::new(),
            occurs: TermMap::default(),
        }
    }

    pub fn add(&mut self, c: Clause<'a>) -> Option<Index> {
        if c.valid() || self.parent.map_or(false, |s| s.subsumes(&c)) {
            None
        } else {
            self.clauses.push(c);
            Some(self.last())
        }
    }

    pub fn init(&mut self) {}

    pub fn spawn(&'b self) -> Self {
        Setup {
            parent: Some(self),
            first: self.last(),
            clauses: Vec::new(),
            occurs: TermMap::default(),
        }
    }

    pub fn subsumes(&self, c: &Clause) -> bool {
        true
    }

    fn first(&self) -> Index {
        self.first
    }

    fn last(&self) -> Index {
        self.first + self.clauses.len() as u32
    }

    fn setups(&'b self) -> Iter<'a, 'b> {
        Iter(Some(self))
    }

    pub fn clauses(&'b self) -> impl Iterator<Item = &'b Clause<'a>> + 'b {
        self.setups().flat_map(|s| s.clauses.iter())
    }

    pub fn literals(&'b self) -> impl Iterator<Item = &'b Literal<'a>> + 'b {
        self.clauses().flat_map(|c| c.literals())
    }

    pub fn sub_terms(&'b self) -> impl Iterator<Item = &'b Term<'a>> + 'b {
        self.literals().flat_map(|a| a.sub_terms())
    }

    pub fn lhs_terms(&'b self) -> impl Iterator<Item = &'b Term<'a>> + 'b {
        self.clauses().flat_map(|c| c.lhs_terms())
    }
}

impl<'a, 'b> Default for Setup<'a, 'b> {
    fn default() -> Setup<'a, 'b> {
        Setup::new()
    }
}


#[derive(Clone, Debug)]
struct Iter<'a: 'b, 'b>(Option<&'b Setup<'a, 'b>>);

impl<'a, 'b> Iterator for Iter<'a, 'b> {
    type Item = &'b Setup<'a, 'b>;

    fn next(&mut self) -> Option<&'b Setup<'a, 'b>> {
        let this = self.0;
        self.0 = self.0.map_or(None, |s| s.parent);
        this
    }
}
