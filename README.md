# AuraPlay: Football Athlete Load & Performance Management Engine

AuraPlay is an Object-Oriented C++11 sports-science and squad management engine designed for grassroots and collegiate football teams. It replaces passive spreadsheets with automated sRPE workload monitoring, position-specific match ratings, programmatic injury prevention, and an interactive web analytics dashboard powered directly by a native C++ HTTP backend.

---

## Key Features & OOP Architecture

- **Inheritance & Abstraction:** Abstract base class `Person` defining athlete identity contracts, inherited by `Player`.
- **Strategy Design Pattern:** Runtime polymorphic match rating calculation via `IRatingStrategy` (`AttackerRatingStrategy`, `DefenderRatingStrategy`, `GoalkeeperRatingStrategy`).
- **Cumulative sRPE Fatigue Monitoring:** Calculates session load from minutes played and perceived exertion (`1–10`) across four readiness tiers (`FRESH`, `MODERATE`, `HIGH RISK`, `CRITICAL`).
- **Custom Exception Handling:** Throws `OveruseFatigueException` when cumulative fatigue reaches `>= 100`, automatically benching the athlete and resetting fatigue to `0/100` upon recovery.
- **Operator Overloading & Rule of Three:** Overloads relational operator `>` to automate Team MVP selection and implements deep-copy memory management (Copy Constructor, Copy Assignment Operator, and Virtual Destructor).
- **Local File Persistence:** Serializes and restores squad rosters via `team_data.txt` using native C++ File I/O (`std::ofstream` / `std::ifstream`).
- **C++11 Web REST Layer (`cpp-httplib`):** Exposes the core C++ OOP domain model over local JSON endpoints (`/api/squad`, `/api/add`, `/api/simulate`, `/api/rest`, `/api/mvp`, `/api/save`, `/api/load`) to a dark-mode `index.html` dashboard.

---

## Project Structure

- `server.cpp` — C++11 OOP domain engine wrapped in a `cpp-httplib` REST server (`port 8080`).
- `index.html` — Interactive frontend dashboard (HTML5, CSS3, Vanilla JS `fetch()`).
- `httplib.h` — Single-header C++ HTTP library (`yhirose/cpp-httplib`).
- `main.cpp` — Standalone terminal Command Line Interface (CLI) version.
- `team_data.txt` — Flat-file persistence store for serialized squad telemetry.

---

## How to Compile & Run

### Option 1: Web Dashboard (Recommended — Windows)
1. **Compile `server.cpp` with Windows Sockets (`-lws2_32`):**
   ```powershell
   g++ -std=c++11 -D_WIN32_WINNT=0x0A00 server.cpp -o server.exe -lws2_32 -lcrypt32
   ```
2. **Start the C++ Server:**
   ```powershell
   .\server.exe
   ```
3. **Open in Browser:**
   Navigate to `http://localhost:8080` while keeping the terminal window running.

### Option 2: Standalone Console CLI (`main.cpp`)
```bash
g++ -std=c++11 main.cpp -o auraplay
./auraplay
```
