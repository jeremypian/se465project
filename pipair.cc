#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iostream>
#include <assert.h>

using namespace std;

// May be set via commandline
int     T_THRESHOLD   = 3;
double  T_CONFIDENCE  = 0.65;
int T_DEPTH = 1;

// Keep a map of id --> string so main datastructures work with
// ids instead of full strings for better performance
typedef int64_t id;

static map<string, id> string_nodeid_map;
static map<id, string> reverse_map;
id get_id_for_string(const string& str) {
  static id current_count = 0;

  if (string_nodeid_map.count(str)) {
    return string_nodeid_map[str];
  }

  string_nodeid_map[str] = current_count;
  reverse_map[current_count] = str;
  return current_count++;
}

string& get_string_for_id(id i) {
  static string empty = "";
  if (reverse_map.count(i)) {
    return reverse_map[i];  
  }

  return empty;
}

/**
 * Helper that splits string s by a single delimiter delim, and returns the list
 * of tokens in a vector.
 */
void split(const string &s, char delim, vector<string>& elems) {
  std::stringstream ss(s);
  std::string item;
  elems.clear();
  while(std::getline(ss, item, delim)) {
    if (item == " " || item == "")
      continue;
    elems.push_back(item);
  }
}

/**
 * Finds potential bugs in the call for the given pair of ids and the support values
 * for the pair and the single ids.
 */

void find_bugs_for_pair(id primary_id, id other_id, const map<id, set<id> >& call_graphs,
    double pair_support, double single_support) {
  float confidence = pair_support / single_support;
  if (confidence >= T_CONFIDENCE) {
    map<id, set<id> >::const_iterator it = call_graphs.begin();
    for (; it!=call_graphs.end(); ++it) {
      const set<id>& callees = it->second;
      if(callees.find(primary_id) != callees.end()
          && callees.find(other_id) == callees.end()) {
        string first = get_string_for_id(primary_id);
        string second = get_string_for_id(other_id);
        cout << "bug: " << first << " in "
          << get_string_for_id(it->first) << ", "
          << "pair: (" << min(first, second) << " "
          << max(first, second)  << "), "
          << "support: " << (int)pair_support << ", "
          << "confidence: " << confidence * 100 << "%" << endl;
      }
    }
  }
}

/*
 * Find bugs given the program call_graphs, the single node support singles_t_support
 * and the pair node support pairs_t_support.
 */
void find_bugs(
  const map<id, set<id> >& call_graphs,
  const map<id, int>& singles_t_support,
  const map<pair<id, id>, int >& pairs_t_support
) {
  // delete pairs that have a support value that is less than the threshold
  cout.setf(ios::fixed);
  cout.precision(2);

  // Iterate through each pair and try and find bugs for pairs where the support
  // is larger than the provided threshold.
  for (map<pair<id, id>, int>::const_iterator it = pairs_t_support.begin();
      it != pairs_t_support.end(); ++it) {
    pair<id, id> id_pair = it->first;
    int pair_t_support = it->second;

    if (pair_t_support < T_THRESHOLD)
      continue;
   
    map<id, int>::const_iterator first_support = singles_t_support.find(id_pair.first);
    map<id, int>::const_iterator second_support = singles_t_support.find(id_pair.second);

    if (first_support == singles_t_support.end() ||
        second_support == singles_t_support.end()) {
      continue;
    }
    
    find_bugs_for_pair(id_pair.first, id_pair.second, call_graphs, pair_t_support,
        first_support->second);
    find_bugs_for_pair(id_pair.second, id_pair.first, call_graphs, pair_t_support,
        second_support->second);
  }
}

/**
 * Add this set of ids to the support of all pairs.
 */
void update_pair_support(const set<id>& ids, map<pair<id, id>, int>& support) {
  for (set<id>::const_iterator i = ids.begin(); i != ids.end(); i++) {
    for (set<id>::const_iterator j = i; j != ids.end(); j++) {
      if (*i < *j) {
        support[pair<id, id>(*i, *j)] += 1;
      }
    }
  }
}

/**
 * Add this set of ids to the support of all singles.
 */
void update_single_support(const set<id>& ids, map<id, int>& support) {
  for (set<id>::const_iterator i = ids.begin(); i != ids.end(); i++) {
    support[*i] += 1;
  }
}

/**
 * Expand the nodes for inter-procedure analysis.
 */
set<id>& expand_callees_helper(id target, const map<id, set<id> >& call_graphs, int depth, bool keep_expanded, map<id, set<id> >& caller_callees_map) {
  
  if (caller_callees_map.count(target)) {
    return caller_callees_map[target];
  }

  map<id, set<id> >::const_iterator child_itr = call_graphs.find(target);
  const set<id>& children = child_itr->second;
  if (depth == 0 || children.size() == 0) {
    caller_callees_map[target].insert(target);
  } else {
    // get set union of all children
    if (keep_expanded) {
      caller_callees_map[target].insert(children.begin(), children.end());
    }
    for (set<id>::const_iterator i = children.begin(); i != children.end(); i++) {
      set<id>& children_callees = expand_callees_helper(*i, call_graphs, depth - 1,
          keep_expanded, caller_callees_map);
      caller_callees_map[target].insert(children_callees.begin(), children_callees.end());
    }
  }

  return caller_callees_map[target];
}

set<id> expand_callees(id target, const map<id, set<id> >& call_graphs, int depth,
    bool keep_expanded) {
  map<id, set<id> > caller_callees_map;
  return expand_callees_helper(target, call_graphs, depth, keep_expanded,
      caller_callees_map);
}

/**
 * Process the call graph generated from the input call graph file and verify it for bugs.
 */
void process_t_support(const map<id, set<id> >& call_graphs) {
  map<pair<id, id>, int > pairs_t_support;
  map<id, int > singles_t_support;

  for (map<id, set<id> >::const_iterator it=call_graphs.begin(); it!=call_graphs.end(); ++it) {

    // update total counts for pair and single support
    const set<id> callees = T_DEPTH > 1 ? expand_callees(it->first, call_graphs, T_DEPTH, false) :
      it->second;

    /*cout << "Caller: " << get_string_for_id(it->first) << " => (";
    for (set<id>::const_iterator it=callees.begin(); it!=callees.end(); ++it)
      cout << get_string_for_id(*it) << "(" << *it << ")"<< ",";
    cout << ")" << endl;*/

    update_pair_support(callees, pairs_t_support);
    update_single_support(callees, singles_t_support);
  }

  find_bugs(call_graphs, singles_t_support, pairs_t_support);
}

inline bool is_null_caller(const string& line) {
  static const string null_function_node = "Call graph node <<null function>>";
  return line.find(null_function_node) != string::npos;
}

/**
 * Detects bugs in program by inferring likely invariants.
 */
void detect_bugs_in_callgraph(const vector<string>& file) {

  map<id, set<id> > call_graphs;

  // get rid of preceding text
  vector<string>::const_iterator line_itr = file.begin();
  string caller = "";
  while (line_itr != file.end() && is_null_caller(*line_itr)) line_itr++;

  for (;line_itr != file.end(); line_itr++) {
    string line = *line_itr;

    if (is_null_caller(line)) {
      caller = "";
    }

    vector<string> tokens;
    split(line, ' ', tokens);
    if (tokens.size() == 7) {
      // Line specifies a caller get the name of the caller inside the quotation marks
      caller = tokens[5].substr(1, tokens[5].find_last_of("'")-1);
    } else if(tokens.size() == 4) {
      // Line specifies a callee
      
      // Don't extract callee for null
      if (caller == "")
        continue;

      if (tokens[2] == "external")
        continue;
        
      string callee = tokens[3].substr(1, tokens[3].length()-2);
      // find the name of the callee and trim the quotes
      call_graphs[get_id_for_string(caller)].insert(get_id_for_string(callee));
    }
  }

  process_t_support(call_graphs);
}

enum {
  PIPE_READ = 0,
  PIPE_WRITE,
};

int main(int argc, char *argv[]) {

  /* pipe to connect opt's stderr and our stdin */
  int pipe_callgraph[2];

  /* pid of opt */
  int opt_pid;

  /* arguments */
  char *bc_file;

  /* check arguments */
  if (argc != 2 && argc != 4 && argc != 5) {
    exit(0);
  }

  bc_file = argv[1];

  if (argc >= 4) {
    T_THRESHOLD = atoi(argv[2]);
    T_CONFIDENCE = atof(argv[3]) / 100.0;
    assert(T_CONFIDENCE <= 1.0);
  }

  if (argc == 5) {
    T_DEPTH = atoi(argv[4]); 
  }

  /* create pipe and check if pipe succeeded */
  if (pipe(pipe_callgraph) < 0) {
    perror("pipe");
    return 1;
  }

  /* create child process */
  opt_pid = fork();
  if (!opt_pid) { /* child process, to spawn opt */

    /* close the read end, since opt only write */
    close(pipe_callgraph[PIPE_READ]);

    /* bind pipe to stderr, and check */
    if (dup2(pipe_callgraph[PIPE_WRITE], STDERR_FILENO) < 0) {
      perror("dup2 pipe_callgraph");
      return 1;
    }

    /* print something to stderr */
    fprintf(stderr, "This is child, just before spawning opt with %s.\n", bc_file);

    /* close stdout so we don't print the binary */
    close(STDOUT_FILENO);

    /* spawn opt */
    if (execl("/usr/local/bin/opt", "opt", "-print-callgraph", bc_file, (char *)NULL) < 0) {
      perror("execl opt");
      return 1;
    }

    /* unreachable code */
    return 0;
  }

  /* parent process */

  /* close the write end, since we only read */
  close(pipe_callgraph[PIPE_WRITE]);

  /* since we don't need stdin, we simply replace stdin with the pipe */
  if (dup2(pipe_callgraph[PIPE_READ], STDIN_FILENO) < 0) {
    perror("dup2 pipe_callgraph");
    return 1;
  }

  /* we print w/e read from the pipe */
	vector<string> file;
  string line = "";

  // create a call graph from the .cg file fed through the input
  while (getline(cin, line)) {
    file.push_back(line);
  }
  detect_bugs_in_callgraph(file);

  /* "That's all folks." */
  return 0;
}
