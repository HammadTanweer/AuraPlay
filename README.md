# AuraPlay: Football Athlete Load & Performance Management Engine

AuraPlay is a console-based Object-Oriented C++ application designed for grassroots and collegiate football squad management. It replaces passive spreadsheets with automated workload monitoring, position-specific match ratings, and programmatic injury prevention.

## Key Features & OOP Architecture
- **Inheritance & Abstraction:** Abstract base class `Person` inherited by `Player`.
- **Strategy Design Pattern:** Runtime polymorphic match rating calculation via `IRatingStrategy` (`AttackerRatingStrategy`, `DefenderRatingStrategy`, `GoalkeeperRatingStrategy`).
- **Cumulative sRPE Fatigue Monitoring:** Calculates session load from minutes played and perceived exertion (`1–10`) across four readiness tiers (`FRESH`, `MODERATE`, `HIGH RISK`, `CRITICAL`).
- **Custom Exception Handling:** Throws `OveruseFatigueException` when fatigue reaches `>= 100`, automatically benching the player and resetting fatigue to `0/100` upon recovery.
- **Operator Overloading & Rule of Three:** Overloads relational operator `>` to identify the Team MVP and implements deep-copy memory management.
- **Local File Persistence:** Serializes and loads roster states via `team_data.txt` using native C++ File I/O.

## How to Compile & Run
```bash
g++ -std=c++11 main.cpp -o auraplay
./auraplay
