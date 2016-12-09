use lela::term::Term;

#[derive(PartialEq, Eq, PartialOrd, Ord, Hash, Clone, Copy, Debug)]
pub enum Literal<'a> {
    Pos(Term<'a>, Term<'a>),
    Neg(Term<'a>, Term<'a>),
}

fn lr<'a>(lhs: Term<'a>, rhs: Term<'a>) -> (Term<'a>, Term<'a>) {
    if lhs.fun() && !rhs.fun() {
        (lhs, rhs)
    } else if rhs.fun() && !lhs.fun() {
        (rhs, lhs)
    } else if lhs < rhs {
        (lhs, rhs)
    } else {
        (rhs, lhs)
    }
}

impl<'a> Literal<'a> {
    pub fn new_pos(lhs: Term<'a>, rhs: Term<'a>) -> Self {
        let (l, r) = lr(lhs, rhs);
        Literal::Pos(l, r)
    }

    pub fn new_neg(lhs: Term<'a>, rhs: Term<'a>) -> Self {
        let (l, r) = lr(lhs, rhs);
        Literal::Neg(l, r)
    }

    pub fn pos(&self) -> bool {
        match *self {
            Literal::Pos(_, _) => true,
            Literal::Neg(_, _) => false,
        }
    }

    pub fn lhs(&self) -> &Term<'a> {
        match *self {
            Literal::Pos(ref lhs, _) => lhs,
            Literal::Neg(ref lhs, _) => lhs,
        }
    }

    pub fn rhs(&self) -> &Term<'a> {
        match *self {
            Literal::Pos(_, ref rhs) => rhs,
            Literal::Neg(_, ref rhs) => rhs,
        }
    }

    pub fn flip(self) -> Self {
        match self {
            Literal::Pos(lhs, rhs) => Literal::Neg(lhs, rhs),
            Literal::Neg(lhs, rhs) => Literal::Pos(lhs, rhs),
        }
    }

    pub fn ground(&self) -> bool {
        self.lhs().ground() && self.rhs().ground()
    }

    pub fn primitive(&self) -> bool {
        self.lhs().primitive() && self.rhs().name()
    }

    pub fn quasiprimitive(&self) -> bool {
        self.lhs().quasiprimitive() && (self.rhs().name() || self.rhs().var())
    }

    pub fn valid(&self) -> bool {
        (self.pos() && self.lhs() == self.rhs()) ||
        (!self.pos() && self.lhs().name() && self.rhs().name() && self.lhs() != self.rhs()) ||
        (!self.pos() && self.lhs().sort() != self.rhs().sort())
    }

    pub fn invalid(&self) -> bool {
        (!self.pos() && self.lhs() == self.rhs()) ||
        (self.pos() && self.lhs().name() && self.rhs().name() && self.lhs() != self.rhs()) ||
        (self.pos() && self.lhs().sort() != self.rhs().sort())
    }

    pub fn complementary(a: &Self, b: &Self) -> bool {
        assert!(a.primitive());
        assert!(b.primitive());
        a.lhs() == b.lhs() &&
        ((a.pos() != b.pos() && a.rhs() == b.rhs()) ||
         (a.pos() == b.pos() && a.rhs().name() && b.rhs().name() && a.rhs() != b.rhs()))
    }

    pub fn subsumes(&self, other: &Self) -> bool {
        assert!(self.primitive());
        assert!(other.primitive());
        self.lhs() == other.lhs() &&
        ((self.pos() == other.pos() && self.rhs() == other.rhs()) ||
         (self.pos() && !other.pos() && self.rhs().name() && other.rhs().name() &&
          self.rhs() != other.rhs()))
    }

    pub fn terms<'b>(&'b self) -> Box<Iterator<Item = &'b Term<'a>> + 'b> {
        Box::new(self.lhs().terms().chain(self.rhs().terms()))
    }
}
