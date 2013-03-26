#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <set>
#include <map>
using namespace std;

// set these statically for now
int     T_THRESHOLD   = 3;
double  T_CONFIDENCE  = 0.65;

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
      cout << "callee:" << callee << " \n";
      if (singles_t_support.find(callee) == singles_t_support.end())
        singles_t_support[callee] = 1;
      else
        singles_t_support[callee]++;
    }
    else if (subset.size() == 2) {
      string callee_pair = "";
      for (set<string>::iterator it=subset.begin(); it!=subset.end(); ++it)
        callee_pair.append(*it);
      if (pairs_t_support.find(callee_pair) == pairs_t_support.end())
        pairs_t_support[callee_pair] = 1;
      else
        pairs_t_support[callee_pair]++;
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
    std::cout << it->first << endl;

    set<string> callees = it->second;
    vector<set<string> > subsets;
    get_subsets(callees, subsets);
    process_subsets(subsets, pairs_t_support, singles_t_support);
  }

  // Print potential bug in output
  map<string,int >::iterator it;
  vector<string> to_delete;
  for (it=singles_t_support.begin(); it!=singles_t_support.end(); ++it) {
    string pair = it->first;
    int t_support = it->second;

    cout << it->first << ":" << it->second << endl;
  }

  for (it=pairs_t_support.begin(); it!=pairs_t_support.end(); ++it) {
    string pair = it->first;
    int t_support = it->second;

    // delete pairs that have a support value that is less than the threshold
    if (t_support < T_THRESHOLD)
      to_delete.push_back(pair);
  }
  for (int i = 0; i<to_delete.size(); i++){
    pairs_t_support.erase(to_delete[i]);
  }

  for (it=pairs_t_support.begin(); it!=pairs_t_support.end(); ++it) {
    string pair = it->first;
    int t_support = it->second;
    cout << it->first << ":" << it->second << endl;
  }
}

/* Test subset func
int main(int argc, char *argv[]) {
  string strs[] = {"a", "b", "c"};
  set<string> myset(strs, strs + 3);
  vector<set<string> > subsets;
  get_subsets(myset, subsets);
  for (int i = 0; i<subsets.size(); i++) {
    set<string> subset = subsets[i];
    for (set<string>::iterator it=subset.begin(); it!=subset.end(); ++it)
      cout << ' ' << *it;
    cout << endl;
  }
}
*/
int main(int argc, char *argv[]) {
  string line;
  string caller = "";
  map<string, set<string> > call_graphs;

  // create a call graph from the .cg file fed through the input
  while (getline(cin, line)) {

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

      // find the name of the callee and trim the quotes
      string callee = tokens[3].substr(1, tokens[3].length()-2);
      call_graphs[caller].insert(callee);
    }
    else if(tokens.size() == 0) {
      continue;
    }
  }

  process_t_support(call_graphs);
}
