use std::collections::HashSet;
use std::mem::transmute;
use lela::symbol::Arity;
use lela::symbol::Symbol;

#[derive(Eq, Hash, Clone, Debug)]
pub struct Term<'a>(&'a TermData<'a>);

impl<'a> Term<'a> {
    pub fn sym(&self) -> Symbol {
        (self.0).0
    }

    pub fn args(&self) -> &Vec<Term> {
        &(self.0).1
    }
}

impl<'a> PartialEq for Term<'a> {
    fn eq(&self, other: &Term) -> bool {
        self.0 as *const TermData == other.0 as *const TermData
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

    pub fn new_term(&mut self, sym: Symbol, args: Vec<Term<'a>>) -> Term<'a> {
        assert_eq!(sym.arity(), args.len() as Arity);
        let tdb = Box::new(TermData(sym, args));
        if !self.0.contains(&tdb) {
            self.0.insert(tdb.clone());
        }
        let tdbr = self.0.get(&tdb).expect("Present or newly inserted");
        let tdr = tdbr.as_ref();
        let tdr2: &'a TermData<'a> = unsafe { transmute(tdr) };
        Term(tdr2)
    }
}
