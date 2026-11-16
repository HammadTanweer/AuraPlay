#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#endif

#include "httplib.h"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <cmath>
#include <iomanip>
#include <cctype>
#include <algorithm>

using namespace std;

// ==========================================
// Position Enum for clean type safety
// ==========================================
enum Position { ATTACKER, MIDFIELDER, DEFENDER, GOALKEEPER };

string positionToString(Position pos) {
    switch (pos) {
        case ATTACKER: return "Attacker";
        case MIDFIELDER: return "Midfielder";
        case DEFENDER: return "Defender";
        case GOALKEEPER: return "Goalkeeper";
        default: return "Unknown";
    }
}

Position stringToPosition(const string& str) {
    string lower = str;
    for (char& c : lower) c = tolower(c);
    if (lower == "attacker" || lower == "att" || lower == "0") return ATTACKER;
    if (lower == "midfielder" || lower == "mid" || lower == "1") return MIDFIELDER;
    if (lower == "defender" || lower == "def" || lower == "2") return DEFENDER;
    if (lower == "goalkeeper" || lower == "gk" || lower == "3") return GOALKEEPER;
    return ATTACKER;
}

// Case-insensitive string comparison helper
bool equalsIgnoreCase(const string& a, const string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (tolower(static_cast<unsigned char>(a[i])) != tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

// ==========================================
// Struct encapsulating match statistics
// ==========================================
struct MatchStats {
    int minutesPlayed;
    int goals;
    int assists;
    int tackles;
    int saves;
    int goalsConceded;
    int teamGoalsAgainst;
};

// Helper function to clamp ratings between 1.0 and 10.0
double clampRating(double rating) {
    if (rating < 1.0) return 1.0;
    if (rating > 10.0) return 10.0;
    return rating;
}

// ==========================================
// 1. CUSTOM EXCEPTION CLASS
// ==========================================
class OveruseFatigueException {
private:
    string message;
public:
    OveruseFatigueException(const string& playerName, int fatigue) {
        message = "[CRITICAL MEDICAL ALERT] Player '" + playerName + "' has breached maximum fatigue (" 
                  + to_string(fatigue) + "/100). Player is BENCHED for recovery!";
    }
    string getMessage() const {
        return message;
    }
};

// ==========================================
// 2. STRATEGY PATTERN FOR POSITION-SPECIFIC RATINGS
// ==========================================
class IRatingStrategy {
public:
    virtual ~IRatingStrategy() {}
    virtual double calculateRating(const MatchStats& stats) const = 0;
};

class AttackerRatingStrategy : public IRatingStrategy {
public:
    double calculateRating(const MatchStats& stats) const override {
        if (stats.minutesPlayed == 0) return 0.0;
        double base = 6.0;
        base += (stats.goals * 1.5) + (stats.assists * 1.0);
        double timeFactor = stats.minutesPlayed / 90.0;
        return clampRating(base * timeFactor);
    }
};

class DefenderRatingStrategy : public IRatingStrategy {
public:
    double calculateRating(const MatchStats& stats) const override {
        if (stats.minutesPlayed == 0) return 0.0;
        double base = 6.0;
        
        // Defensive core duties
        base += (stats.tackles * 0.4);
        
        // Attacking contributions (Bonus for defenders)
        base += (stats.goals * 2.0);    
        base += (stats.assists * 1.2);  
        
        // Team defensive context
        if (stats.teamGoalsAgainst == 0) base += 1.5;
        else base -= (stats.teamGoalsAgainst * 0.5);

        return clampRating(base);
    }
};

class GoalkeeperRatingStrategy : public IRatingStrategy {
public:
    double calculateRating(const MatchStats& stats) const override {
        if (stats.minutesPlayed == 0) return 0.0;
        double base = 6.0;
        base += (stats.saves * 0.5);

        if (stats.teamGoalsAgainst == 0) base += 2.0;
        else base -= (stats.goalsConceded * 0.8);

        return clampRating(base);
    }
};

// ==========================================
// 3. BASE CLASS: Person (Abstract Class)
// ==========================================
class Person {
protected:
    string name;
    int age;

public:
    Person(string n, int a) : name(n), age(a) {}
    virtual ~Person() {}

    virtual void displayInfo() const = 0;

    string getName() const { return name; }
    int getAge() const { return age; }
};

// ==========================================
// 4. DERIVED CLASS: Player (Encapsulation, Rule of Three, Operator Overloading)
// ==========================================
class Player : public Person {
private:
    Position position;
    int matchesPlayed;
    int totalFatigue;
    double totalRating;
    IRatingStrategy* ratingStrategy; 

    void setStrategy() {
        if (ratingStrategy != nullptr) {
            delete ratingStrategy;
        }

        switch (position) {
            case ATTACKER:
            case MIDFIELDER:
                ratingStrategy = new AttackerRatingStrategy();
                break;
            case DEFENDER:
                ratingStrategy = new DefenderRatingStrategy();
                break;
            case GOALKEEPER:
                ratingStrategy = new GoalkeeperRatingStrategy();
                break;
        }
    }

public:
    Player(string n, int a, Position pos) 
        : Person(n, a), position(pos), matchesPlayed(0), totalFatigue(0), totalRating(0.0), ratingStrategy(nullptr) {
        setStrategy();
    }

    Player(string n, int a, Position pos, int matches, int fatigue, double rating) 
        : Person(n, a), position(pos), matchesPlayed(matches), totalFatigue(fatigue), totalRating(rating), ratingStrategy(nullptr) {
        setStrategy();
    }

    // Copy Constructor (Rule of Three)
    Player(const Player& other) 
        : Person(other.name, other.age), position(other.position), 
          matchesPlayed(other.matchesPlayed), totalFatigue(other.totalFatigue), totalRating(other.totalRating), ratingStrategy(nullptr) {
        setStrategy();
    }

    // Copy Assignment Operator (Rule of Three)
    Player& operator=(const Player& other) {
        if (this != &other) {
            name = other.name;
            age = other.age;
            position = other.position;
            matchesPlayed = other.matchesPlayed;
            totalFatigue = other.totalFatigue;
            totalRating = other.totalRating;
            setStrategy();
        }
        return *this;
    }

    // Destructor (Rule of Three) - cleans up dynamically allocated IRatingStrategy
    ~Player() override {
        delete ratingStrategy;
    }

    // Operator Overloading: Compares two players based on average rating
    bool operator>(const Player& other) const {
        return this->getAverageRating() > other.getAverageRating();
    }

    double computeMatchRating(const MatchStats& stats) const {
        return ratingStrategy->calculateRating(stats);
    }

    void playMatch(double matchRating, int minutesPlayed, int exertionScale) {
        matchesPlayed++;
        totalRating += matchRating;
        int fatigueGained = static_cast<int>((minutesPlayed / 90.0) * exertionScale * 5);
        totalFatigue += fatigueGained;
    }

    bool needsRest() const { return totalFatigue >= 100; }
    void rest() { totalFatigue = 0; }

    Position getPosition() const { return position; }
    int getMatchesPlayed() const { return matchesPlayed; }
    int getTotalFatigue() const { return totalFatigue; }
    double getTotalRating() const { return totalRating; }

    double getAverageRating() const {
        return (matchesPlayed > 0) ? (totalRating / matchesPlayed) : 0.0;
    }

    string getFatigueStatus() const {
        if (totalFatigue < 40) return "[FRESH - GREEN]";
        if (totalFatigue < 75) return "[MODERATE - YELLOW]";
        if (totalFatigue < 100) return "[HIGH RISK - AMBER]";
        return "[CRITICAL - RED]";
    }

    void displayInfo() const override {
        cout << "Player Name    : " << name << endl;
        cout << "Position       : " << positionToString(position) << endl;
        cout << "Age            : " << age << endl;
        cout << "Matches Played : " << matchesPlayed << endl;
        cout << "Average Rating : " << getAverageRating() << "/10.0" << endl;
        cout << "Fatigue Level  : " << totalFatigue << "/100 " << getFatigueStatus() << endl;
        cout << "-----------------------------------------" << endl;
    }
};

// ==========================================
// 5. MANAGER CLASS: TeamManager
// ==========================================
class TeamManager {
private:
    vector<Player> team;

public:
    void addPlayer(const string& name, int age, Position pos) {
        team.emplace_back(name, age, pos);
        cout << "Player " << name << " (" << positionToString(pos) << ") added to the team!" << endl;
    }

    // Duplicate check: same name (case-insensitive), same age, and same position
    bool hasPlayer(const string& name, int age, Position pos) const {
        for (const auto& p : team) {
            if (p.getAge() == age && p.getPosition() == pos && equalsIgnoreCase(p.getName(), name)) {
                return true;
            }
        }
        return false;
    }

    // Erase a player by index from std::vector<Player> (triggering Player::~Player())
    bool removePlayerByIndex(size_t index) {
        if (index < team.size()) {
            cout << "Erasing player " << team[index].getName() << " (calling Player::~Player())..." << endl;
            team.erase(team.begin() + index);
            return true;
        }
        return false;
    }

    void simulateTeamMatch() {
        if (team.empty()) {
            cout << "No players in the team! Add players first." << endl;
            return;
        }

        int teamGoalsAgainst;
        cout << "\n=== ENTER MATCH CONTEXT ===" << endl;
        cout << "Enter total goals conceded by the team in this match: ";
        cin >> teamGoalsAgainst;

        for (size_t i = 0; i < team.size(); i++) {
            cout << "\n-----------------------------------------" << endl;
            cout << "Evaluating Player: " << team[i].getName() 
                 << " (" << positionToString(team[i].getPosition()) << ")" << endl;
            
            try {
                if (team[i].needsRest()) {
                    throw OveruseFatigueException(team[i].getName(), team[i].getTotalFatigue());
                }

                MatchStats stats{};
                stats.teamGoalsAgainst = teamGoalsAgainst;

                cout << "Enter minutes played (1-90): ";
                cin >> stats.minutesPlayed;

                if (team[i].getPosition() == ATTACKER || team[i].getPosition() == MIDFIELDER) {
                    cout << "Enter goals scored: "; cin >> stats.goals;
                    cout << "Enter assists: "; cin >> stats.assists;
                } 
                else if (team[i].getPosition() == DEFENDER) {
                    cout << "Enter successful tackles: "; cin >> stats.tackles;
                    cout << "Enter goals scored (if any): "; cin >> stats.goals;
                    cout << "Enter assists (if any): "; cin >> stats.assists;
                } 
                else if (team[i].getPosition() == GOALKEEPER) {
                    cout << "Enter saves made: "; cin >> stats.saves;
                    cout << "Enter goals conceded by goalkeeper: "; cin >> stats.goalsConceded;
                }

                int exertion;
                cout << "Enter perceived exertion rate (1-10): ";
                cin >> exertion;

                double rating = team[i].computeMatchRating(stats);
                cout << "-> Calculated Match Rating: " << rating << "/10.0" << endl;

                team[i].playMatch(rating, stats.minutesPlayed, exertion);

            } catch (const OveruseFatigueException& e) {
                cout << e.getMessage() << endl; 
                team[i].rest();
                cout << "-> " << team[i].getName() << " has recovered during the match (Fatigue reset to 0/100)." << endl;
            }
        }
        cout << "\nMatch simulation complete!" << endl;
    }

    void showTeamStats() {
        if (team.empty()) {
            cout << "No players in the team!" << endl;
            return;
        }

        cout << "\n================ AURAPLAY: TEAM STATISTICS ================" << endl;
        for (size_t i = 0; i < team.size(); i++) {
            Person* p = &team[i];
            p->displayInfo();
        }
    }

    void showMVP() {
        if (team.empty()) {
            cout << "No players in the team to determine MVP!" << endl;
            return;
        }

        size_t mvpIndex = 0;
        for (size_t i = 1; i < team.size(); i++) {
            if (team[i] > team[mvpIndex]) {
                mvpIndex = i;
            }
        }

        cout << "\n=========================================" << endl;
        cout << "       AURAPLAY: TEAM MVP SELECTION      " << endl;
        cout << "=========================================" << endl;
        team[mvpIndex].displayInfo();
    }

    void restPlayer() {
        if (team.empty()) {
            cout << "No players in the team!" << endl;
            return;
        }

        cout << "\nSelect a player to rest:" << endl;
        for (size_t i = 0; i < team.size(); i++) {
            cout << i + 1 << ". " << team[i].getName() 
                 << " (Fatigue: " << team[i].getTotalFatigue() << " " << team[i].getFatigueStatus() << ")" << endl;
        }

        int index;
        cout << "Enter player number: ";
        cin >> index;

        if (index >= 1 && index <= (int)team.size()) {
            team[index - 1].rest();
            cout << team[index - 1].getName() << " is now fully rested (Fatigue reset to 0)!" << endl;
        } else {
            cout << "Invalid player number!" << endl;
        }
    }

    bool saveToFile(const string& filename = "team_data.txt") {
        ofstream outFile(filename);
        if (!outFile) {
            cout << "Error opening file for saving!" << endl;
            return false;
        }

        outFile << team.size() << "\n";
        for (size_t i = 0; i < team.size(); i++) {
            outFile << team[i].getName() << "\n";
            outFile << team[i].getAge() << " "
                    << static_cast<int>(team[i].getPosition()) << " "
                    << team[i].getMatchesPlayed() << " "
                    << team[i].getTotalFatigue() << " "
                    << team[i].getTotalRating() << "\n";
        }

        outFile.close();
        cout << "Team data saved to " << filename << " successfully!" << endl;
        return true;
    }

    bool loadFromFile(const string& filename = "team_data.txt") {
        ifstream inFile(filename);
        if (!inFile) {
            cout << "Error: " << filename << " not found!" << endl;
            return false;
        }

        team.clear();
        int count = 0;
        if (!(inFile >> count)) return false;
        inFile.ignore();

        for (int i = 0; i < count; i++) {
            string name;
            int age = 0, posInt = 0, matches = 0, fatigue = 0;
            double rating = 0.0;

            if (!getline(inFile, name)) break;
            if (!(inFile >> age >> posInt >> matches >> fatigue >> rating)) break;
            inFile.ignore();

            Player loadedPlayer(name, age, static_cast<Position>(posInt), matches, fatigue, rating);
            team.push_back(loadedPlayer);
        }

        inFile.close();
        cout << "Team data loaded from " << filename << " successfully!" << endl;
        return true;
    }

    // ==========================================
    // Web REST API Accessor Methods
    // ==========================================
    const vector<Player>& getTeam() const { return team; }
    vector<Player>& getTeam() { return team; }

    const Player* getMVP(size_t* outIndex = nullptr) const {
        if (team.empty()) return nullptr;
        size_t mvpIndex = 0;
        for (size_t i = 1; i < team.size(); i++) {
            // Operator Overloading (operator>) used here to determine MVP
            if (team[i] > team[mvpIndex]) {
                mvpIndex = i;
            }
        }
        if (outIndex) *outIndex = mvpIndex;
        return &team[mvpIndex];
    }

    bool restPlayerByIndex(size_t index) {
        if (index < team.size()) {
            team[index].rest();
            return true;
        }
        return false;
    }
};

// ==========================================
// Lightweight C++11 JSON Parser & Serializer
// ==========================================
struct Json {
    enum Type { NUL, BOOL, NUMBER, STRING, ARRAY, OBJECT } type = NUL;
    bool b_val = false;
    double n_val = 0.0;
    string s_val;
    vector<Json> arr_val;
    map<string, Json> obj_val;

    Json() : type(NUL) {}
    Json(Type t) : type(t) {}
    Json(bool b) : type(BOOL), b_val(b) {}
    Json(int n) : type(NUMBER), n_val(static_cast<double>(n)) {}
    Json(size_t n) : type(NUMBER), n_val(static_cast<double>(n)) {}
    Json(double n) : type(NUMBER), n_val(n) {}
    Json(const string& s) : type(STRING), s_val(s) {}
    Json(const char* s) : type(STRING), s_val(s ? s : "") {}

    static void skip_ws(const string& s, size_t& i) {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) i++;
    }

    static string parse_str(const string& s, size_t& i) {
        string res;
        i++; // skip initial '"'
        while (i < s.size()) {
            char c = s[i++];
            if (c == '"') return res;
            if (c == '\\' && i < s.size()) {
                char esc = s[i++];
                if (esc == '"') res += '"';
                else if (esc == '\\') res += '\\';
                else if (esc == '/') res += '/';
                else if (esc == 'b') res += '\b';
                else if (esc == 'f') res += '\f';
                else if (esc == 'n') res += '\n';
                else if (esc == 'r') res += '\r';
                else if (esc == 't') res += '\t';
                else res += esc;
            } else {
                res += c;
            }
        }
        return res;
    }

    static Json parse_val(const string& s, size_t& i) {
        skip_ws(s, i);
        if (i >= s.size()) return Json();
        char c = s[i];
        if (c == '"') {
            Json j;
            j.type = STRING;
            j.s_val = parse_str(s, i);
            return j;
        } else if (c == '{') {
            Json j;
            j.type = OBJECT;
            i++; // skip '{'
            while (true) {
                skip_ws(s, i);
                if (i >= s.size() || s[i] == '}') { if (i < s.size()) i++; break; }
                if (s[i] != '"') break;
                string key = parse_str(s, i);
                skip_ws(s, i);
                if (i < s.size() && s[i] == ':') i++;
                Json val = parse_val(s, i);
                j.obj_val[key] = val;
                skip_ws(s, i);
                if (i < s.size() && s[i] == ',') i++;
                else if (i < s.size() && s[i] == '}') { i++; break; }
            }
            return j;
        } else if (c == '[') {
            Json j;
            j.type = ARRAY;
            i++; // skip '['
            while (true) {
                skip_ws(s, i);
                if (i >= s.size() || s[i] == ']') { if (i < s.size()) i++; break; }
                j.arr_val.push_back(parse_val(s, i));
                skip_ws(s, i);
                if (i < s.size() && s[i] == ',') i++;
                else if (i < s.size() && s[i] == ']') { i++; break; }
            }
            return j;
        } else if (s.compare(i, 4, "true") == 0) {
            i += 4; return Json(true);
        } else if (s.compare(i, 5, "false") == 0) {
            i += 5; return Json(false);
        } else if (s.compare(i, 4, "null") == 0) {
            i += 4; return Json();
        } else if (c == '-' || (c >= '0' && c <= '9')) {
            size_t start = i;
            if (s[i] == '-') i++;
            while (i < s.size() && (isdigit(s[i]) || s[i] == '.' || s[i] == 'e' || s[i] == 'E' || s[i] == '+' || s[i] == '-')) i++;
            double num = 0.0;
            try { num = stod(s.substr(start, i - start)); } catch (...) {}
            return Json(num);
        }
        return Json();
    }

    static Json parse(const string& s) {
        size_t i = 0;
        return parse_val(s, i);
    }

    string dump() const {
        if (type == NUL) return "null";
        if (type == BOOL) return b_val ? "true" : "false";
        if (type == NUMBER) {
            ostringstream ss;
            if (n_val == static_cast<long long>(n_val)) {
                ss << static_cast<long long>(n_val);
            } else {
                ss << fixed << setprecision(2) << n_val;
            }
            return ss.str();
        }
        if (type == STRING) {
            string res = "\"";
            for (char c : s_val) {
                if (c == '"') res += "\\\"";
                else if (c == '\\') res += "\\\\";
                else if (c == '\n') res += "\\n";
                else if (c == '\r') res += "\\r";
                else if (c == '\t') res += "\\t";
                else res += c;
            }
            res += "\"";
            return res;
        }
        if (type == ARRAY) {
            string res = "[";
            for (size_t i = 0; i < arr_val.size(); i++) {
                if (i > 0) res += ",";
                res += arr_val[i].dump();
            }
            res += "]";
            return res;
        }
        if (type == OBJECT) {
            string res = "{";
            size_t count = 0;
            for (auto const& kv : obj_val) {
                if (count > 0) res += ",";
                res += "\"" + kv.first + "\":" + kv.second.dump();
                count++;
            }
            res += "}";
            return res;
        }
        return "null";
    }

    bool has(const string& key) const {
        return type == OBJECT && obj_val.find(key) != obj_val.end();
    }

    Json operator[](const string& key) const {
        if (type == OBJECT) {
            auto it = obj_val.find(key);
            if (it != obj_val.end()) return it->second;
        }
        return Json();
    }

    Json& operator[](const string& key) {
        type = OBJECT;
        return obj_val[key];
    }

    Json operator[](size_t idx) const {
        if (type == ARRAY && idx < arr_val.size()) return arr_val[idx];
        return Json();
    }

    void push_back(const Json& val) {
        type = ARRAY;
        arr_val.push_back(val);
    }

    string get_str(const string& def = "") const {
        return (type == STRING) ? s_val : def;
    }

    int get_int(int def = 0) const {
        return (type == NUMBER) ? static_cast<int>(n_val) : def;
    }

    double get_double(double def = 0.0) const {
        return (type == NUMBER) ? n_val : def;
    }

    bool get_bool(bool def = false) const {
        return (type == BOOL) ? b_val : def;
    }
};

// ==========================================
// Helper: Player to Json Serializer
// ==========================================
Json playerToJson(const Player& p, size_t id) {
    Json j;
    j["id"] = static_cast<int>(id);
    j["name"] = p.getName();
    j["age"] = p.getAge();
    j["position"] = positionToString(p.getPosition());
    j["posInt"] = static_cast<int>(p.getPosition());
    j["matchesPlayed"] = p.getMatchesPlayed();
    
    double avg = p.getAverageRating();
    j["avgRating"] = std::round(avg * 100.0) / 100.0;
    j["fatigue"] = p.getTotalFatigue();
    j["statusTier"] = p.getFatigueStatus(); // e.g. "[FRESH - GREEN]"
    j["totalRating"] = std::round(p.getTotalRating() * 100.0) / 100.0;
    
    // Clean tier keyword for styling
    if (p.getTotalFatigue() < 40) j["tier"] = "FRESH";
    else if (p.getTotalFatigue() < 75) j["tier"] = "MODERATE";
    else if (p.getTotalFatigue() < 100) j["tier"] = "HIGH RISK";
    else j["tier"] = "CRITICAL";

    return j;
}

void setCorsHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

// ==========================================
// Read index.html from Disk
// ==========================================
string readIndexHtml() {
    vector<string> candidatePaths = {
        "index.html",
        "d:/Hammad/COLLEGE/SEM 3/OOPS/Project/AuraPlay_Web/index.html",
        "d:\\Hammad\\COLLEGE\\SEM 3\\OOPS\\Project\\AuraPlay_Web\\index.html"
    };

    for (const auto& path : candidatePaths) {
        ifstream file(path);
        if (file.is_open()) {
            stringstream buf;
            buf << file.rdbuf();
            return buf.str();
        }
    }
    return "<html><body><h1>AuraPlay Server Active</h1><p>index.html not found in current directory.</p></body></html>";
}

// ==========================================
// MAIN SERVER ENTRY POINT
// ==========================================
int main() {
    // Squad starts completely empty (0 players) on startup
    TeamManager manager;
    httplib::Server svr;

    cout << "==========================================================" << endl;
    cout << "           AURAPLAY WEB REST SERVER (C++11)              " << endl;
    cout << "  Football Athlete Load & Performance Management Engine   " << endl;
    cout << "==========================================================" << endl;
    cout << "Initial squad size: " << manager.getTeam().size() << " players." << endl;

    // CORS pre-flight handler
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        res.status = 200;
    });

    // 1. GET / and /index.html -> Serves the web dashboard
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        res.set_content(readIndexHtml(), "text/html; charset=utf-8");
    });

    svr.Get("/index.html", [](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        res.set_content(readIndexHtml(), "text/html; charset=utf-8");
    });

    // 2. GET /api/squad -> Returns all players
    svr.Get("/api/squad", [&manager](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        const auto& team = manager.getTeam();
        Json squadArr(Json::ARRAY);
        for (size_t i = 0; i < team.size(); i++) {
            squadArr.push_back(playerToJson(team[i], i));
        }
        res.set_content(squadArr.dump(), "application/json");
    });

    // 3. POST /api/add -> Adds a player with duplicate check
    svr.Post("/api/add", [&manager](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        Json body = Json::parse(req.body);

        string name = body["name"].get_str();
        int age = body["age"].get_int();
        if (age <= 0 && body["age"].type == Json::STRING) {
            try { age = stoi(body["age"].get_str()); } catch (...) { age = 22; }
        }

        Position pos = ATTACKER;
        if (body["position"].type == Json::NUMBER) {
            pos = static_cast<Position>(body["position"].get_int());
        } else if (body["position"].type == Json::STRING) {
            pos = stringToPosition(body["position"].get_str());
        }

        if (name.empty()) {
            res.status = 400;
            res.set_content("{\"success\":false,\"error\":\"Player name cannot be empty\"}", "application/json");
            return;
        }

        // Duplicate check: same name (case-insensitive), same age, and same position
        if (manager.hasPlayer(name, age, pos)) {
            string exactMsg = "A player named '" + name + "' aged " + to_string(age) + 
                              " with position " + positionToString(pos) + " already exists!";
            res.status = 409;
            Json errResp;
            errResp["success"] = false;
            errResp["error"] = exactMsg;
            errResp["message"] = exactMsg;
            res.set_content(errResp.dump(), "application/json");
            cout << "[DUPLICATE REJECTED] " << exactMsg << endl;
            return;
        }

        manager.addPlayer(name, age, pos);

        Json resp;
        resp["success"] = true;
        resp["message"] = "Player " + name + " added successfully!";
        resp["player"] = playerToJson(manager.getTeam().back(), manager.getTeam().size() - 1);
        res.set_content(resp.dump(), "application/json");
    });

    // 4. POST /api/remove -> Erases a player by index from std::vector<Player> (triggering Player::~Player())
    svr.Post("/api/remove", [&manager](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        auto& team = manager.getTeam();
        if (team.empty()) {
            res.status = 400;
            res.set_content("{\"success\":false,\"error\":\"No players in squad to remove\"}", "application/json");
            return;
        }

        Json body = Json::parse(req.body);
        int targetIdx = -1;
        if (body.has("index")) targetIdx = body["index"].get_int();
        else if (body.has("id")) targetIdx = body["id"].get_int();
        else if (body.has("name")) {
            string name = body["name"].get_str();
            for (size_t i = 0; i < team.size(); i++) {
                if (equalsIgnoreCase(team[i].getName(), name)) {
                    targetIdx = static_cast<int>(i);
                    break;
                }
            }
        }

        if (targetIdx >= 0 && targetIdx < static_cast<int>(team.size())) {
            string removedName = team[targetIdx].getName();
            manager.removePlayerByIndex(targetIdx);

            Json resp;
            resp["success"] = true;
            resp["message"] = "Player '" + removedName + "' removed from squad successfully!";
            res.set_content(resp.dump(), "application/json");
        } else {
            res.status = 404;
            res.set_content("{\"success\":false,\"error\":\"Player index out of range\"}", "application/json");
        }
    });

    // 5. POST /api/simulate -> Matchweek evaluation with OveruseFatigueException handling
    svr.Post("/api/simulate", [&manager](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        auto& team = manager.getTeam();
        if (team.empty()) {
            res.status = 400;
            res.set_content("{\"success\":false,\"error\":\"No players in squad to simulate\"}", "application/json");
            return;
        }

        Json body = Json::parse(req.body);
        int teamGoalsAgainst = 0;
        if (body.has("teamGoalsConceded")) teamGoalsAgainst = body["teamGoalsConceded"].get_int();
        else if (body.has("teamGoalsAgainst")) teamGoalsAgainst = body["teamGoalsAgainst"].get_int();

        Json playerStatsArr;
        if (body.has("playerStats") && body["playerStats"].type == Json::ARRAY) {
            playerStatsArr = body["playerStats"];
        } else if (body.has("players") && body["players"].type == Json::ARRAY) {
            playerStatsArr = body["players"];
        }

        vector<string> criticalAlerts;
        Json resultsArr(Json::ARRAY);

        // Iterate over squad players and evaluate match
        for (size_t i = 0; i < team.size(); i++) {
            Json pStat;
            for (size_t j = 0; j < playerStatsArr.arr_val.size(); j++) {
                Json item = playerStatsArr[j];
                if (item.has("id") && item["id"].get_int() == static_cast<int>(i)) {
                    pStat = item; break;
                }
                if (item.has("name") && item["name"].get_str() == team[i].getName()) {
                    pStat = item; break;
                }
                if (j == i && !item.has("id") && !item.has("name")) {
                    pStat = item; break;
                }
            }

            MatchStats stats{};
            stats.teamGoalsAgainst = teamGoalsAgainst;
            stats.minutesPlayed = pStat.has("minutesPlayed") ? pStat["minutesPlayed"].get_int() : 90;
            if (stats.minutesPlayed < 0) stats.minutesPlayed = 0;
            if (stats.minutesPlayed > 90) stats.minutesPlayed = 90;

            stats.goals = pStat.has("goals") ? pStat["goals"].get_int() : 0;
            stats.assists = pStat.has("assists") ? pStat["assists"].get_int() : 0;
            stats.tackles = pStat.has("tackles") ? pStat["tackles"].get_int() : 0;
            stats.saves = pStat.has("saves") ? pStat["saves"].get_int() : 0;
            stats.goalsConceded = pStat.has("goalsConceded") ? pStat["goalsConceded"].get_int() : 0;

            int exertion = pStat.has("exertion") ? pStat["exertion"].get_int() : 7;
            if (exertion < 1) exertion = 1;
            if (exertion > 10) exertion = 10;

            bool benched = false;
            double rating = 0.0;
            string alertMsg = "";

            try {
                // Catch OveruseFatigueException when fatigue >= 100 to bench & reset fatigue to 0
                if (team[i].needsRest()) {
                    throw OveruseFatigueException(team[i].getName(), team[i].getTotalFatigue());
                }

                rating = team[i].computeMatchRating(stats);
                team[i].playMatch(rating, stats.minutesPlayed, exertion);

            } catch (const OveruseFatigueException& e) {
                benched = true;
                alertMsg = e.getMessage();
                criticalAlerts.push_back(alertMsg);
                team[i].rest(); // Benched for recovery, fatigue reset to 0/100
            }

            Json rItem;
            rItem["id"] = static_cast<int>(i);
            rItem["name"] = team[i].getName();
            rItem["position"] = positionToString(team[i].getPosition());
            rItem["benched"] = benched;
            rItem["rating"] = std::round(rating * 10.0) / 10.0;
            rItem["fatigue"] = team[i].getTotalFatigue();
            rItem["alert"] = alertMsg;
            resultsArr.push_back(rItem);
        }

        Json resp;
        resp["success"] = true;
        resp["alerts"] = Json(Json::ARRAY);
        for (const auto& a : criticalAlerts) resp["alerts"].push_back(a);
        resp["medicalAlerts"] = resp["alerts"];
        resp["results"] = resultsArr;

        Json updatedSquad(Json::ARRAY);
        for (size_t i = 0; i < team.size(); i++) {
            updatedSquad.push_back(playerToJson(team[i], i));
        }
        resp["squad"] = updatedSquad;

        res.set_content(resp.dump(), "application/json");
    });

    // 6. POST /api/rest -> Resets selected player's fatigue to 0
    svr.Post("/api/rest", [&manager](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        auto& team = manager.getTeam();
        if (team.empty()) {
            res.status = 400;
            res.set_content("{\"success\":false,\"error\":\"No players in squad\"}", "application/json");
            return;
        }

        Json body = Json::parse(req.body);
        int targetIdx = -1;
        if (body.has("id")) targetIdx = body["id"].get_int();
        else if (body.has("index")) targetIdx = body["index"].get_int();
        else if (body.has("name")) {
            string name = body["name"].get_str();
            for (size_t i = 0; i < team.size(); i++) {
                if (equalsIgnoreCase(team[i].getName(), name)) {
                    targetIdx = static_cast<int>(i);
                    break;
                }
            }
        }

        if (targetIdx >= 0 && targetIdx < static_cast<int>(team.size())) {
            team[targetIdx].rest();
            Json resp;
            resp["success"] = true;
            resp["message"] = "Player " + team[targetIdx].getName() + " is now fully rested (Fatigue reset to 0)!";
            resp["player"] = playerToJson(team[targetIdx], targetIdx);
            res.set_content(resp.dump(), "application/json");
        } else {
            res.status = 404;
            res.set_content("{\"success\":false,\"error\":\"Player not found\"}", "application/json");
        }
    });

    // 7. GET /api/mvp -> Uses overloaded operator> to return Team MVP
    svr.Get("/api/mvp", [&manager](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        size_t mvpIdx = 0;
        const Player* mvp = manager.getMVP(&mvpIdx);

        if (!mvp) {
            res.status = 200;
            res.set_content("{\"empty\":true,\"message\":\"No players in the squad to determine MVP!\"}", "application/json");
            return;
        }

        Json resp = playerToJson(*mvp, mvpIdx);
        resp["isMVP"] = true;
        res.set_content(resp.dump(), "application/json");
    });

    // 8. POST /api/save -> Calls saveToFile("team_data.txt")
    svr.Post("/api/save", [&manager](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        bool ok = manager.saveToFile("team_data.txt");
        Json resp;
        resp["success"] = ok;
        resp["message"] = ok ? "Team data saved to team_data.txt successfully!" : "Failed to write team_data.txt";
        res.set_content(resp.dump(), "application/json");
    });

    // 9. POST /api/load -> Calls loadFromFile("team_data.txt")
    svr.Post("/api/load", [&manager](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        bool ok = manager.loadFromFile("team_data.txt");
        Json resp;
        resp["success"] = ok;
        resp["message"] = ok ? "Team data loaded from team_data.txt successfully!" : "Error: team_data.txt not found!";
        resp["count"] = static_cast<int>(manager.getTeam().size());

        Json sq(Json::ARRAY);
        for (size_t i = 0; i < manager.getTeam().size(); i++) {
            sq.push_back(playerToJson(manager.getTeam()[i], i));
        }
        resp["squad"] = sq;

        res.set_content(resp.dump(), "application/json");
    });

    cout << "\n>>> AuraPlay Server listening at http://0.0.0.0:8080 (Local: http://localhost:8080) <<<" << endl;
    cout << "Endpoints exposed:" << endl;
    cout << "  GET  /              -> Serves index.html" << endl;
    cout << "  GET  /api/squad     -> Returns all players JSON" << endl;
    cout << "  POST /api/add       -> Adds a player (with duplicate check)" << endl;
    cout << "  POST /api/remove    -> Erases player by index (Player::~Player())" << endl;
    cout << "  POST /api/simulate  -> Runs match evaluation & medical alerts" << endl;
    cout << "  POST /api/rest      -> Resets player fatigue to 0" << endl;
    cout << "  GET  /api/mvp       -> Evaluates Team MVP via operator>" << endl;
    cout << "  POST /api/save      -> Saves squad to team_data.txt" << endl;
    cout << "  POST /api/load      -> Loads squad from team_data.txt" << endl;
    cout << "\nReady for requests...\n" << endl;

    if (!svr.listen("0.0.0.0", 8080)) {
        cerr << "Error: Failed to bind to 0.0.0.0:8080!" << endl;
        return 1;
    }

    return 0;
}
