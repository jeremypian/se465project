#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <assert.h>

using namespace std;

// May be set via commandline
int     T_THRESHOLD   = 3;
double  T_CONFIDENCE  = 0.65;

enum
{
  PIPE_READ = 0,
  PIPE_WRITE,
};

/* helper that splits string s by a single delimiter delim, and returns the list
 *of tokens in a vector
 */
void split(const string &s, char delim, vector<string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while(std::getline(ss, item, delim)) {
    if (item == " " || item == "")
      continue;
    elems.push_back(item);
  }
}

/* helper that computes all subsets of the set callees, and then return the
 * result via the reference vector subsets.
 */
void get_subsets(set<string> callees, vector<set<string> > &subsets) {
  if (callees.size() == 1) {
    subsets.push_back(set<string>());
    subsets.push_back(callees);
    return;
  }

  set<string>::iterator it = callees.begin();
  string single = *it;
  callees.erase(single);
  get_subsets(callees, subsets);

  int subset_size = subsets.size();
  for (int i = 0; i<subset_size; i++) {
    set<string> copy(subsets[i]);
    copy.insert(single);
    subsets.push_back(copy);
  }
}

/* Given the subset subsets of callees, iterate through them and increment the
 * count of T-support for single and pair callees.
 */
void process_subsets(
  vector<set<string> > &subsets,
  map<string, int > &pairs_t_support,
  map<string, int > &singles_t_support
)
{
  for (int i = 0; i<subsets.size(); i++) {
    set<string> subset = subsets[i];
    if (subset.size() == 1) {
      set<string>::iterator it = subset.begin();
      string callee = *it;
      //cout << "callee:" << callee << " \n";

      // continue if the node is an external node
      if (callee == "")
        continue;

      if (singles_t_support.find(callee) == singles_t_support.end())
        singles_t_support[callee] = 1;
      else
        singles_t_support[callee]++;
    }
    else if (subset.size() == 2) {
      string callee_pair = "";
      set<string>::iterator it=subset.begin();
      callee_pair.append(*it++);
      callee_pair.append(",");
      callee_pair.append(*it);
      if (pairs_t_support.find(callee_pair) == pairs_t_support.end())
        pairs_t_support[callee_pair] = 1;
      else
        pairs_t_support[callee_pair]++;
    }
  }
}

/*
 * Generate the bug report based on the support of callees
 */
void find_bugs(
  map<string, set<string> > call_graphs,
  map<string, int > singles_t_support,
  map<string, int > pairs_t_support
) {
  // delete pairs that have a support value that is less than the threshold
  cout.setf(ios::fixed);
  cout.precision(2);
  map<string,int >::iterator it;
  for (it=pairs_t_support.begin(); it!=pairs_t_support.end(); ++it) {
    vector<string> tokens;
    string pair = it->first;
    int pair_t_support = it->second;
    if (pair_t_support < T_THRESHOLD)
      continue;

    split(pair, ',', tokens);
    for (int i = 0; i<2; i++) {
      int single_t_support = singles_t_support[tokens[i]];
      double confidence = double(pair_t_support) / double(single_t_support);
      //cout << pair_t_support << ", " << single_t_support << endl;
      if (confidence >= T_CONFIDENCE) {
        string func_name = "";
        map<string,set<string> >::iterator it=call_graphs.begin();
        for (; it!=call_graphs.end(); ++it) {
          set<string> callees = it->second;
          if(callees.find(tokens[i]) != callees.end()
            && callees.find(tokens[1-i]) == callees.end()) {
            cout << "bug: " << tokens[i] << " in " << it->first << ", "
            << "pair: (" << tokens[0] << " " << tokens[1] << "), "
            << "support: " << pair_t_support << ", "
            << "confidence: " << confidence * 100 << "%" << endl;
          }
        }
      }
    }
  }
}

/* Process the call graph generated from the input call graph file
 * Args:
 *   call_graphs - a map of callers to their respective callees
 */
void process_t_support(map<string, set<string> > call_graphs) {
  map<string, int > pairs_t_support;
  map<string, int > singles_t_support;

  for (map<string,set<string> >::iterator it=call_graphs.begin(); it!=call_graphs.end(); ++it) {
    set<string> callees = it->second;

		/*
    cout << "Caller: " << it->first << " => (";
    for (set<string>::iterator it=callees.begin(); it!=callees.end(); ++it)
      cout << *it << ",";
    cout << ")" << endl;
		*/

    vector<set<string> > subsets;
    get_subsets(callees, subsets);
    process_subsets(subsets, pairs_t_support, singles_t_support);
  }

  find_bugs(call_graphs, singles_t_support, pairs_t_support);
}
int main(int argc, char *argv[]) {
  /* pipe to connect opt's stderr and our stdin */
  int pipe_callgraph[2];

  /* pid of opt */
  int opt_pid;

  /* arguments */
  char *bc_file;

  /* check arguments */
  if (argc == 2) {
    bc_file = argv[1];
  }
  else if (argc == 4) {
    T_THRESHOLD = atoi(argv[2]);
    T_CONFIDENCE = atof(argv[3]) / 100.0;
    assert(T_CONFIDENCE <= 1.0);
    cout << T_THRESHOLD << endl;
    cout << T_CONFIDENCE << endl;
  }
  else {
    exit(0);
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
  char c = '\0';
  string line = "";

  // create a call graph from the .cg file fed through the input
  while (scanf("%c", &c) >= 1) {
    if(c != '\n') {
        line += c;
    }
		else {
			file.push_back(line);
			line = "";
		}
  }

  string caller = "";
	bool first_line_found = false;
  map<string, set<string> > call_graphs;
	for (int i = 0; i<file.size(); i++) {
		string line = file[i];
		if (!first_line_found && line.find("Call graph node <<null function>>") == string::npos)
			continue;
		else if (!first_line_found && line.find("Call graph node <<null function>>") != string::npos)
			first_line_found = true;

		vector<string> tokens;
		split(line, ' ', tokens);
		if (tokens.size() == 7) {
			// Line specifies a caller get the name of the caller inside the quotation marks
			caller = tokens[5].substr(1, tokens[5].find_last_of("'")-1);
			call_graphs[caller] = set<string>();
		}
		else if(tokens.size() == 6) {
			// Line specifies the root caller
			// Do nothing for now
		}
		else if(tokens.size() == 4) {
			// Line specifies a callee
			if (caller == "")
				continue;

			string callee;
			if (tokens[2] == "external")
				callee = "";
			else
				callee = tokens[3].substr(1, tokens[3].length()-2);
			// find the name of the callee and trim the quotes
			call_graphs[caller].insert(callee);
		}
		else if(tokens.size() == 0) {
			continue;
		}
	}

  process_t_support(call_graphs);

  /* "That's all folks." */
  return 0;
}
