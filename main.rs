#![feature(conservative_impl_trait)]
#[allow(dead_code)]

mod lela;

fn main() {
    let mut sf = lela::symbol::Factory::new();
    let sort = sf.new_sort();
    let a = sf.new_fun(sort, 0);
    let u = sf.new_fun(sort, 0);
    let n = sf.new_name(sort);
    let f = sf.new_fun(sort, 2);
    let g = sf.new_fun(sort, 1);

    println!("Sort: {}", sort);
    println!("a: {:?}", a);
    println!("f: {:?}", f);
    println!("g: {:?}", g);
    println!("n: {:?}", n);

    let mut tf = lela::term::Factory::new();

    let t_a = tf.new_term(a, vec![]);
    println!("a = {:?}", t_a);

    let t_u = tf.new_term(u, vec![]);
    println!("u = {:?}", t_u);

    let t_n = tf.new_term(n, vec![]);
    println!("n = {:?}", t_n);

    {
        let t_a = tf.new_term(a, vec![]);
        let t_ga = tf.new_term(g, vec![t_a]);
        println!("g(a) = {:?}", t_ga);
    }
    let t_a = tf.new_term(a, vec![]);
    let t_ga = tf.new_term(g, vec![t_a]);
    println!("g(a) = {:?}", t_ga);

    let t_fan = tf.new_term(a, vec![]);
    println!("f(a,n) = {:?}", t_fan);
    let t_fun = t_fan.substitute(&lela::substitution::SingleSubstitution::new(t_a, t_u),
                                 &mut tf);
    println!("f(u,n) = {:?}", t_fun);

    println!("Hello World");
}
