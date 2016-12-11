use std::collections::HashMap;
use std::hash::{Hash, Hasher, BuildHasherDefault};

use lela::hash::Fnv1aHasher;
use lela::clause::Clause;
use lela::term::Term;

pub type Index = u32;
type TermMap<'a> = HashMap<Term<'a>, Index, BuildHasherDefault<Fnv1aHasher>>;

#[derive(Clone, Debug)]
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

    fn inherit(&'b self) -> Self {
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
