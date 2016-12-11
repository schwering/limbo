type Id = u32;
pub type Sort = u8;
pub type Arity = u8;

#[derive(PartialEq, Eq, Hash, Clone, Copy, Debug)]
pub enum Symbol {
    Var(Id, Sort),
    Name(Id, Sort),
    Fun(Id, Sort, Arity),
}

impl Symbol {
    pub fn var(&self) -> bool {
        match *self {
            Symbol::Var(_, _) => true,
            _ => false,
        }
    }

    pub fn name(&self) -> bool {
        match *self {
            Symbol::Name(_, _) => true,
            _ => false,
        }
    }

    pub fn fun(&self) -> bool {
        match *self {
            Symbol::Fun(_, _, _) => true,
            _ => false,
        }
    }

    pub fn sort(&self) -> Sort {
        match *self {
            Symbol::Var(_, sort) => sort,
            Symbol::Name(_, sort) => sort,
            Symbol::Fun(_, sort, _) => sort,
        }
    }

    pub fn arity(&self) -> Arity {
        match *self {
            Symbol::Var(_, _) => 0,
            Symbol::Name(_, _) => 0,
            Symbol::Fun(_, _, arity) => arity,
        }
    }
}



#[derive(Default, Debug)]
pub struct Factory {
    sort: Sort,
    var: Id,
    name: Id,
    fun: Id,
}

impl Factory {
    pub fn new() -> Self {
        Factory {
            sort: 0,
            var: 0,
            name: 0,
            fun: 0,
        }
    }

    pub fn new_sort(&mut self) -> Sort {
        self.sort += 1;
        self.sort
    }

    pub fn new_var(&mut self, sort: Sort) -> Symbol {
        self.var += 1;
        Symbol::Var(self.var, sort)
    }

    pub fn new_name(&mut self, sort: Sort) -> Symbol {
        self.name += 1;
        Symbol::Name(self.name, sort)
    }

    pub fn new_fun(&mut self, sort: Sort, arity: Arity) -> Symbol {
        self.fun += 1;
        Symbol::Fun(self.fun, sort, arity)
    }
}
