// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Copyright 2016 Christoph Schwering

#include <algorithm>
#include <iostream>
#include <list>
#include <functional>
#include <string>
#include <sstream>
#include <utility>

#include <emscripten.h>

#include <lela/setup.h>
#include <lela/formula.h>
#include <lela/format/output.h>
#include <lela/format/parser.h>

using lela::format::output::operator<<;

template<typename T>
inline std::string to_string(const T& x) {
  std::stringstream ss;
  ss << x;
  return ss.str();
}

inline void parse(const char* c_str) {
  typedef lela::format::pdl::Parser<std::string::const_iterator> StrParser;

  struct PrintAnnouncer : public lela::format::pdl::Announcer {
    void AnnounceEntailment(int k, const lela::Setup& s, const lela::Formula& phi, bool yes) override {
      std::string phi_str = to_string(phi);
      std::cout << "Setup = " << std::endl << s << std::endl;
      std::cout << "Entails(" << k << ", " << phi_str << ") = " << std::boolalpha << yes << std::endl;
      EM_ASM_({
        announceEntailment($0, Pointer_stringify($1), $2);
      }, k, phi_str.c_str(), yes);
    }

    void AnnounceConsistency(int k, const lela::Setup& s, const lela::Formula& phi, bool yes) override {
      std::string phi_str = to_string(phi);
      std::cout << "Setup = " << std::endl << s << std::endl;
      std::cout << "Consistent(" << k << ", " << phi_str << ") = " << std::boolalpha << yes << std::endl;
      EM_ASM_({
        announceConsistency($0, Pointer_stringify($1), $2);
      }, k, phi_str.c_str(), yes);
    }

    void AnnounceResult(const StrParser::Result<bool> r) const {
      bool success = bool(r);
      std::string r_str = to_string(r);
      std::cout << "Parser result: " << r_str << std::endl;
      EM_ASM_({
        announceResult($0, Pointer_stringify($1));
      }, success, r_str.c_str());
    }
  };

  std::string str = c_str;
  lela::format::pdl::Context ctx;
  PrintAnnouncer announcer;

  StrParser parser(str.begin(), str.end(), &ctx, &announcer);
  StrParser::Result<bool> r = parser.Parse();

  announcer.AnnounceResult(r);
}

extern "C" void lela_parse(const char* s) {
  return parse(s);
}

