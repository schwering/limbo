use std::collections::HashMap;
use std::hash::{Hash, Hasher, BuildHasherDefault};

use lela::hash::Fnv1aHasher;
use lela::clause::Clause;
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

    fn spawn(&'b self) -> Self {
        Setup {
            parent: Some(self),
            first: self.last(),
            clauses: Vec::new(),
            occurs: TermMap::default(),
        }
    }

    pub fn root(&self) -> &Self {
        if let Some(setup) = self.parent {
            setup.root()
        } else {
            self
        }
    }

    fn first(&self) -> Index {
        self.first
    }

    fn last(&self) -> Index {
        self.first + self.clauses.len() as u32
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
