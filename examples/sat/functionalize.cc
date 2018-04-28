#include <cassert>
#include <cstdio>
#include <cstring>

#include <fstream>
#include <iostream>
#include <sstream>

#include <limbo/format/output.h>

#include "solver.h"

using namespace limbo;
using namespace limbo::format;

class Lit {
 public:
  Lit() = default;
  explicit Lit(int lit) : lit_(lit) {}

  bool operator==(const Lit lit) const { return lit_ == lit.lit_; }
  bool operator!=(const Lit lit) const { return !(*this == lit); }

  Lit flip() const { return Lit(-lit_); }
  bool pos() const { return lit_ > 0; }
  int var() const { return lit_ < 0 ? -lit_ : lit_; }
  int lit() const { return lit_; }

  bool null() const { return lit_ == 0; }
  int index() const { return var(); }

 private:
  int lit_ = 0;
};

class FLit {
 public:
  FLit() = default;
  FLit(int lhs, int rhs) : lhs_(lhs), rhs_(rhs) {}

  bool operator==(const FLit lit) const { return lhs_ == lit.lhs_ && rhs_ == lit.rhs_; }
  bool operator!=(const FLit lit) const { return !(*this == lit); }

  FLit flip() const { return FLit(-lhs_, rhs_); }
  bool pos() const { return lhs_ < 0; }
  int func() const { return lhs_ < 0 ? -lhs_ : lhs_; }
  int name() const { return rhs_; }

  bool null() const { return lhs_ == 0 && rhs_ == 0; }

  int lhs() const { return lhs_; }
  int rhs() const { return rhs_; }

 private:
  int lhs_ = 0;
  int rhs_ = 0;
};

std::ostream& operator<<(std::ostream& os, const Lit a) {
  os << a.lit();
  return os;
}

std::ostream& operator<<(std::ostream& os, const FLit a) {
  os << a.lhs() << '=' << a.rhs();
  return os;
}

static void LoadCnf(std::istream& stream, std::vector<std::vector<Lit>>* cnf) {
  cnf->clear();
  for (std::string line; std::getline(stream, line); ) {
    int n_funcs;
    int n_names;
    int n_clauses;
    if (line.length() == 0) {
      continue;
    } else if (line.length() >= 1 && line[0] == 'c') {
    } else if (sscanf(line.c_str(), "p cnf %d %d", &n_funcs, &n_clauses) == 2) {
    } else {
      std::vector<Lit> c;
      int i = -1;
      for (std::istringstream iss(line); (iss >> i) && i != 0; ) {
        c.push_back(Lit(i));
      }
      if (i == 0) {
        cnf->push_back(c);
      }
    }
  }
}

static std::vector<std::vector<FLit>> Functionalize(const std::vector<std::vector<Lit>>& cnf, int* n_funcs, int* n_names) {
  DenseMap<Lit, DenseMap<Lit, bool>> exclusive;
  DenseSet<Lit> props;
  int max_index = -1;

  for (const std::vector<Lit>& c : cnf) {
    for (const Lit lit : c) {
      if (max_index < lit.index()) {
        max_index = lit.index();
      }
    }
  }
  props.Capacitate(max_index);
  exclusive.Capacitate(max_index);
  for (auto& m : exclusive) {
    m.Capacitate(max_index);
  }
  for (int i = 0; i < cnf.size(); ++i) {
    auto& c = cnf[i];
    for (const Lit a : c) {
      props.Insert(a);
    }
    if (c.size() == 2 && !c[0].pos() && !c[1].pos() && c[0] != c[1]) {
      exclusive[c[0]][c[1]] = true;
      exclusive[c[1]][c[0]] = true;
    }
  }
  auto all_props = [&props](auto pred) { return std::all_of(props.begin(), props.end(), [&pred](const Lit a) { return a.null() || pred(a); }); };

  std::vector<Lit> process_queue;
  all_props([&](const Lit a) { process_queue.push_back(a); return true; });

  *n_funcs = 0;
  *n_names = 0;

  DenseMap<Lit, FLit> sub;
  sub.Capacitate(max_index);

  while (!process_queue.empty()) {
    const Lit cluster_root = process_queue.back();
    process_queue.pop_back();

    DenseSet<Lit> cluster;
    cluster.Capacitate(max_index);
    int cluster_size = 0;

    std::vector<Lit> cluster_queue;
    cluster_queue.push_back(cluster_root);

    while (!cluster_queue.empty()) {
      const Lit candidate = cluster_queue.back();
      cluster_queue.pop_back();
      if (!cluster.Contains(candidate) &&
          all_props([&](const Lit a) { return !cluster.Contains(a) || exclusive[candidate][a]; })) {
        cluster.Insert(candidate);
        ++cluster_size;
        all_props([&](const Lit a) { if (exclusive[candidate][a]) { cluster_queue.push_back(a); } return true; });
      }
    }

    for (const Lit a : cluster) {
      if (!a.null()) {
        for (const Lit b : cluster) {
          if (!b.null() && a != b) {
            exclusive[a][b] = false;
            exclusive[b][a] = false;
          }
        }
      }
    }

    if (cluster_size >= 2) {
      const int func = ++(*n_funcs);
      int i = 0;
      for (const Lit a : cluster) {
        if (!a.null()) {
          sub[a] = FLit(func, ++i);
        }
      }
      *n_names = std::max(*n_names, i);
    }
  }

  for (Lit a : props) {
    if (!a.null() && sub[a].null()) {
      sub[a] = FLit(++(*n_funcs), 1);
      *n_names = std::max(*n_names, 1);
    }
  }

  std::vector<std::vector<FLit>> fcnf;
  fcnf.reserve(cnf.size());
  for (const std::vector<Lit>& c : cnf) {
    if (!(c.size() == 2 && !c[0].pos() && !c[1].pos() && c[0] != c[1])) {
      std::vector<FLit> fc;
      fc.reserve(c.size());
      for (const Lit a : c) {
        const FLit b = sub[a];
        fc.push_back(a.pos() ? b : b.flip());
      }
      fcnf.push_back(fc);
    }
  }
  return fcnf;
}

int main(int argc, char *argv[]) {
  std::vector<std::vector<Lit>> cnf;
  for (int i = 1; i < argc; ++i) {
    std::ifstream ifs = std::ifstream(argv[i]);
    LoadCnf(ifs, &cnf);
  }
  if (cnf.empty()) {
    LoadCnf(std::cin, &cnf);
  }
  int funcs;
  int names;
  std::vector<std::vector<FLit>> fcnf = Functionalize(cnf, &funcs, &names);
  std::cout << "p fcnf " << funcs << " " << names << " " << fcnf.size() << std::endl;
  for (const std::vector<FLit>& fc : fcnf) {
    for (FLit a : fc) {
      std::cout << a << " ";
    }
    std::cout << " 0" << std::endl;
  }
}

