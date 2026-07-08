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
    vector<int> solve_times_list; // Sorted descending for ranking
};

bool compareTeams(const Team* a, const Team* b) {
    if (a->solved_count != b->solved_count)
        return a->solved_count > b->solved_count;
    if (a->total_penalty != b->total_penalty)
        return a->total_penalty < b->total_penalty;
    
    for (size_t i = 0; i < a->solve_times_list.size(); ++i) {
        if (i >= b->solve_times_list.size()) return true; // a has more solved (though solved_count check handles this)
        if (a->solve_times_list[i] != b->solve_times_list[i]) 
            return a->solve_times_list[i] < b->solve_times_list[i];
    }
    
    return a->name < b->name;
}

void update_team_stats(Team& t) {
    t.solved_count = 0;
    t.total_penalty = 0;
    t.problem_solve_time.clear();
    t.problem_attempts.clear();
    t.solve_times_list.clear();

    map<char, int> first_ac_time;
    map<char, int> attempts_before_ac;

    // We only care about submissions that are NOT frozen for the current ranking
    // However, the Team struct stores all. The logic for "what is solved" 
    // depends on whether we are in a frozen state or not.
    // To handle this, we'll pass the "freeze_time" or a list of frozen problems.
}

// Since ranking depends on the freeze state, we need a way to calculate 
// "effective" stats based on whether a problem is frozen for a team.
struct EffectiveStats {
    int solved_count = 0;
    int total_penalty = 0;
    vector<int> solve_times;
    string name;

    bool operator<(const EffectiveStats& other) const {
        if (solved_count != other.solved_count) return solved_count > other.solved_count;
        if (total_penalty != other.total_penalty) return total_penalty < other.total_penalty;
        for (size_t i = 0; i < solve_times.size(); ++i) {
            if (i >= other.solve_times.size()) return false;
            if (solve_times[i] != other.solve_times[i]) return solve_times[i] < other.solve_times[i];
        }
        return name < other.name;
    }
};

EffectiveStats calculate_stats(const Team& t, const set<char>& frozen_probs) {
    EffectiveStats es;
    es.name = t.name;
    
    map<char, int> first_ac_time;
    map<char, int> attempts_before_ac;

    for (const auto& s : t.all_submissions) {
        if (frozen_probs.count(s.problem)) continue;
        if (first_ac_time.count(s.problem)) continue;
        if (s.status == "Accepted") {
            first_ac_time[s.problem] = s.time;
        }
    }

    for (const auto& s : t.all_submissions) {
        if (frozen_probs.count(s.problem)) continue;
        if (first_ac_time.count(s.problem)) {
            if (s.time < first_ac_time[s.problem] && s.status != "Accepted") {
                attempts_before_ac[s.problem]++;
            }
        }
    }

    for (auto const& [prob, time] : first_ac_time) {
        es.solved_count++;
        es.total_penalty += 20 * attempts_before_ac[prob] + time;
        es.solve_times.push_back(time);
    }
    sort(es.solve_times.rbegin(), es.solve_times.rend());
    
    return es;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string cmd;
    map<string, Team> teams_map;
    vector<string> team_names_order;
    bool started = false;
    bool frozen = false;
    int duration = 0, prob_count = 0;
    int freeze_time = -1;
    vector<string> last_flushed_ranking;

    while (cin >> cmd) {
        if (cmd == "ADDTEAM") {
            string name; cin >> name;
            if (started) {
                cout << "[Error]Add failed: competition has started.\n";
            } else if (teams_map.count(name)) {
                cout << "[Error]Add failed: duplicated team name.\n";
            } else {
                teams_map[name] = {name};
                team_names_order.push_back(name);
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
            teams_map[team_name].all_submissions.push_back({prob, status, time});
        } else if (cmd == "FLUSH") {
            vector<EffectiveStats> stats;
            set<char> current_frozen; // Flush doesn't unfreeze, but we need to know what's frozen
            // Actually, FLUSH just updates the ranking based on current frozen state.
            // If not frozen, current_frozen is empty. If frozen, we need to track which problems are frozen for which team.
            // Wait, the rules say: "Before the first scoreboard flush, rankings are based on the lexicographic order of team names."
            // And "rankings on the scoreboard will not change" after freezing until scrolling.
            
            // To implement this correctly, we need to know the "frozen" status of each problem per team.
            // Let's simplify: if not frozen, all problems are unfrozen. If frozen, problems unsolved before freeze are frozen.
            
            vector<EffectiveStats> current_stats;
            for (auto const& [name, t] : teams_map) {
                set<char> team_frozen;
                if (frozen) {
                    for (int p = 0; p < prob_count; ++p) {
                        char pc = 'A' + p;
                        bool solved_before = false;
                        for (auto& s : t.all_submissions) {
                            if (s.problem == pc && s.status == "Accepted" && s.time <= freeze_time) {
                                solved_before = true; break;
                            }
                        }
                        if (!solved_before) {
                            bool submitted_after = false;
                            for (auto& s : t.all_submissions) {
                                if (s.problem == pc && s.time > freeze_time) {
                                    submitted_after = true; break;
                                }
                            }
                            if (submitted_after) team_frozen.insert(pc);
                        }
                    }
                }
                current_stats.push_back(calculate_stats(t, team_frozen));
            }
            
            sort(current_stats.begin(), current_stats.end());
            last_flushed_ranking.clear();
            for (auto& s : current_stats) last_flushed_ranking.push_back(s.name);
            cout << "[Info]Flush scoreboard.\n";
        } else if (cmd == "FREEZE") {
            if (frozen) {
                cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            } else {
                frozen = true;
                // The freeze time is the time of the last submission before the FREEZE command
                freeze_time = -1;
                for (auto const& [name, t] : teams_map) {
                    for (auto& s : t.all_submissions) freeze_time = max(freeze_time, s.time);
                }
                cout << "[Info]Freeze scoreboard.\n";
            }
        } else if (cmd == "SCROLL") {
            if (!frozen) {
                cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            } else {
                cout << "[Info]Scroll scoreboard.\n";
                
                // 1. Flush first
                vector<EffectiveStats> current_stats;
                vector<set<char>> team_frozen_sets(teams_map.size());
                vector<string> team_names;
                
                int idx = 0;
                for (auto const& [name, t] : teams_map) {
                    team_names.push_back(name);
                    set<char> tf;
                    for (int p = 0; p < prob_count; ++p) {
                        char pc = 'A' + p;
                        bool solved_before = false;
                        for (auto& s : t.all_submissions) {
                            if (s.problem == pc && s.status == "Accepted" && s.time <= freeze_time) {
                                solved_before = true; break;
                            }
                        }
                        if (!solved_before) {
                            bool submitted_after = false;
                            for (auto& s : t.all_submissions) {
                                if (s.problem == pc && s.time > freeze_time) {
                                    submitted_after = true; break;
                                }
                            }
                            if (submitted_after) tf.insert(pc);
                        }
                    }
                    team_frozen_sets[idx++] = tf;
                    current_stats.push_back(calculate_stats(t, tf));
                }
                sort(current_stats.begin(), current_stats.end());

                auto print_scoreboard = [&](const vector<EffectiveStats>& sorted_s, const vector<set<char>>& f_sets) {
                    for (int i = 0; i < (int)sorted_s.size(); ++i) {
                        const string& name = sorted_s[i].name;
                        cout << name << " " << i + 1 << " " << sorted_s[i].solved_count << " " << sorted_s[i].total_penalty;
                        
                        // Find this team's frozen set
                        set<char> tf;
                        int t_idx = -1;
                        for(int j=0; j<(int)team_names.size(); ++j) if(team_names[j] == name) t_idx = j;
                        tf = f_sets[t_idx];
                        
                        const Team& t = teams_map[name];
                        for (int p = 0; p < prob_count; ++p) {
                            char pc = 'A' + p;
                            if (tf.count(pc)) {
                                int x = 0, y = 0;
                                for (auto& s : t.all_submissions) {
                                    if (s.problem == pc) {
                                        if (s.time <= freeze_time && s.status != "Accepted") x++;
                                        if (s.time > freeze_time) y++;
                                    }
                                }
                                if (x == 0) cout << " " << 0 << "/" << y;
                                else cout << " -" << x << "/" << y;
                            } else {
                                bool solved = false;
                                int first_ac_time = 2000000000;
                                for (auto& s : t.all_submissions) {
                                    if (s.problem == pc && s.status == "Accepted") {
                                        solved = true;
                                        first_ac_time = min(first_ac_time, s.time);
                                    }
                                }
                                if (solved) {
                                    int att = 0;
                                    for (auto& s : t.all_submissions) {
                                        if (s.problem == pc && s.status != "Accepted" && s.time < first_ac_time) att++;
                                    }
                                    cout << " +" << (att == 0 ? "" : to_string(att));
                                } else {
                                    int att = 0;
                                    for (auto& s : t.all_submissions) if (s.problem == pc) att++;
                                    if (att == 0) cout << " .";
                                    else cout << " -" << att;
                                }
                            }
                        }
                        cout << "\n";
                    }
                };

                print_scoreboard(current_stats, team_frozen_sets);

                // 2. Scroll process
                while (true) {
                    int lowest_team_idx = -1;
                    for (int i = (int)current_stats.size() - 1; i >= 0; --i) {
                        string name = current_stats[i].name;
                        int t_idx = -1;
                        for(int j=0; j<(int)team_names.size(); ++j) if(team_names[j] == name) t_idx = j;
                        if (!team_frozen_sets[t_idx].empty()) {
                            lowest_team_idx = i;
                            break;
                        }
                    }
                    if (lowest_team_idx == -1) break;

                    string team_name = current_stats[lowest_team_idx].name;
                    int t_idx = -1;
                    for(int j=0; j<(int)team_names.size(); ++j) if(team_names[j] == team_name) t_idx = j;
                    
                    char prob_to_unfreeze = *team_frozen_sets[t_idx].begin();
                    team_frozen_sets[t_idx].erase(prob_to_unfreeze);

                    // Recalculate all stats
                    vector<EffectiveStats> next_stats;
                    for (int j = 0; j < (int)team_names.size(); ++j) {
                        next_stats.push_back(calculate_stats(teams_map[team_names[j]], team_frozen_sets[j]));
                    }
                    sort(next_stats.begin(), next_stats.end());

                    // Check for ranking change
                    for (int i = 0; i < (int)current_stats.size(); ++i) {
                        if (next_stats[i].name != current_stats[i].name) {
                            // Find who moved up to this position
                            // The problem says: "team_name1 represents the team whose ranking increased... 
                            // team_name2 represents the team whose ranking was replaced by team_name1"
                            // This happens when we find the first index where the team is different.
                            // But we need to find the specific team that was unfrozen.
                            
                            // Let's find where team_name is now
                            int new_rank = -1;
                            for(int r=0; r<(int)next_stats.size(); ++r) if(next_stats[r].name == team_name) { new_rank = r; break; }
                            
                            if (new_rank < lowest_team_idx) {
                                string team_name2 = current_stats[new_rank].name;
                                cout << team_name << " " << team_name2 << " " << next_stats[new_rank].solved_count << " " << next_stats[new_rank].total_penalty << "\n";
                            }
                            break; 
                        }
                    }
                    current_stats = next_stats;
                }

                print_scoreboard(current_stats, team_frozen_sets);
                frozen = false;
            }
        } else if (cmd == "QUERY_RANKING") {
            string name; cin >> name;
            if (!teams_map.count(name)) {
                cout << "[Error]Query ranking failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query ranking.\n";
                if (frozen) cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
                int rank = -1;
                for (int i = 0; i < (int)last_flushed_ranking.size(); ++i) {
                    if (last_flushed_ranking[i] == name) { rank = i + 1; break; }
                }
                cout << name << " NOW AT RANKING " << rank << "\n";
            }
        } else if (cmd == "QUERY_SUBMISSION") {
            string name, where, prob_q, and_q, status_q;
            cin >> name >> where >> prob_q >> and_q >> status_q;
            if (!teams_map.count(name)) {
                cout << "[Error]Query submission failed: cannot find the team.\n";
            } else {
                cout << "[Info]Complete query submission.\n";
                Team& t = teams_map[name];
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
