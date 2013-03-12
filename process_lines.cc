#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <set>
#include <map>
using namespace std;

void split(const string &s, char delim, vector<string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        if (item == " " || item == "")
            continue;
        elems.push_back(item);
    }
}

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
    for (map<string,int >::iterator it=singles_t_support.begin(); it!=singles_t_support.end(); ++it)
        cout << it->first << ":" << it->second << endl;
    for (map<string,int >::iterator it=pairs_t_support.begin(); it!=pairs_t_support.end(); ++it) {
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
    while (getline(cin, line)) {

        vector<string> tokens;
        split(line, ' ', tokens);
        if (tokens.size() == 7) {
            // Line specifies a caller
            caller = tokens[5];
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
            call_graphs[caller].insert(tokens[3]);
        }
        else if(tokens.size() == 0) {
            continue;
        }
    }

    process_t_support(call_graphs);
}
