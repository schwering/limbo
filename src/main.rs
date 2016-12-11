#![feature(conservative_impl_trait)]
#[allow(dead_code)]

extern crate bitmap;

mod lela;
use lela::symbol::Symbol;
use lela::term::Term;
use lela::literal::Literal;
use lela::clause::Clause;

fn main() {
    let mut sf = lela::symbol::Factory::new();
    let mut tf = lela::term::Factory::new();

    let sort = sf.new_sort();

    let mut name: Vec<Term> = vec![];
    for _ in 0..3 {
        name.push(tf.new_term(sf.new_name(sort), vec![]));
    }
    println!("name {:?}", name.len());

    let mut nullary: Vec<Term> = vec![];
    for _ in 0..2 {
        nullary.push(tf.new_term(sf.new_fun(sort, 0), vec![]));
    }
    println!("nullary {:?}", nullary.len());

    let mut unary: Vec<Term> = vec![];
    for _ in 0..2 {
        for &t in name.iter() {
            unary.push(tf.new_term(sf.new_fun(sort, 1), vec![t]));
        }
    }
    println!("unary {:?}", unary.len());

    let mut binary: Vec<Term> = vec![];
    // for _ in 0..2 {
    // for &t1 in name.iter() {
    // for &t2 in name.iter() {
    // binary.push(tf.new_term(sf.new_fun(sort, 2), vec![t1, t2]));
    // }
    // }
    // }
    // println!("binary {:?}", binary.len());
    //

    let mut lits: Vec<Literal> = vec![];
    for &t1 in binary.iter().chain(unary.iter()).chain(nullary.iter()) {
        for &t2 in name.iter() {
            lits.push(Literal::new_pos(t1, t2));
            lits.push(Literal::new_neg(t2, t1));
        }
    }
    println!("lits {:?}", lits.len());

    let mut clauses: Vec<Clause> = vec![];
    for &a in lits.iter() {
        for &b in lits.iter() {
            for &c in lits.iter() {
                clauses.push(Clause::new(vec![a, b, c]));
            }
        }
    }
    println!("clauses {:?}", clauses.len());

    for c in clauses.iter() {
        for d in clauses.iter() {
            let sub = c.subsumes(d);
            // println!("{:?}", sub);
        }
    }
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
