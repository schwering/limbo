use std::hash::Hasher;

#[derive(PartialEq, Eq, Hash, Clone, Copy, Debug)]
pub struct Fnv1aHasher(u64);

impl Fnv1aHasher {
    pub fn new() -> Self {
        Fnv1aHasher(0xcbf29ce484222325u64)
    }
}

impl Hasher for Fnv1aHasher {
    fn finish(&self) -> u64 {
        self.0
    }

    fn write(&mut self, bytes: &[u8]) {
        for byte in bytes.iter() {
            self.write_u8(*byte);
        }
    }

    fn write_u8(&mut self, i: u8) {
        self.0 = (self.0 ^ (i as u64)).wrapping_mul(magic_prime());
    }

    #[cfg_attr(rustfmt, rustfmt_skip)]
    fn write_u16(&mut self, i : u16) {
        self.0 = (self.0 ^ (((i >> 0) & 0xFF) as u64)).wrapping_mul(magic_prime())
               ^ (self.0 ^ (((i >> 8) & 0xFF) as u64)).wrapping_mul(magic_prime());
    }

    #[cfg_attr(rustfmt, rustfmt_skip)]
    fn write_u32(&mut self, i : u32) {
        self.0 = (self.0 ^ (((i >>  0) & 0xFF) as u64)).wrapping_mul(magic_prime())
               ^ (self.0 ^ (((i >>  8) & 0xFF) as u64)).wrapping_mul(magic_prime())
               ^ (self.0 ^ (((i >> 16) & 0xFF) as u64)).wrapping_mul(magic_prime())
               ^ (self.0 ^ (((i >> 24) & 0xFF) as u64)).wrapping_mul(magic_prime());
    }

    #[cfg_attr(rustfmt, rustfmt_skip)]
    fn write_u64(&mut self, i : u64) {
        self.0 = (self.0 ^ (((i >>  0) & 0xFF) as u64)).wrapping_mul(magic_prime())
               ^ (self.0 ^ (((i >>  8) & 0xFF) as u64)).wrapping_mul(magic_prime())
               ^ (self.0 ^ (((i >> 16) & 0xFF) as u64)).wrapping_mul(magic_prime())
               ^ (self.0 ^ (((i >> 24) & 0xFF) as u64)).wrapping_mul(magic_prime())
               ^ (self.0 ^ (((i >> 32) & 0xFF) as u64)).wrapping_mul(magic_prime())
               ^ (self.0 ^ (((i >> 40) & 0xFF) as u64)).wrapping_mul(magic_prime())
               ^ (self.0 ^ (((i >> 48) & 0xFF) as u64)).wrapping_mul(magic_prime())
               ^ (self.0 ^ (((i >> 56) & 0xFF) as u64)).wrapping_mul(magic_prime());
    }
}

fn offset_basis() -> u64 {
    0xcbf29ce484222325u64
}

fn magic_prime() -> u64 {
    0x00000100000001b3u64
}


#[cfg_attr(rustfmt, rustfmt_skip)]
pub fn fnv1a_hash(x: u64) -> u64 {
    ((((((((((((((((
      offset_basis()
      ^ ((x >>  0) & 0xFF)).wrapping_mul(magic_prime()))
      ^ ((x >>  8) & 0xFF)).wrapping_mul(magic_prime()))
      ^ ((x >> 16) & 0xFF)).wrapping_mul(magic_prime()))
      ^ ((x >> 24) & 0xFF)).wrapping_mul(magic_prime()))
      ^ ((x >> 32) & 0xFF)).wrapping_mul(magic_prime()))
      ^ ((x >> 40) & 0xFF)).wrapping_mul(magic_prime()))
      ^ ((x >> 48) & 0xFF)).wrapping_mul(magic_prime()))
      ^ ((x >> 56) & 0xFF)).wrapping_mul(magic_prime()))

}
