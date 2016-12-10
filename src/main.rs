#![feature(conservative_impl_trait)]
#[allow(dead_code)]

mod lela;
use lela::symbol::Symbol;
use lela::term::Term;
use lela::literal::Literal;
use lela::clause::Clause;

fn main() {
    let mut sf = lela::symbol::Factory::new();
    let mut tf = lela::term::Factory::new();

    let sort = sf.new_sort();

    let mut var: Vec<Term> = vec![];
    for _ in [1..5].iter() {
        var.push(tf.new_term(sf.new_var(sort), vec![]));
    }

    let mut name: Vec<Term> = vec![];
    for _ in [1..10].iter() {
        name.push(tf.new_term(sf.new_name(sort), vec![]));
    }

    let mut nullary: Vec<Term> = vec![];
    for _ in [1..10].iter() {
        nullary.push(tf.new_term(sf.new_fun(sort, 0), vec![]));
    }

    let mut unary: Vec<Term> = vec![];
    for _ in [1..5].iter() {
        for &t in name.iter().chain(nullary.iter()) {
            unary.push(tf.new_term(sf.new_fun(sort, 1), vec![t]));
        }
    }

    let mut binary: Vec<Term> = vec![];
    for _ in [1..5].iter() {
        for &t1 in name.iter().chain(nullary.iter()).chain(unary.iter()) {
            for &t2 in name.iter().chain(nullary.iter()).chain(unary.iter()) {
                binary.push(tf.new_term(sf.new_fun(sort, 2), vec![t1, t2]));
            }
        }
    }

    let mut lits: Vec<Literal> = vec![];
}

fn main2() {
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
