// clang-format off

const char *usage =
"usage: babysat [ <option> ... ] [ <dimacs> ]\n"
"\n"
"where '<option>' can be one of the following\n"
"\n"
"  -h | --help        print this command line option summary\n"
#ifdef LOGGING
"  -l | --logging     print very verbose logging information\n"
#endif
"  -q | --quiet       do not print any messages\n"
"  -n | --no-witness  do not print witness if satisfiable\n"
"  -v | --verbose     print verbose messages\n"
"\n"
"and '<dimacs>' is the input file in DIMACS format.  The solver\n"
"reads from '<stdin>' if no input file is specified.\n";

// clang-format on

#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

// Linux/Unix system specific.

#include <sys/resource.h>
#include <sys/time.h>

// Global options accessible through the command line.

static bool witness = true;

static int verbosity; // -1=quiet, 0=normal, 1=verbose, INT_MAX=logging

// Global options fixed at compile time.

struct Clause {
#ifndef NDEBUG
  size_t id;
#endif
  unsigned size;
  int literals[];

  // The following two functions allow simple ranged-based for-loop
  // iteration over Clause literals with the following idiom:
  //
  //   Clause *c = ...
  //   for (auto lit : *c)
  //     do_something_with (lit);
  //
  int *begin() { return literals; }
  int *end() { return literals + size; }
};

static int variables;       // Variable range: 1,..,<variables>
static int initial_clauses; // Number of initial clauses
static signed char *values; // Lit assignment 0=unassigned,-1=false,1=true.
static unsigned *levels;    // Maps variables to their level
static Clause **reasons;    // Maps variables to their reasons (clauses)
static bool *seen;

static std::vector<Clause *> clauses;
static std::vector<Clause *> *occurrences;

static Clause *empty_clause; // Empty clause found.

static std::vector<int> trail;
static std::vector<size_t> control;


static unsigned level;    // Decision level.
static size_t propagated; // Next position on trail to propagate.

// Decision Heuristics:

static double *activity; //activity score for each variable
static double var_inc = 1; //value to increase activities
static double var_decay = 0.95; //decay value for activites
static int decay_interval = 50; //conflicts before applying decay

// Statistics:

static size_t added;        // Number of added clauses.
static size_t conflicts;    // Number of conflicts.
static size_t decisions;    // Number of decisions.
static size_t propagations; // Number of propagated literals.
static size_t reports;      // Number of calls to 'report'.
static int fixed;           // Number of root-level assigned variables.

// Get process-time of this process.  This is not portable to Windows but
// should work on other Unixes such as MacOS as is.

static double process_time(void) {
  struct rusage u;
  double res;
  if (getrusage(RUSAGE_SELF, &u))
    return 0;
  res = u.ru_utime.tv_sec + 1e-6 * u.ru_utime.tv_usec;
  res += u.ru_stime.tv_sec + 1e-6 * u.ru_stime.tv_usec;
  return res;
}

// Report progress once in a while.

static void report(char type) {
  if (verbosity < 0)
    return;
  if (!(reports++ % 20))
    printf("c\n"
           "c             decisions             variables\n"
           "c   seconds              conflicts            remaining\n"
           "c\n");
  int remaining = variables - fixed;
  printf("c %c %7.2f %11zu %11zu %9d %3.0f%%\n", type, process_time(),
         decisions, conflicts, remaining,
         variables ? 100.0 * remaining / variables : 0);
  fflush(stdout);
}

// Generates nice compiler warnings if format string does not fit arguments.

static void message(const char *, ...) __attribute__((format(printf, 1, 2)));
static void die(const char *, ...) __attribute__((format(printf, 1, 2)));

static void parse_error(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

#ifdef LOGGING

static void debug(const char *, ...) __attribute__((format(printf, 1, 2)));

static void debug(Clause *, const char *, ...)
    __attribute__((format(printf, 2, 3)));

static bool logging() { return verbosity == INT_MAX; }

// Print debugging message if '--debug' is used.  This is only enabled
// if the solver is configured with './configure --logging' (which is the
// default for './configure --debug').  Even if logging code is included
// this way, it still needs to be enabled at run-time through '-l'.

static char debug_buffer[4][32];
static size_t next_debug_buffer;

// Get a statically allocate string buffer.
// Used here only for printing literals.

static char *debug_string(void) {
  char *res = debug_buffer[next_debug_buffer++];
  if (next_debug_buffer == sizeof debug_buffer / sizeof *debug_buffer)
    next_debug_buffer = 0;
  return res;
}

static char *debug(int lit) {
  if (!logging())
    return 0;
  char *res = debug_string();
  sprintf(res, "%d", lit);
  int value = values[lit];
  if (value) {
    size_t len = strlen(res);
    size_t remaining = sizeof debug_buffer[0] - len;
    snprintf(res + len, remaining, "@%u=%d", levels[abs(lit)], value);
  }
  assert(strlen(res) <= sizeof debug_buffer[0]);
  return res;
}

static void debug_prefix(void) { printf("c DEBUG %u ", level); }

static void debug_suffix(void) {
  fputc('\n', stdout);
  fflush(stdout);
}

static void debug(const char *fmt, ...) {
  if (!logging())
    return;
  debug_prefix();
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  debug_suffix();
}

static void debug(Clause *c, const char *fmt, ...) {
  if (!logging())
    return;
  debug_prefix();
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  printf(" size %u clause[%zu]", c->size, c->id);
  for (auto lit : *c)
    printf(" %s", debug(lit));
  debug_suffix();
}

#else

#define debug(...)                                                             \
  do {                                                                         \
  } while (0)

#endif

// Print message to '<stdout>' and flush it.

static void message(const char *fmt, ...) {
  if (verbosity < 0)
    return;
  fputs("c ", stdout);
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  fputc('\n', stdout);
  fflush(stdout);
}

static void line() {
  if (verbosity < 0)
    return;
  fputs("c\n", stdout);
  fflush(stdout);
}

static void verbose(const char *fmt, ...) {
  if (verbosity <= 0)
    return;
  fputs("c ", stdout);
  va_list ap;
  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
  fputc('\n', stdout);
  fflush(stdout);
}

// Print error message and 'die'.

static void die(const char *fmt, ...) {
  fprintf(stderr, "babysat: error: ");
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  exit(1);
}

static void initialize(void) {
  assert(variables < INT_MAX);
  unsigned size = variables + 1;

  unsigned twice = 2 * size;

  values = new signed char[twice]();
  occurrences = new std::vector<Clause *>[twice];

  levels = new unsigned[size];
  activity = new double[size](); //initialize with 0
  reasons = new Clause*[size];
  seen = new bool[size](); //initialize with false

  // We subtract 'variables' in order to be able to access
  // the arrays with a negative index (valid in C/C++).

  occurrences += variables;
  values += variables;

  // for (int lit = -variables; lit <= variables; lit++) values[lit] = 0;

  assert(!propagated);
  assert(!level);
}

static void delete_clause(Clause *c) {
  debug(c, "delete");
  delete[] c;
}

static void release(void) {
  for (auto c : clauses)
    delete_clause(c);

  occurrences -= variables;
  values -= variables;

  delete[] occurrences;
  delete[] values;

  delete[] levels;
  delete[] activity;
  delete[] reasons;
  delete[] seen;
}

static bool satisfied(Clause *c) {
  for (auto lit : *c)
    if (values[lit] > 0)
      return true;
  return false;
}

// Check whether all clauses are satisfied.
// DONE implement satisfied ()
static bool satisfied() {
  for (auto c : clauses)
    if (!satisfied(c))
      return false;
  return true;
}

// DONE implement assign (int lit)
static void assign(int lit, Clause *reason) {
  // Set 'values[lit]' and 'values[-lit]'.
  values[lit] = 1;
  values[-lit] = -1;

  levels[abs(lit)] = level;
  reasons[abs(lit)] = reason;
  // Push literal on trail.
  trail.push_back(lit);
  // If root-level (so level == 0) increase fixed.
  if(level == 0) fixed++;

  debug("assign %s", debug(lit));
}

static void connect_literal(int lit, Clause *c) {
  debug(c, "connecting %s to", debug(lit));
  occurrences[lit].push_back(c);
}

// if a unit clause is added, it is assigned. The parameter reason is the
// set as the reason for this assignment
static Clause *add_clause(std::vector<int> &literals, Clause *reason) {
  size_t size = literals.size();
  size_t bytes = sizeof(struct Clause) + size * sizeof(int);
  Clause *c = (Clause *)new char[bytes];

#ifndef NDEBUG
  c->id = added;
#endif
  added++;

  assert(clauses.size() <= (size_t)INT_MAX);
  c->size = size;

  int *q = c->literals;
  for (auto lit : literals)
    *q++ = lit;

  debug(c, "new");

  clauses.push_back(c); // Save it on global stack of clauses.

  // Connect the literals of the clause in the matrix.

  for (auto lit : *c)
    connect_literal(lit, c);

  // Handle the special case of empty and unit clauses.

  if (!size) {
    debug(c, "parsed empty clause");
    empty_clause = c;
  } else if (size == 1) {
    int unit = literals[0];
    signed char value = values[unit];
    if (!value)
      assign(unit, reason);
    else if (value < 0) {
      debug(c, "inconsistent unit clause");
      empty_clause = c;
    }
  }

  return c;
}

static const char *file_name;
static bool close_file;
static FILE *file;

static void parse_error(const char *fmt, ...) {
  fprintf(stderr, "babysat: parse error in '%s': ", file_name);
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fputc('\n', stderr);
  exit(1);
}

static void parse(void) {
  int ch;
  while ((ch = getc(file)) == 'c') {
    while ((ch = getc(file)) != '\n')
      if (ch == EOF)
        parse_error("end-of-file in comment");
  }
  if (ch != 'p')
    parse_error("expected 'c' or 'p'");
  int clauses;
  if (fscanf(file, " cnf %d %d", &variables, &clauses) != 2 || variables < 0 ||
      variables >= INT_MAX || clauses < 0 || clauses >= INT_MAX)
    parse_error("invalid header");
  message("parsed header 'p cnf %d %d'", variables, clauses);
  initialize();
  std::vector<int> clause;
  int lit = 0, parsed = 0;
  size_t literals = 0;
  while (fscanf(file, "%d", &lit) == 1) {
    if (parsed == clauses)
      parse_error("too many clauses");
    if (lit == INT_MIN || abs(lit) > variables)
      parse_error("invalid literal '%d'", lit);
    if (lit) {
      clause.push_back(lit);
      literals++;
    } else {
      add_clause(clause, nullptr);
      clause.clear();
      parsed++;
    }
  }
  if (lit)
    parse_error("terminating zero missing");
  if (parsed != clauses)
    parse_error("clause missing");
  if (close_file)
    fclose(file);
  verbose("parsed %zu literals in %d clauses", literals, parsed);
}

static void bump_activity(int var) {
  activity[var] += var_inc;
  if(activity[var] > 1e100 ) {
    // Rescale:
    for (int v = 1; v <= variables; v++)
        activity[v] *= 1e-100;
    var_inc *= 1e-100; 
  }
}

static void decay_activity() {
  if(conflicts % decay_interval == 0){
    var_inc /= var_decay;
  }
}

// Return the conflict clause if propagation detects an empty clause otherwise if it
// completes propagating all literals since the last time it was called
// without finding an empty clause it returns nullptr.  Beside finding
// conflicts propagating a literal also detects unit clauses and
// then assign the forced literal by that unit clause.

// DONE implement propagate ()
static Clause* propagate(void) {
  // While not all literals propagated.
  while(propagated < trail.size()){
    // Propagated next literal 'lit' on trail.
    int lit = trail.at(propagated);
    // Go over all clauses in which '-lit' occurs.
    for(auto c : occurrences[-lit]){
      // For each clause check whether it is satisfied, falsified, or forcing.
      int unassignedLit = 0;
      int unassignedCount = 0;
      bool falsified = true;
      bool satisfied = false;
      for(auto lit : *c){
        if(values[lit] > 0) {
          satisfied = true;
          break;
        }
        if(values[lit] == 0){
          falsified = false;
          unassignedCount++;
          unassignedLit = lit;
          if(unassignedCount >= 2) break;
        }
      }
      if(satisfied) continue;
      // If clause falsified return 'false' (increase 'conflicts').
      if(falsified){
        conflicts++;
        return c;
      }
      // If forcing assign the forced unit.
      if(unassignedCount == 1){
        assign(unassignedLit, c);
      }
    }

    // Increase 'propagations'.
    propagations++;
    propagated++;
  }
  // If all literals propagated without finding a falsified clause (?no? conflict):
  return nullptr;
}

static int is_power_of_two(size_t n) { return n && !(n & (n - 1)); }

// DONE implement decide ()
static int decide(void) {
  decisions++;

  int res = 0;
  double max = -1;
  // Find a variable/literal which is not assigned yet.
  for(int v = 1; v <= variables; v++) {
    if(values[v] == 0){
      if(activity[v] > max){
        max = activity[v];
        if(occurrences[v].size() > occurrences[-v].size()){
          res = v;
        }else{
          res = -v;
        }
      }
    }
  }

  if(res == 0) return res;
  // Increase decision level.
  level++;
  // Save the current trail on the control stack for backtracking.
  control.push_back(trail.size());
  // Assign the picked decision literal.
  assign(res, nullptr);

  if (is_power_of_two(decisions))
    report('d');
  return res;
}

// DONE implement unassign (int lit)
static void unassign(int lit) {
  debug("unassign %s", debug(lit));
  // Reset 'values[lit]' and 'values[-lit]'.
  values[lit] = 0;
  values[-lit] = 0;
  reasons[abs(lit)] = nullptr;
}

static void backjump(unsigned bjlevel) {
  assert(level);
  debug("backjumping to level %d", bjlevel);

  // the decision we want to backjump to on the trail
  unsigned bjindex = 0;
  while(level > bjlevel){
    bjindex = control.back();
    control.pop_back();
    level--;
  }

  size_t size = trail.size();
  for(size_t i = bjindex; i < size; i++){
    int lit = trail.back();
    trail.pop_back();
    unassign(lit);
  }

  //propagate the backjump level again
  propagated = control.empty() ? 0 : control.back();
}

// finds the first UIP clause, adds the literals of the minimized UIP clause to minimized
// returns the backjump level
static void analyze(Clause *conflict, std::vector<int> &minimized) {
  int index = trail.size()-1;
  int to_resolve = 0;
  int uip = 0;
  std::vector<int> learned;

  do{
    // resolve with current clause
    if(conflict != nullptr){
      for(auto lit : *conflict){
        if(lit == uip) continue;
        int var = abs(lit);
        if(!seen[var] && levels[var] > 0){
          seen[var] = true;
          // if var was assigned at the current decision level it needs to be resolved
          // otherwise add the literal to the learned clause
          if(levels[var] >= level){
            to_resolve++;
          }else{
            learned.push_back(lit);
          }
        }
      }
    }
    
    //find next unseen variable
    while (!seen[abs(trail[index])]) index--;
    uip = trail[index--];
    int uip_var = abs(uip);
    conflict = reasons[uip_var];
    seen[uip_var] = false;
    to_resolve--;
  }while(to_resolve > 0);

  //bump activity
  for(auto lit : learned){
    int var = abs(lit);
    bump_activity(var);
  }

  learned.push_back(-uip);
  bump_activity(abs(uip));
  seen[abs(uip)] = true;
  
  // all variables of the UIP clause are marked as seen
  // only add variables with unseen antecedents to minimized clause
  for(auto lit : learned) {
    int var = abs(lit);
    if(reasons[var] == nullptr){
      minimized.push_back(lit);
    }else{
      bool redundant = true;
      for(auto antecedent : *reasons[var]){
        if(!seen[abs(antecedent)]){
          redundant = false;
          break;
        }
      }
      if(!redundant){
        minimized.push_back(lit);
      }
    }
  }

  // reset seen
  for(auto lit : learned){
    seen[abs(lit)] = false;
  }
}

unsigned find_backjump_level(std::vector<int> literals){
  unsigned bjlevel = 0;
  for(auto lit : literals){
    int var = abs(lit);
    if(levels[var] < level && levels[var] > bjlevel){
      bjlevel = levels[var];
    }
  }

  return bjlevel;
}

// The SAT competition standardized exit codes (the 'exit (code)' or 'return
// res' in 'main').  All other exit codes denote unsolved or error.

static const int satisfiable = 10;   // Exit code for satisfiable and
static const int unsatisfiable = 20; // unsatisfiable formulas.

static int cdcl(void) {
  Clause *conflict;
  std::vector<int> learned;

  while(true){
    conflict = propagate();
    if(conflict != nullptr){
      debug(conflict, "conflict clause");
      decay_activity();
      //conflict at level 0 => UNSAT
      if(level == 0) return unsatisfiable;
      learned.clear();
      analyze(conflict, learned);
      backjump(find_backjump_level(learned));
      //assign asserting literal
      Clause *c = add_clause(learned, conflict);
      int asserting = learned.back();
      if (values[asserting] == 0)
          assign(asserting, c);
    }else{
      //if all variables are assigned, return SAT
      if(!decide()) return satisfiable;
    }
  }
}

static int solve(void) {
  if (empty_clause)
    return unsatisfiable;
  return cdcl();
}

// Checking the model on the original formula is extremely useful for
// testing and debugging.  This 'checker' aborts if an unsatisfied clause is
// found and prints the clause on '<stderr>' for debugging purposes.

static void check_model(void) {
  debug("checking model");
  for (auto c : clauses) {
    if (satisfied(c))
      continue;
    fputs("babysat: unsatisfied clause:\n", stderr);
    for (auto lit : *c)
      fprintf(stderr, "%d ", lit);
    fputs("0\n", stderr);
    fflush(stderr);
    abort();
    exit(1);
  }
}

// Printing the model in the format of the SAT competition, e.g.,
//
//   v -1 2 3 0
//
// Always prints a full assignments even if not all values are set.

static void print_model(void) {
  printf("v ");
  for (int idx = 1; idx <= variables; idx++) {
    if (values[idx] < 0)
      printf("-");
    printf("%d ", idx);
  }
  printf("0\n");
}

static double average(double a, double b) { return b ? a / b : 0; }

// The main function expects at most one argument which is then considered
// as the path to a DIMACS file. Without argument the solver reads from
// '<stdin>' (the standard input connected for instance to the terminal).

static void print_statistics() {
  double t = process_time();
  printf("c %-15s %16zu %12.2f per second\n", "conflicts:", conflicts,
         average(conflicts, t));
  printf("c %-15s %16zu %12.2f per second\n", "decisions:", decisions,
         average(decisions, t));
  printf("c %-15s %16zu %12.2f million per second\n",
         "propagations:", propagations, average(propagations * 1e-6, t));
  printf("c\n");
  printf("c %-15s %16.2f seconds\n", "process-time:", t);
}

#include "config.hpp"

int main(int argc, char **argv) {
  for (int i = 1; i != argc; i++) {
    const char *arg = argv[i];
    if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
      fputs(usage, stdout);
      exit(0);
    } else if (!strcmp(arg, "-l") || !strcmp(arg, "--logging"))
#ifdef LOGGING
      verbosity = INT_MAX;
#else
      die("compiled without logging code (use './configure --logging')");
#endif
    else if (!strcmp(arg, "-q") || !strcmp(arg, "--quiet"))
      verbosity = -1;
    else if (!strcmp(arg, "-v") || !strcmp(arg, "--verbose"))
      verbosity = 1;
    else if (!strcmp(arg, "-n") || !strcmp(arg, "--no-witness"))
      witness = false;
    else if (arg[0] == '-')
      die("invalid option '%s' (try '-h')", arg);
    else if (file_name)
      die("too many arguments '%s' and '%s' (try '-h')", file_name, arg);
    else
      file_name = arg;
  }

  if (!file_name) {
    file_name = "<stdin>";
    assert(!close_file);
    file = stdin;
  } else if (!(file = fopen(file_name, "r")))
    die("could not open and read '%s'", file_name);
  else
    close_file = true;

  message("BabySAT DPLL SAT Solver");
  line();
  message("Copyright (c) 2026, Benjamin Deutsch");
  message("Version %s %s", VERSION, GITID);
  message("Compiled with '%s'", BUILD);
  line();
  message("reading from '%s'", file_name);

  parse();
  report('*');

  int res = solve();
  report(res == 10 ? '1' : res == 20 ? '0' : '?');
  line();

  if (res == 10) {
    check_model();
    printf("s SATISFIABLE\n");
    if (witness)
      print_model();
  } else if (res == 20)
    printf("s UNSATISFIABLE\n");

  release();

  if (verbosity >= 0)
    line(), print_statistics(), line();

  message("exit code %d", res);

  return res;
}
