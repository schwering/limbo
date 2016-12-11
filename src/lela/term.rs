use std::cmp::Ordering;
use std::collections::HashSet;
use std::hash::{Hash, Hasher, BuildHasherDefault};
use std::iter;
use std::mem::transmute;
use std::slice::Iter;

use lela::hash::Fnv1aHasher;
use lela::substitution::Substitution;
use lela::symbol::Arity;
use lela::symbol::Sort;
use lela::symbol::Symbol;

#[derive(Eq, Clone, Copy, Debug)]
pub struct Term<'a>(&'a TermData<'a>);

impl<'a> Term<'a> {
    pub fn sym(&self) -> Symbol {
        (self.0).sym
    }

    pub fn sort(&self) -> Sort {
        self.sym().sort()
    }

    pub fn var(&self) -> bool {
        self.sym().var()
    }

    pub fn name(&self) -> bool {
        self.sym().name()
    }

    pub fn fun(&self) -> bool {
        self.sym().fun()
    }

    pub fn arity(&self) -> Arity {
        debug_assert_eq!(self.sym().arity(), (self.0).args.len() as Arity);
        self.sym().arity()
    }

    pub fn args(&self) -> Iter<Term<'a>> {
        (self.0).args.iter()
    }

    pub fn arg(&self, i: usize) -> &Term<'a> {
        &(self.0).args[i]
    }

    pub fn ground(&self) -> bool {
        self.name() || (self.fun() && self.args().all(|t| t.ground()))
    }

    pub fn primitive(&self) -> bool {
        self.fun() && self.args().all(|t| t.name())
    }

    pub fn quasiprimitive(&self) -> bool {
        self.fun() && self.args().all(|t| t.name() || t.var())
    }

    pub fn terms<'b>(&'b self) -> Box<Iterator<Item = &'b Term<'a>> + 'b> {
        Box::new(iter::once(self).chain(self.args()))
    }

    pub fn substitute<S>(&self, theta: &S, factory: &mut Factory<'a>) -> Self
        where S: Substitution<Term<'a>>
    {
        if let Some(new) = theta.substitute(self) {
            new
        } else if self.arity() > 0 {
            let args = self.args()
                .map(|t| t.substitute(theta, factory))
                .collect::<Vec<Term<'a>>>();
            factory.new_term(self.sym(), args)
        } else {
            *self
        }
    }
}

impl<'a> PartialEq for Term<'a> {
    fn eq(&self, other: &Self) -> bool {
        self.0 as *const TermData == other.0 as *const TermData
    }
}

impl<'a> PartialOrd for Term<'a> {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        (self.0 as *const TermData).partial_cmp(&(other.0 as *const TermData))
    }
}

impl<'a> Ord for Term<'a> {
    fn cmp(&self, other: &Self) -> Ordering {
        (self.0 as *const TermData).cmp(&(other.0 as *const TermData))
    }
}

impl<'a> Hash for Term<'a> {
    fn hash<H: Hasher>(&self, state: &mut H) {
        (self.0 as *const TermData).hash(state);
    }
}

#[derive(PartialEq, Eq, Hash, Clone, Debug)]
pub struct TermData<'a> {
    sym: Symbol,
    args: Vec<Term<'a>>,
}


#[derive(Default, Debug)]
pub struct Factory<'a>(HashSet<Box<TermData<'a>>, BuildHasherDefault<Fnv1aHasher>>);

impl<'a> Factory<'a> {
    pub fn new() -> Self {
        Factory(HashSet::default())
    }

    pub fn new_term<'b>(&'b mut self, sym: Symbol, args: Vec<Term<'a>>) -> Term<'a> {
        debug_assert_eq!(sym.arity(), args.len() as Arity);
        let tdb = Box::new(TermData {
            sym: sym,
            args: args,
        });
        if !self.0.contains(&tdb) {
            self.0.insert(tdb.clone());
        }
        let tdbr = self.0.get(&tdb).expect("Present or newly inserted");
        let tdr1: &'b TermData = tdbr.as_ref();
        let tdr2: &'a TermData = unsafe { transmute(tdr1) };
        Term(tdr2)
    }
}
