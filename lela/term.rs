use std::cmp::Ordering;
use std::collections::HashSet;
use std::hash::{Hash, Hasher};
use std::mem::transmute;
use lela::symbol::Arity;
use lela::symbol::Sort;
use lela::symbol::Symbol;

#[derive(Eq, Clone, Copy, Debug)]
pub struct Term<'a>(&'a TermData<'a>);

impl<'a> Term<'a> {
    pub fn sym(&self) -> Symbol {
        (self.0).0
    }

    pub fn args(&self) -> &Vec<Term> {
        &(self.0).1
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

    pub fn ground(&self) -> bool {
        self.name() || (self.fun() && self.args().iter().all(|t| t.ground()))
    }

    pub fn primitive(&self) -> bool {
        self.fun() && self.args().iter().all(|t| t.name())
    }

    pub fn quasiprimitive(&self) -> bool {
        self.fun() && self.args().iter().all(|t| t.name() || t.var())
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
pub struct TermData<'a>(Symbol, Vec<Term<'a>>);


#[derive(Debug)]
pub struct Factory<'a>(HashSet<Box<TermData<'a>>>);

impl<'a> Factory<'a> {
    pub fn new() -> Self {
        Factory(HashSet::new())
    }

    pub fn new_term<'b>(&'b mut self, sym: Symbol, args: Vec<Term<'a>>) -> Term<'a> {
        assert_eq!(sym.arity(), args.len() as Arity);
        let tdb = Box::new(TermData(sym, args));
        if !self.0.contains(&tdb) {
            self.0.insert(tdb.clone());
        }
        let tdbr = self.0.get(&tdb).expect("Present or newly inserted");
        let tdr1: &'b TermData = tdbr.as_ref();
        let tdr2: &'a TermData = unsafe { transmute(tdr1) };
        Term(tdr2)
    }
}
