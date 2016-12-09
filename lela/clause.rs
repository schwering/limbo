use std::slice::Iter;
use lela::bloom::Bloom;
use lela::literal::Literal;
use lela::term::Term;

#[derive(PartialEq, Eq, Hash, Clone, Debug)]
pub struct Clause<'a> {
    bloom: Bloom,
    lits: Vec<Literal<'a>>,
}

impl<'a> Clause<'a> {
    pub fn new(mut lits: Vec<Literal<'a>>) -> Self {
        lits.sort();
        lits.dedup();
        lits.retain(|a| !a.valid());
        let mut b = Bloom::new();
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
        self.lits().any(|a| a.valid())
    }

    pub fn invalid(&self) -> bool {
        self.lits().all(|a| a.invalid())
    }

    pub fn ground(&self) -> bool {
        self.lits().all(|a| a.ground())
    }

    pub fn primitive(&self) -> bool {
        self.lits().all(|a| a.primitive())
    }

    pub fn quasiprimitive(&self) -> bool {
        self.lits().all(|a| a.quasiprimitive())
    }

    pub fn subsumes(&self, other: &Self) -> bool {
        assert!(self.primitive());
        assert!(other.primitive());
        Bloom::subset(&self.bloom, &other.bloom) &&
        self.lits().all(|a| other.lits().any(|b| a.subsumes(b)))
    }

    pub fn propagate_in_place(&mut self, a: &Literal) -> bool {
        assert!(self.primitive());
        assert!(a.primitive());
        if self.bloom.contains(a.lhs()) {
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

    pub fn lits(&self) -> Iter<Literal<'a>> {
        self.lits.iter()
    }

    pub fn terms<'b>(&'b self) -> Box<Iterator<Item = &'b Term<'a>> + 'b> {
        Box::new(self.lits().flat_map(|a| a.terms()))
    }
}
