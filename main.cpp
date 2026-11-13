#include <iostream>
#include <string>
#include <vector>
#include <fstream>

using namespace std;

// Position Enum for clean type safety
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

// Struct encapsulating match statistics across all positions
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

    // Destructor (Rule of Three)
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

    void saveToFile() {
        ofstream outFile("team_data.txt");
        if (!outFile) {
            cout << "Error opening file for saving!" << endl;
            return;
        }

        outFile << team.size() << endl;
        for (size_t i = 0; i < team.size(); i++) {
            outFile << team[i].getName() << endl;
            outFile << team[i].getAge() << " "
                    << static_cast<int>(team[i].getPosition()) << " "
                    << team[i].getMatchesPlayed() << " "
                    << team[i].getTotalFatigue() << " "
                    << team[i].getTotalRating() << endl;
        }

        outFile.close();
        cout << "Team data saved to team_data.txt successfully!" << endl;
    }

    void loadFromFile() {
        ifstream inFile("team_data.txt");
        if (!inFile) {
            cout << "Error: team_data.txt not found!" << endl;
            return;
        }

        team.clear();
        int count;
        inFile >> count;
        inFile.ignore();

        for (int i = 0; i < count; i++) {
            string name;
            int age, posInt, matches, fatigue;
            double rating;

            getline(inFile, name);
            inFile >> age >> posInt >> matches >> fatigue >> rating;
            inFile.ignore();

            Player loadedPlayer(name, age, static_cast<Position>(posInt), matches, fatigue, rating);
            team.push_back(loadedPlayer);
        }

        inFile.close();
        cout << "Team data loaded from team_data.txt successfully!" << endl;
    }
};

// ==========================================
// 6. MAIN MENU
// ==========================================
int main() {
    TeamManager manager;
    int choice = 0;

    cout << "==========================================================" << endl;
    cout << "                         AURAPLAY                         " << endl;
    cout << "   Football Athlete Load & Performance Management Engine  " << endl;
    cout << "==========================================================" << endl;

    while (choice != 8) {
        cout << "\n=== AuraPlay: Main Menu ===" << endl;
        cout << "1. Add Player" << endl;
        cout << "2. Simulate Match" << endl;
        cout << "3. Show Team Stats" << endl;
        cout << "4. Show Team MVP" << endl;
        cout << "5. Rest a Player" << endl;
        cout << "6. Save Data" << endl;
        cout << "7. Load Data" << endl;
        cout << "8. Exit" << endl;
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice) {
            case 1: {
                string name;
                int age, posChoice;
                cout << "Enter player name: ";
                cin.ignore();
                getline(cin, name);
                cout << "Enter player age: ";
                cin >> age;
                cout << "Select Position (0: Attacker, 1: Midfielder, 2: Defender, 3: Goalkeeper): ";
                cin >> posChoice;
                manager.addPlayer(name, age, static_cast<Position>(posChoice));
                break;
            }
            case 2:
                manager.simulateTeamMatch();
                break;
            case 3:
                manager.showTeamStats();
                break;
            case 4:
                manager.showMVP();
                break;
            case 5:
                manager.restPlayer();
                break;
            case 6:
                manager.saveToFile();
                break;
            case 7:
                manager.loadFromFile();
                break;
            case 8:
                cout << "Exiting AuraPlay. Goodbye!" << endl;
                break;
            default:
                cout << "Invalid choice! Please select 1-8." << endl;
        }
    }

    return 0;
}