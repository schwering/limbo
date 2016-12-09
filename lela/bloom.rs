use std::hash::{Hash, Hasher};
use lela::hash::Fnv1aHasher;

#[derive(PartialEq, Eq, Hash, Clone, Copy, Default, Debug)]
pub struct Bloom(u64);

impl Bloom {
    pub fn new() -> Self {
        Bloom(0u64)
    }

    #[cfg_attr(rustfmt, rustfmt_skip)]
    pub fn add_u64(&mut self, x: u64) {
        self.0 |= (1u64 << (shift(0, x)))
               |  (1u64 << (shift(1, x)))
               |  (1u64 << (shift(2, x)))
               |  (1u64 << (shift(3, x)));
    }

    pub fn add<T: Hash>(&mut self, x: &T) {
        let mut h = Fnv1aHasher::new();
        x.hash(&mut h);
        self.add_u64(h.finish());
    }

    #[cfg_attr(rustfmt, rustfmt_skip)]
    pub fn contains_u64(&self, x: u64) -> bool {
        ( (self.0 >> (shift(0, x)))
        & (self.0 >> (shift(1, x)))
        & (self.0 >> (shift(2, x)))
        & (self.0 >> (shift(3, x))) & 1u64) != 0
    }

    pub fn contains<T: Hash>(&mut self, x: &T) -> bool {
        let mut h = Fnv1aHasher::new();
        x.hash(&mut h);
        self.contains_u64(h.finish())
    }

    pub fn subset(a: &Self, b: &Self) -> bool {
        !(!a.0 | b.0) == 0u64
    }

    pub fn disjoint(a: &Self, b: &Self) -> bool {
        a.0 & b.0 == 0u64
    }
}

fn shift(i: u8, x: u64) -> u64 {
    hash(i, x) % 64
}

fn hash(i: u8, x: u64) -> u64 {
    (x >> (i * 16)) & 0xFFFF
}
