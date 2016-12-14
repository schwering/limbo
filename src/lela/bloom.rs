use std::hash::{Hash, Hasher};

use lela::hash::Fnv1aHasher;

#[derive(PartialEq, Eq, Hash, Clone, Default, Debug)]
pub struct BloomFilter(u64);

impl BloomFilter {
    pub fn new() -> Self {
        BloomFilter(0u64)
    }

    pub fn union(a: &Self, b: &Self) -> Self {
        BloomFilter(a.0 | b.0)
    }

    pub fn intersection(a: &Self, b: &Self) -> Self {
        BloomFilter(a.0 & b.0)
    }

    #[cfg_attr(rustfmt, rustfmt_skip)]
    fn add(&mut self, x: u64) {
        self.0 |= (1u64 << (shift(0, x)))
               |  (1u64 << (shift(1, x)))
               |  (1u64 << (shift(2, x)))
               |  (1u64 << (shift(3, x)));
    }

    #[cfg_attr(rustfmt, rustfmt_skip)]
    fn contains(&self, x: u64) -> bool {
        ( (self.0 >> (shift(0, x)))
        & (self.0 >> (shift(1, x)))
        & (self.0 >> (shift(2, x)))
        & (self.0 >> (shift(3, x))) & 1u64) != 0
    }

    pub fn subset(a: &Self, b: &Self) -> bool {
        !(!a.0 | b.0) == 0u64
    }

    pub fn overlap(a: &Self, b: &Self) -> bool {
        a.0 & b.0 != 0u64
    }
}

#[derive(PartialEq, Eq, Hash, Clone, Default, Debug)]
pub struct BloomSet(BloomFilter);

impl BloomSet {
    pub fn new() -> Self {
        BloomSet(BloomFilter::new())
    }

    pub fn union(a: &Self, b: &Self) -> Self {
        BloomSet(BloomFilter::union(&a.0, &b.0))
    }

    pub fn intersection(a: &Self, b: &Self) -> Self {
        BloomSet(BloomFilter::intersection(&a.0, &b.0))
    }

    pub fn add<T: Hash>(&mut self, x: &T) {
        let mut h = Fnv1aHasher::new();
        x.hash(&mut h);
        self.0.add(h.finish());
    }

    pub fn possibly_contains<T: Hash>(&mut self, x: &T) -> bool {
        let mut h = Fnv1aHasher::new();
        x.hash(&mut h);
        self.0.contains(h.finish())
    }

    pub fn possibly_includes(&self, other: &Self) -> bool {
        BloomFilter::subset(&other.0, &self.0)
    }

    pub fn possibly_overlap(a: &Self, b: &Self) -> bool {
        BloomFilter::overlap(&a.0, &b.0)
    }
}

fn shift(i: u8, x: u64) -> u64 {
    hash(i, x) % 64
}

fn hash(i: u8, x: u64) -> u64 {
    (x >> (i * 16)) & 0xFFFF
}
