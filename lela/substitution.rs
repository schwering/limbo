pub trait Substitution<T> {
    fn substitute(&self, object: &T) -> Option<T>;
}

pub struct SingleSubstitution<T> {
    old: T,
    new: T,
}

impl<T> SingleSubstitution<T> {
    pub fn new(old: T, new: T) -> Self {
        SingleSubstitution {
            old: old,
            new: new,
        }
    }
}

impl<T> Substitution<T> for SingleSubstitution<T>
    where T: PartialEq + Clone
{
    fn substitute(&self, object: &T) -> Option<T> {
        if &self.old == object {
            Some(self.new.clone())
        } else {
            None
        }
    }
}
