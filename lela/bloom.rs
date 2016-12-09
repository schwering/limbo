#[derive(PartialEq, Eq, Hash, Clone, Copy, Debug)]
pub struct Bloom(u64);

impl Bloom {
    pub fn new() -> Self {
        Bloom(0u64)
    }

    #[cfg_attr(rustfmt, rustfmt_skip)]
    pub fn add(&mut self, x: u64) {
        self.0 |= (1u64 << (hash(0, x) % 64))
               |  (1u64 << (hash(1, x) % 64))
               |  (1u64 << (hash(2, x) % 64))
               |  (1u64 << (hash(3, x) % 64));
    }

    #[cfg_attr(rustfmt, rustfmt_skip)]
    pub fn contains(&self, x: u64) -> bool {
        ( ((self.0 >> (hash(0, x) % 64)) & 1u64)
        & ((self.0 >> (hash(1, x) % 64)) & 1u64)
        & ((self.0 >> (hash(2, x) % 64)) & 1u64)
        & ((self.0 >> (hash(3, x) % 64)) & 1u64)) != 0
    }

    pub fn subset(a: &Self, b: &Self) -> bool {
        !(!a.0 | b.0) == 0
    }

    pub fn disjoint(a: &Self, b: &Self) -> bool {
        a.0 & b.0 == 0
    }
}

fn hash(i: u8, x: u64) -> u64 {
    (x >> (i * 16)) & 0xFFFF
}
