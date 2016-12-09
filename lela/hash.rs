#[cfg_attr(rustfmt, rustfmt_skip)]
pub fn fnv1a_hash(x: u64) -> u64 {
    let offset_basis = 0xcbf29ce484222325u64;
    let magic_prime = 0x00000100000001b3u64;
    ((((((((((((((((
      offset_basis
      ^ ((x >>  0) & 0xFF)) * magic_prime)
      ^ ((x >>  8) & 0xFF)) * magic_prime)
      ^ ((x >> 16) & 0xFF)) * magic_prime)
      ^ ((x >> 24) & 0xFF)) * magic_prime)
      ^ ((x >> 32) & 0xFF)) * magic_prime)
      ^ ((x >> 40) & 0xFF)) * magic_prime)
      ^ ((x >> 48) & 0xFF)) * magic_prime)
      ^ ((x >> 56) & 0xFF)) * magic_prime)

}
