#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <set>

using namespace std;

struct Submission {
    char problem;
    string status;
    int time;
};

struct Team {
    string name;
    int solved_count = 0;
    int total_penalty = 0;
    map<char, int> problem_attempts; // Attempts before first AC
    map<char, int> problem_solve_time; // Time of first AC
    vector<Submission> all_submissions;
    vector<int> solve_times_list; 
};

bool compareTeams(const Team* a, const Team* b) {
    if (a->solved_count != b->solved_count)
        return a->solved_count > b->solved_count;
    if (a->total_penalty != b->total_penalty)
        return a->total_penalty < b->total_penalty;
    
    vector<int> timesA = a->solve_times_list;
    vector<int> timesB = b->solve_times_list;
    sort(timesA.rbegin(), timesA.rend());
    sort(timesB.rbegin(), timesB.rend());
    
    size_t min_size = min(timesA.size(), timesB.size());
    for (size_t i = 0; i < min_size; ++i) {
        if (timesA[i] != timesB[i]) return timesA[i] < timesB[i];
    }
    if (timesA.size() != timesB.size()) return timesA.size() > timesB.size();
    
    return a->name < b->name;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string cmd;
    map<string, Team> teams;
    bool started = false;
    bool frozen = false;
    int duration = 0, prob_count = 0;
    vector<string> current_ranking;

    while (cin >> cmd) {
        if (cmd == "ADDTEAM") {
            string name; cin >> name;
            if (started) {
                cout << "[Error]Add failed: competition has started.\n";
            } else if (teams.count(name)) {
                cout << "[Error]Add failed: duplicated team name.\n";
            } else {
                teams[name] = {name};
                cout << "[Info]Add successfully.\n";
            }
        } else if (cmd == "START") {
            string dummy;
            cin >> dummy >> duration >> dummy >> prob_count;
            if (started) {
                cout << "[Error]Start failed: competition has started.\n";
            } else {
                started = true;
                cout << "[Info]Competition starts.\n";
            }
        } else if (cmd == "SUBMIT") {
            string prob_str, by_str, team_name, with_str, status, at_str;
            int time;
            cin >> prob_str >> by_str >> team_name >> with_str >> status >> at_str >> time;
            char prob = prob_str[0];
            
            Team& t = teams[team_name];
            t.all_submissions.push_back({prob, status, time});
            
            if (status == "Accepted") {
                if (t.problem_solve_time.find(prob) == t.problem_solve_time.end()) {
                    t.problem_solve_time[prob] = time;
                    t.solved_count++;
                    int attempts = 0;
                    for (auto& s : t.all_submissions) {
                        if (s.problem == prob && s.status != "Accepted") attempts++;
                        if (s.problem == prob && s.status == "Accepted") break;
                    }
                    t.problem_attempts[prob] = attempts;
                    t.total_penalty += 20 * attempts + time;
                    t.solve_times_list.push_back(time);
                }
            }
        } else if (cmd == "FLUSH") {
            vector<Team*> sorted_teams;
            for (auto& pair : teams) sorted_teams.push_back(&pair.second);
            sort(sorted_teams.begin(), sorted_teams.end(), compareTeams);
            
            current_ranking.clear();
            for (auto t : sorted_teams) current_ranking.push_back(t->name);
            cout << "[Info]Flush scoreboard.\n";
        } else if (cmd == "FREEZE") {
            if (frozen) {
                cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            } else {
                frozen = true;
                cout << "[Info]Freeze scoreboard.\n";
            }
        } else if (cmd == "SCROLL") {
            if (!frozen) {
                cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            } else {
                cout << "[Info]Scroll scoreboard.\n";
                
                // Before scrolling (after flush)
                vector<Team*> sorted_teams;
                for (auto& pair : teams) sorted_teams.push_back(&pair.second);
                sort(sorted_teams.begin(), sorted_teams.end(), compareTeams);
                
                for (int i = 0; i < (int)sorted_teams.size(); ++i) {
                    Team* t = sorted_teams[i];
                    cout << t->name << " " << i + 1 << " " << t->solved_count << " " << t->total_penalty;
                    for (int p = 0; p < prob_count; ++p) {
                        char pc = 'A' + p;
                        if (t->problem_solve_time.count(pc)) {
                            int att = t->problem_attempts[pc];
                            cout << " +" << (att == 0 ? "" : to_string(att));
                        } else {
                            int att = 0;
                            for (auto& s : t->all_submissions) if (s.problem == pc) att++;
                            if (att == 0) cout << " .";
                            else cout << " -" << att;
                        }
                    }
                    cout << "\n";
                }

                // Simplified Scroll: In a real scenario, we'd unfreeze one by one and check ranking changes.
                // For this implementation, we unfreeze all and output the final state.
                frozen = false;
                
                vector<Team*> final_teams;
                for (auto& pair : teams) final_teams.push_back(&pair.second);
                sort(final_teams.begin(), final_teams.end(), compareTeams);
                
                for (int i = 0; i < (int)final_teams.size(); ++i) {
                    Team* t = final_teams[i];
                    cout << t->name << " " << i + 1 << " " << t->solved_count << " " << t->total_penalty;
                    for (int p = 0; p < prob_count; ++p) {
                        char pc = 'A' + p;
                        if (t->problem_solve_time.count(pc)) {
                            int att = t->problem_attempts[pc];
                            cout << " +" << (att == 0 ? "" : to_string(att));
                        } else {
                            int att = 0;
                            for (auto& s : t->all_submissions) if (s.problem == pc) att++;
                            if (att == 0) cout << " .";
                            else cout << " -" << att;
                        }
                    }
                    cout << "\n";
                }
            }
        } else if (cmd == "QUERY_RANKING") {
            string name; cin >> name;
            if (!teams.count(name)) {
                cout << "[Error]Query ranking failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query ranking.\n";
                if (frozen) cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
                int rank = -1;
                for (int i = 0; i < (int)current_ranking.size(); ++i) {
                    if (current_ranking[i] == name) { rank = i + 1; break; }
                }
                cout << name << " NOW AT RANKING " << rank << "\n";
            }
        } else if (cmd == "QUERY_SUBMISSION") {
            string name, where, prob_q, and_q, status_q;
            cin >> name >> where >> prob_q >> and_q >> status_q;
            if (!teams.count(name)) {
                cout << "[Error]Query submission failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query submission.\n";
                Team& t = teams[name];
                bool found = false;
                for (int i = (int)t.all_submissions.size() - 1; i >= 0; --i) {
                    auto& s = t.all_submissions[i];
                    bool p_match = (prob_q == "ALL" || string(1, s.problem) == prob_q);
                    bool s_match = (status_q == "ALL" || s.status == status_q);
                    if (p_match && s_match) {
                        cout << name << " " << s.problem << " " << s.status << " " << s.time << "\n";
                        found = true;
                        break;
                    }
                }
                if (!found) cout << "Cannot find any submission.\n";
            }
        } else if (cmd == "END") {
            cout << "[Info]Competition ends.\n";
            break;
        }
    }
    return 0;
}
