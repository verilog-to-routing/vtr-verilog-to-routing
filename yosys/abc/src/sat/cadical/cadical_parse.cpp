#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

/*------------------------------------------------------------------------*/

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

// Parse error.

#define PER(...) \
  do { \
    internal->error_message.init ( \
        "%s:%" PRIu64 ": parse error: ", file->name (), \
        (uint64_t) file->lineno ()); \
    return internal->error_message.append (__VA_ARGS__); \
  } while (0)

/*------------------------------------------------------------------------*/

// Parsing utilities.

inline int Parser::parse_char () { return file->get (); }

// Return an non zero error string if a parse error occurred.

inline const char *Parser::parse_string (const char *str, char prev) {
  for (const char *p = str; *p; p++)
    if (parse_char () == *p)
      prev = *p;
    else if (*p == ' ')
      PER ("expected space after '%c'", prev);
    else
      PER ("expected '%c' after '%c'", *p, prev);
  return 0;
}

inline const char *Parser::parse_positive_int (int &ch, int &res,
                                               const char *name) {
  CADICAL_assert (isdigit (ch));
  res = ch - '0';
  while (isdigit (ch = parse_char ())) {
    int digit = ch - '0';
    if (INT_MAX / 10 < res || INT_MAX - digit < 10 * res)
      PER ("too large '%s' in header", name);
    res = 10 * res + digit;
  }
  return 0;
}

static const char *cube_token = "unexpected 'a' in CNF";

inline const char *Parser::parse_lit (int &ch, int &lit, int &vars,
                                      int strict) {
  if (ch == 'a')
    return cube_token;
  int sign = 0;
  if (ch == '-') {
    if (!isdigit (ch = parse_char ()))
      PER ("expected digit after '-'");
    sign = -1;
  } else if (!isdigit (ch))
    PER ("expected digit or '-'");
  else
    sign = 1;
  lit = ch - '0';
  while (isdigit (ch = parse_char ())) {
    int digit = ch - '0';
    if (INT_MAX / 10 < lit || INT_MAX - digit < 10 * lit)
      PER ("literal too large");
    lit = 10 * lit + digit;
  }
  if (ch == '\r')
    ch = parse_char ();
  if (ch != 'c' && ch != ' ' && ch != '\t' && ch != '\n' && ch != EOF)
    PER ("expected white space after '%d'", sign * lit);
  if (lit > vars) {
    if (strict != FORCED)
      PER ("literal %d exceeds maximum variable %d", sign * lit, vars);
    else
      vars = lit;
  }
  lit *= sign;
  return 0;
}

/*------------------------------------------------------------------------*/

// Parsing CNF in DIMACS format.

const char *Parser::parse_dimacs_non_profiled (int &vars, int strict) {

#ifndef CADICAL_QUIET
  double start = internal->time ();
#endif

  bool found_inccnf_header = false;
  int ch, clauses = 0;
  vars = 0;

  // First read comments before header with possibly embedded options.
  //
  for (;;) {
    ch = parse_char ();
    if (strict != STRICT)
      if (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r')
        continue;
    if (ch != 'c')
      break;
    string buf;
    while ((ch = parse_char ()) != '\n')
      if (ch == EOF)
        PER ("unexpected end-of-file in header comment");
      else if (ch != '\r')
        buf.push_back (ch);
    const char *o;
    for (o = buf.c_str (); *o && *o != '-'; o++)
      ;
    if (!*o)
      continue;
    PHASE ("parse-dimacs", "found option '%s'", o);
    if (*o)
      solver->set_long_option (o);
  }

  if (ch != 'p')
    PER ("expected 'c' or 'p'");

  ch = parse_char ();
  if (strict == STRICT) {
    if (ch != ' ')
      PER ("expected space after 'p'");
    ch = parse_char ();
  } else if (ch != ' ' && ch != '\t')
    PER ("expected white space after 'p'");
  else {
    do
      ch = parse_char ();
    while (ch == ' ' || ch == '\t');
  }

  // Now read 'p cnf <var> <clauses>' header of DIMACS file
  // or 'p inccnf' of incremental 'INCCNF' file.
  //
  if (ch == 'c') {
    CADICAL_assert (!found_inccnf_header);
    if (strict == STRICT) {
      const char *err = parse_string ("nf ", 'c');
      if (err)
        return err;
      ch = parse_char ();
      if (!isdigit (ch))
        PER ("expected digit after 'p cnf '");
      err = parse_positive_int (ch, vars, "<max-var>");
      if (err)
        return err;
      if (ch != ' ')
        PER ("expected ' ' after 'p cnf %d'", vars);
      if (!isdigit (ch = parse_char ()))
        PER ("expected digit after 'p cnf %d '", vars);
      err = parse_positive_int (ch, clauses, "<num-clauses>");
      if (err)
        return err;
      if (ch != '\n')
        PER ("expected new-line after 'p cnf %d %d'", vars, clauses);
    } else {
      if (parse_char () != 'n')
        PER ("expected 'n' after 'p c'");
      if (parse_char () != 'f')
        PER ("expected 'f' after 'p cn'");
      ch = parse_char ();
      if (!isspace (ch))
        PER ("expected space after 'p cnf'");
      do
        ch = parse_char ();
      while (isspace (ch));
      if (!isdigit (ch))
        PER ("expected digit after 'p cnf '");
      const char *err = parse_positive_int (ch, vars, "<max-var>");
      if (err)
        return err;
      if (!isspace (ch))
        PER ("expected space after 'p cnf %d'", vars);
      do
        ch = parse_char ();
      while (isspace (ch));
      if (!isdigit (ch))
        PER ("expected digit after 'p cnf %d '", vars);
      err = parse_positive_int (ch, clauses, "<num-clauses>");
      if (err)
        return err;
      while (ch != '\n') {
        if (ch != '\r' && !isspace (ch))
          PER ("expected new-line after 'p cnf %d %d'", vars, clauses);
        ch = parse_char ();
      }
    }

    MSG ("found %s'p cnf %d %d'%s header", tout.green_code (), vars,
         clauses, tout.normal_code ());

    if (strict != FORCED)
      solver->reserve (vars);
    internal->reserve_ids (clauses);
  } else if (!parse_inccnf_too)
    PER ("expected 'c' after 'p '");
  else if (ch == 'i') {
    found_inccnf_header = true;
    const char *err = parse_string ("nccnf", 'i');
    if (err)
      return err;
    ch = parse_char ();
    if (strict == STRICT) {
      if (ch != '\n')
        PER ("expected new-line after 'p inccnf'");
    } else {
      while (ch != '\n') {
        if (ch != '\r' && !isspace (ch))
          PER ("expected new-line after 'p inccnf'");
        ch = parse_char ();
      }
    }

    MSG ("found %s'p inccnf'%s header", tout.green_code (),
         tout.normal_code ());

    strict = FORCED;
  } else
    PER ("expected 'c' or 'i' after 'p '");

  if (parse_inccnf_too)
    *parse_inccnf_too = false;

  // Now read body of DIMACS part.
  //
  int lit = 0, parsed = 0;
  while ((ch = parse_char ()) != EOF) {
    if (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r')
      continue;
    if (ch == 'c') {
      while ((ch = parse_char ()) != '\n' && ch != EOF)
        ;
      if (ch == EOF)
        break;
      continue;
    }
    if (ch == 'a' && found_inccnf_header)
      break;
    const char *err = parse_lit (ch, lit, vars, strict);
    if (err)
      return err;
    if (ch == 'c') {
      while ((ch = parse_char ()) != '\n')
        if (ch == EOF)
          PER ("unexpected end-of-file in comment");
    }
    solver->add (lit);
    if (!found_inccnf_header && !lit && parsed++ >= clauses &&
        strict != FORCED)
      PER ("too many clauses");
  }

  if (lit)
    PER ("last clause without terminating '0'");

  if (!found_inccnf_header && parsed < clauses && strict != FORCED)
    PER ("clause missing");

#ifndef CADICAL_QUIET
  double end = internal->time ();
  MSG ("parsed %d clauses in %.2f seconds %s time", parsed, end - start,
       internal->opts.realtime ? "real" : "process");
#endif

#ifndef CADICAL_QUIET
  start = end;
  size_t num_cubes = 0;
#endif
  if (ch == 'a') {
    CADICAL_assert (parse_inccnf_too);
    CADICAL_assert (found_inccnf_header);
    if (!*parse_inccnf_too)
      *parse_inccnf_too = true;
    for (;;) {
      ch = parse_char ();
      if (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r')
        continue;
      if (ch == 'c') {
        while ((ch = parse_char ()) != '\n' && ch != EOF)
          ;
        if (ch == EOF)
          break;
        continue;
      }
      const char *err = parse_lit (ch, lit, vars, strict);
      if (err == cube_token)
        PER ("two 'a' in a row");
      else if (err)
        return err;
      if (ch == 'c') {
        while ((ch = parse_char ()) != '\n')
          if (ch == EOF)
            PER ("unexpected end-of-file in comment");
      }
      if (cubes)
        cubes->push_back (lit);
      if (!lit) {
#ifndef CADICAL_QUIET
        num_cubes++;
#endif
        for (;;) {
          ch = parse_char ();
          if (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r')
            continue;
          if (ch == 'c') {
            while ((ch = parse_char ()) != '\n' && ch != EOF)
              ;
            if (ch == EOF)
              break;
          }
          if (ch == EOF)
            break;
          if (ch != 'a')
            PER ("expected 'a' or end-of-file after zero");
          lit = INT_MIN;
          break;
        }
        if (ch == EOF)
          break;
      }
    }
    if (lit)
      PER ("last cube without terminating '0'");
  }
#ifndef CADICAL_QUIET
  if (found_inccnf_header) {
    double end = internal->time ();
    MSG ("parsed %zd cubes in %.2f seconds %s time", num_cubes, end - start,
         internal->opts.realtime ? "real" : "process");
  }
#endif

  return 0;
}

/*------------------------------------------------------------------------*/

// Parsing solution in competition output format.

const char *Parser::parse_solution_non_profiled () {
  external->solution = new signed char[external->max_var + 1u];
  external->solution_size = external->max_var;
  clear_n (external->solution, external->max_var + 1u);
  int ch;
  for (;;) {
    ch = parse_char ();
    if (ch == EOF)
      PER ("missing 's' line");
    else if (ch == 'c') {
      while ((ch = parse_char ()) != '\n')
        if (ch == EOF)
          PER ("unexpected end-of-file in comment");
    } else if (ch == 's')
      break;
    else
      PER ("expected 'c' or 's'");
  }
  const char *err = parse_string (" SATISFIABLE", 's');
  if (err)
    return err;
  if ((ch = parse_char ()) == '\r')
    ch = parse_char ();
  if (ch != '\n')
    PER ("expected new-line after 's SATISFIABLE'");
#ifndef CADICAL_QUIET
  int count = 0;
#endif
  for (;;) {
    ch = parse_char ();
    if (ch != 'v')
      PER ("expected 'v' at start-of-line");
    if ((ch = parse_char ()) != ' ')
      PER ("expected ' ' after 'v'");
    int lit = 0;
    ch = parse_char ();
    do {
      if (ch == ' ' || ch == '\t') {
        ch = parse_char ();
        continue;
      }
      err = parse_lit (ch, lit, external->max_var, false);
      if (err)
        return err;
      if (ch == 'c')
        PER ("unexpected comment");
      if (!lit)
        break;
      if (external->solution[abs (lit)])
        PER ("variable %d occurs twice", abs (lit));
      LOG ("solution %d", lit);
      external->solution[abs (lit)] = sign (lit);
#ifndef CADICAL_QUIET
      count++;
#endif
      if (ch == '\r')
        ch = parse_char ();
    } while (ch != '\n');
    if (!lit)
      break;
  }
  MSG ("parsed %d values %.2f%%", count,
       percent (count, external->max_var));
  return 0;
}

/*------------------------------------------------------------------------*/

// Wrappers to profile parsing and at the same time use the convenient
// implicit 'return' in PER in the non-profiled versions.

const char *Parser::parse_dimacs (int &vars, int strict) {
  CADICAL_assert (strict == FORCED || strict == RELAXED || strict == STRICT);
  START (parse);
  const char *err = parse_dimacs_non_profiled (vars, strict);
  STOP (parse);
  return err;
}

const char *Parser::parse_solution () {
  START (parse);
  const char *err = parse_solution_non_profiled ();
  STOP (parse);
  return err;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
