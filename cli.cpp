#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "data.h"
#include "harness.h"

using namespace std;

//Where the benchmark csv gets written for plot_benchmarks.py
const string RESULTS_PATH = "results/benchmark_results.csv";

//Everything the menu needs to remember between choices
struct AppState {
    Dataset data;
    vector<string> approaches;
    vector<unique_ptr<Recommender>> built;
    size_t current = 0;
    vector<BenchmarkRow> lastBenchmark;
};

bool readLine(const string& prompt, string& line) {
    cout << prompt;
    return static_cast<bool>(getline(cin, line));
}

bool parseInt(const string& text, int& value) {
    try {
        size_t used = 0;
        value = stoi(text, &used);
        return used == text.size();
    } catch (...) {
        return false;
    }
}

double timeMs(chrono::steady_clock::time_point start) {
    return chrono::duration<double, milli>(chrono::steady_clock::now() - start).count();
}

void buildAll(AppState& state) {
    state.built.clear();
    size_t approachIndex = 0;
    while (approachIndex < state.approaches.size()) {
        state.built.push_back(makeApproach(state.approaches[approachIndex]));
        auto start = chrono::steady_clock::now();
        state.built.back()->buildFrom(state.data);
        cout << "Built " << state.approaches[approachIndex] << " in " << fixed
             << setprecision(1) << timeMs(start) << " ms" << endl;
        approachIndex++;
    }
}

void printMatches(const Dataset& data, const vector<int>& matches) {
    size_t shown = min<size_t>(matches.size(), 15);
    size_t index = 0;
    while (index < shown) {
        cout << "  " << matches[index] << " | " << data.title(matches[index]) << " | "
             << data.likeCount(matches[index]) << " likes" << endl;
        index++;
    }
    if (matches.size() > shown) {
        cout << "  ...and " << matches.size() - shown << " more, try a longer search" << endl;
    }
}

void searchMovie(AppState& state) {
    string query;
    if (!readLine("Search title: ", query) || query.empty()) { return; }

    vector<int> matches = state.data.searchTitle(query);
    if (matches.empty()) {
        cout << "No movies found for \"" << query << "\"" << endl;
        return;
    }
    printMatches(state.data, matches);
}

//A number is treated as a movieID, anything else is a title search
bool chooseMovie(AppState& state, int& movieID) {
    string input;
    if (!readLine("Movie title or ID: ", input) || input.empty()) { return false; }

    if (parseInt(input, movieID)) {
        if (state.data.hasMovie(movieID)) { return true; }
        cout << "No movie with ID " << movieID << endl;
        return false;
    }

    vector<int> matches = state.data.searchTitle(input);
    if (matches.empty()) {
        cout << "No movies found for \"" << input << "\"" << endl;
        return false;
    }
    if (matches.size() == 1) {
        movieID = matches.front();
        return true;
    }

    cout << "Multiple matches:" << endl;
    printMatches(state.data, matches);
    string pick;
    if (!readLine("Enter an ID from the list (blank = most liked, " +
                      to_string(matches.front()) + "): ", pick)) {
        return false;
    }
    if (pick.empty()) {
        movieID = matches.front();
        return true;
    }
    if (parseInt(pick, movieID) && state.data.hasMovie(movieID)) { return true; }
    cout << "That wasn't a valid movie ID" << endl;
    return false;
}

void printRecommendations(AppState& state, int movieID) {
    Recommender& recommender = *state.built[state.current];

    auto start = chrono::steady_clock::now();
    vector<Recommendation> results = recommender.topFor(movieID);
    double ms = timeMs(start);

    cout << endl << "Movie: \"" << state.data.title(movieID) << "\"  Approach: " << recommender.label()
         << endl;
    if (results.empty()) {
        cout << "No recommendations (nobody rated this movie 3 or higher)" << endl;
    } else {
        cout << "Top 10:" << endl;
        size_t index = 0;
        while (index < results.size()) {
            cout << "  " << setw(2) << index + 1 << ". "
                 << state.data.title(results[index].second)
                 << " | Shared Likes: " << results[index].first << endl;
            index++;
        }
    }
    cout << "Query Time: " << fixed << setprecision(3) << ms << " ms" << endl << endl;
}

void recommend(AppState& state) {
    int movieID;
    if (chooseMovie(state, movieID)) { printRecommendations(state, movieID); }
}

void switchApproach(AppState& state) {
    cout << "Approaches:" << endl;
    for (size_t index = 0; index < state.approaches.size(); index++) {
        cout << "  " << index + 1 << ") " << state.approaches[index]
             << (index == state.current ? "  <-- current" : "") << endl;
    }
    string input;
    int choice;
    bool valid = readLine("Pick one: ", input) && parseInt(input, choice) && choice >= 1 &&
                 choice <= static_cast<int>(state.approaches.size());
    if (!valid) {
        cout << "Keeping " << state.approaches[state.current] << endl;
        return;
    }
    state.current = static_cast<size_t>(choice - 1);
    cout << "Now using " << state.approaches[state.current] << endl;
}

void addNewRating(AppState& state) {
    string input;
    int userID;
    int movieID;
    double rating;

    if (!readLine("User ID: ", input) || !parseInt(input, userID)) {
        cout << "User ID must be a whole number" << endl;
        return;
    }
    if (!chooseMovie(state, movieID)) { return; }
    if (!readLine("Rating (0.5 - 5): ", input)) { return; }
    try {
        rating = stod(input);
    } catch (...) {
        cout << "Rating must be a number" << endl;
        return;
    }
    if (rating < 0.5 || rating > 5.0) {
        cout << "Rating must be between 0.5 and 5" << endl;
        return;
    }

    if (!state.data.addRating(userID, movieID, rating)) {
        cout << "Saved, but it isn't a new like (3+ stars) so recommendations won't change" << endl;
        return;
    }
    //only a brand new like gets sent to the approaches so nothing is counted twice
    for (auto& recommender : state.built) { recommender->addLike(userID, movieID); }
    cout << "Added! User " << userID << " now likes \"" << state.data.title(movieID) << "\""
         << endl;
}

void runBenchmarkMenu(AppState& state) {
    cout << "Running benchmark (this rebuilds everything a few times)..." << endl;
    state.lastBenchmark = runBenchmark(state.data, state.approaches, BenchmarkConfig());
    cout << endl;
    printBenchmark(state.lastBenchmark);
    cout << endl;
}

bool exportResults(const vector<BenchmarkRow>& results) {
    filesystem::create_directories("results");
    return writeBenchmarkCsv(results, RESULTS_PATH);
}

void exportMenu(AppState& state) {
    if (state.lastBenchmark.empty()) {
        cout << "Nothing to export yet, run the benchmark (option 5) first" << endl;
        return;
    }
    if (exportResults(state.lastBenchmark)) {
        cout << "Wrote " << RESULTS_PATH << " (run: python3 plot_benchmarks.py)" << endl;
    } else {
        cout << "Could not write " << RESULTS_PATH << endl;
    }
}

void printMenu(const AppState& state) {
    cout << "===== GatorBoxd Movie Recommender =====" << endl;
    cout << "Current approach: " << state.approaches[state.current] << endl;
    cout << "1) Search movie title" << endl;
    cout << "2) Recommendations" << endl;
    cout << "3) Switch approach" << endl;
    cout << "4) Add new rating" << endl;
    cout << "5) Run benchmark" << endl;
    cout << "6) Export results" << endl;
    cout << "0) Quit" << endl;
}

int main(int argc, char* argv[]) {
    string dataDir = "data/ml-1m";
    bool benchmarkOnly = false;
    int argIndex = 1;
    while (argIndex < argc) {
        string argument = argv[argIndex];
        if (argument == "--test") { return runSelfTests(); }
        if (argument == "--benchmark") {
            benchmarkOnly = true;
        } else {
            dataDir = argument;
        }
        argIndex++;
    }

    AppState state;
    string error;
    cout << "Loading " << dataDir << "..." << endl;
    if (!state.data.load(dataDir, error)) {
        cout << error << endl;
        cout << "Download MovieLens 1M and unzip it into data/ml-1m (see README)" << endl;
        return 1;
    }
    cout << "Loaded " << state.data.allMovies().size() << " movies, "
         << state.data.allRatings().size() << " ratings (" << state.data.numLikes()
         << " likes)" << endl;

    state.approaches = approachNames();

    if (benchmarkOnly) {
        state.lastBenchmark = runBenchmark(state.data, state.approaches, BenchmarkConfig());
        cout << endl;
        printBenchmark(state.lastBenchmark);
        if (!exportResults(state.lastBenchmark)) {
            cout << "Could not write " << RESULTS_PATH << endl;
            return 1;
        }
        cout << "Wrote " << RESULTS_PATH << endl;
        return 0;
    }

    buildAll(state);

    //menu loop, runs until the user picks 0 or the input ends
    string choice;
    for (;;) {
        cout << endl;
        printMenu(state);
        if (!readLine("> ", choice) || choice == "0") { break; }

        if (choice == "1") { searchMovie(state); }
        else if (choice == "2") { recommend(state); }
        else if (choice == "3") { switchApproach(state); }
        else if (choice == "4") { addNewRating(state); }
        else if (choice == "5") { runBenchmarkMenu(state); }
        else if (choice == "6") { exportMenu(state); }
        else { cout << "Please pick 0-6" << endl; }
    }
    cout << "Bye!" << endl;
    return 0;
}
