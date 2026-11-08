#include "data.h"
#include "harness.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "graph.h"

#ifdef HAVE_COWATCH_INDEX
#include "cowatch_index.h"
#endif

using namespace std;

//Split a line on the two character "::" that MovieLens 1M uses
vector<string> splitFields(const string& line) {
    vector<string> fields;
    size_t start = 0;
    for (;;) {
        size_t position = line.find("::", start);
        if (position == string::npos) {
            fields.push_back(line.substr(start));
            return fields;
        }
        fields.push_back(line.substr(start, position - start));
        start = position + 2;
    }
}

string toLower(string text) {
    size_t index = 0;
    while (index < text.size()) {
        text[index] = tolower((unsigned char)text[index]);
        index++;
    }
    return text;
}

//Member 1's graph only counts 4.0 and up, so send 5.0 for anything that counts as a like here
double forTeammate(double rating) {
    if (rating >= LIKE_THRESHOLD) { return 5.0; }
    return rating;
}

//Milliseconds since start
double elapsedMs(chrono::steady_clock::time_point start) {
    chrono::duration<double, milli> elapsed = chrono::steady_clock::now() - start;
    return elapsed.count();
}

//One number for a (user, movie) pair, movieIDs in MovieLens are well under 1,000,000
long long Dataset::pairKey(int userID, int movieID) {
    return (long long)userID * 1000000 + movieID;
}

bool Dataset::load(const string& dataDir, string& error) {
    ifstream movieFile(dataDir + "/movies.dat");
    if (!movieFile) {
        error = "Could not open " + dataDir + "/movies.dat";
        return false;
    }
    string line;
    while (getline(movieFile, line)) {
        vector<string> fields = splitFields(line);
        if (fields.size() >= 3) { addMovie(stoi(fields[0]), fields[1], fields[2]); }
    }

    ifstream ratingFile(dataDir + "/ratings.dat");
    if (!ratingFile) {
        error = "Could not open " + dataDir + "/ratings.dat";
        return false;
    }
    while (getline(ratingFile, line)) {
        vector<string> fields = splitFields(line);
        if (fields.size() >= 4) {
            addRating(stoi(fields[0]), stoi(fields[1]), stod(fields[2]), stoll(fields[3]));
        }
    }
    return true;
}

void Dataset::addMovie(int movieID, const string& title, const string& genres) {
    movies[movieID] = Movie{movieID, title, genres};
}

//Only a new like (3 or above, and the user hasn't liked that movie yet) returns true
bool Dataset::addRating(int userID, int movieID, double rating, long long timestamp) {
    bool isLike = rating >= LIKE_THRESHOLD;
    bool isNewLike = isLike && likedPairs.insert(pairKey(userID, movieID)).second;
    if (isLike && !isNewLike) { return false; }
    ratings.push_back(Rating{userID, movieID, rating, timestamp});
    if (isNewLike) { movieLikeCount[movieID]++; }
    return isNewLike;
}

Dataset Dataset::subsample(int percent) const {
    Dataset sub;
    sub.movies = movies;
    size_t index = 0;
    while (index < ratings.size()) {
        if (index % 100 < percent) {
            sub.addRating(ratings[index].userID, ratings[index].movieID, ratings[index].rating,
                          ratings[index].timestamp);
        }
        index++;
    }
    return sub;
}

vector<int> Dataset::searchTitle(const string& query) const {
    string needle = toLower(query);
    vector<int> matches;
    for (const auto& entry : movies) {
        if (toLower(entry.second.title).find(needle) != string::npos) {
            matches.push_back(entry.first);
        }
    }
    //most liked first, a tie goes to the smaller movieID
    sort(matches.begin(), matches.end(), [this](int movieA, int movieB) {
        int likesA = likeCount(movieA);
        int likesB = likeCount(movieB);
        if (likesA != likesB) { return likesA > likesB; }
        return movieA < movieB;
    });
    return matches;
}

bool Dataset::hasMovie(int movieID) const {
    return movies.count(movieID) > 0;
}

string Dataset::title(int movieID) const {
    if (!hasMovie(movieID)) { return "Unknown movie " + to_string(movieID); }
    return movies.at(movieID).title;
}

int Dataset::likeCount(int movieID) const {
    if (movieLikeCount.count(movieID) == 0) { return 0; }
    return movieLikeCount.at(movieID);
}

vector<int> Dataset::moviesWithLikes(int minLikes) const {
    vector<int> movieIDs;
    for (const auto& entry : movieLikeCount) {
        if (entry.second >= minLikes) { movieIDs.push_back(entry.first); }
    }
    sort(movieIDs.begin(), movieIDs.end());
    return movieIDs;
}

//Top 10 by shared likes, a tie goes to the smaller movieID
vector<Recommendation> topTen(const unordered_map<int, int>& movieLikeCount, int excludeMovieID) {
    vector<Recommendation> candidates;
    for (const auto& entry : movieLikeCount) {
        if (entry.first != excludeMovieID) {
            candidates.push_back({entry.second, entry.first});
        }
    }
    sort(candidates.begin(), candidates.end(),
         [](const Recommendation& better, const Recommendation& worse) {
             if (better.first != worse.first) { return better.first > worse.first; }
             return better.second < worse.second;
         });
    if (candidates.size() > 10) { candidates.resize(10); }
    return candidates;
}

void BruteForceRecommender::buildFrom(const Dataset& data) {
    usersToMovies.clear();
    const vector<Rating>& allRows = data.allRatings();
    size_t index = 0;
    while (index < allRows.size()) {
        if (allRows[index].rating >= LIKE_THRESHOLD) {
            addLike(allRows[index].userID, allRows[index].movieID);
        }
        index++;
    }
}

void BruteForceRecommender::addLike(int userID, int movieID) {
    vector<int>& likedMovies = usersToMovies[userID];
    for (int thisMovie : likedMovies) {
        if (thisMovie == movieID) { return; }
    }
    likedMovies.push_back(movieID);
}

//Check every user, and if they liked this movie count all the other movies they liked
vector<Recommendation> BruteForceRecommender::topFor(int movieID) {
    unordered_map<int, int> movieLikeCount;
    for (const auto& entry : usersToMovies) {
        const vector<int>& likedMovies = entry.second;

        bool likedIt = false;
        for (int thisMovie : likedMovies) {
            if (thisMovie == movieID) { likedIt = true; }
        }

        if (likedIt) {
            for (int thisMovie : likedMovies) { movieLikeCount[thisMovie]++; }
        }
    }
    return topTen(movieLikeCount, movieID);
}

void PopularityRecommender::buildFrom(const Dataset& data) {
    movieLikeCount.clear();
    for (const auto& entry : data.allMovies()) {
        int likes = data.likeCount(entry.first);
        if (likes > 0) { movieLikeCount[entry.first] = likes; }
    }
}

void PopularityRecommender::addLike(int userID, int movieID) {
    movieLikeCount[movieID]++;
}

vector<Recommendation> PopularityRecommender::topFor(int movieID) {
    return topTen(movieLikeCount, movieID);
}

class GraphRecommender : public Recommender {
private:
    Graph graph;

public:
    string label() const override { return "Graph"; }

    void buildFrom(const Dataset& data) override {
        graph = Graph();
        for (const auto& entry : data.allMovies()) {
            graph.addMovie(entry.first, entry.second.title);
        }
        const vector<Rating>& allRows = data.allRatings();
        size_t index = 0;
        while (index < allRows.size()) {
            graph.addRating(allRows[index].userID, allRows[index].movieID,
                            forTeammate(allRows[index].rating));
            index++;
        }
    }

    vector<Recommendation> topFor(int movieID) override {
        return graph.recommendedMovies(movieID);
    }

    void addLike(int userID, int movieID) override {
        graph.addRating(userID, movieID, forTeammate(LIKE_THRESHOLD));
    }
};

#ifdef HAVE_COWATCH_INDEX
class IndexRecommender : public Recommender {
private:
    CoWatchIndex index;

public:
    string label() const override { return "Index"; }

    void buildFrom(const Dataset& data) override {
        index = CoWatchIndex();
        for (const auto& entry : data.allMovies()) {
            index.addMovie(entry.first, entry.second.title);
        }
        const vector<Rating>& allRows = data.allRatings();
        size_t position = 0;
        while (position < allRows.size()) {
            index.addRating(allRows[position].userID, allRows[position].movieID,
                            forTeammate(allRows[position].rating));
            position++;
        }
        index.buildTable();
    }

    vector<Recommendation> topFor(int movieID) override {
        return index.recommendedMovies(movieID);
    }

    void addLike(int userID, int movieID) override {
        index.addRating(userID, movieID, forTeammate(LIKE_THRESHOLD));
        index.buildTable();
    }
};
#endif

//Make a fresh empty approach from its name
unique_ptr<Recommender> makeApproach(const string& name) {
    if (name == "Graph") { return make_unique<GraphRecommender>(); }
#ifdef HAVE_COWATCH_INDEX
    if (name == "Index") { return make_unique<IndexRecommender>(); }
#endif
    if (name == "BruteForce") { return make_unique<BruteForceRecommender>(); }
    return make_unique<PopularityRecommender>();
}

vector<string> approachNames() {
    vector<string> names = {"Graph"};
#ifdef HAVE_COWATCH_INDEX
    names.push_back("Index");
#endif
    names.push_back("BruteForce");
    names.push_back("Popularity");
    return names;
}

//Build each approach at each data size, then time the build and the queries
vector<BenchmarkRow> runBenchmark(const Dataset& fullData, const vector<string>& approaches,
                                  const BenchmarkConfig& config) {
    vector<BenchmarkRow> results;

    size_t sizeIndex = 0;
    while (sizeIndex < config.percents.size()) {
        int percent = config.percents[sizeIndex];
        Dataset subset = fullData;
        if (percent < 100) { subset = fullData.subsample(percent); }

        //spread the query movies evenly over every movie with enough likes
        vector<int> candidates = subset.moviesWithLikes(config.minLikesForQuery);
        size_t wanted = config.queriesPerRun;
        size_t step = max<size_t>(1, candidates.size() / wanted);
        vector<int> queries;
        for (size_t position = 0; position < candidates.size() && queries.size() < wanted;
             position += step) {
            queries.push_back(candidates[position]);
        }

        cout << "--- " << percent << "% of ratings (" << subset.allRatings().size()
             << " ratings, " << queries.size() << " query movies) ---" << endl;

        //brute force answers first so every approach is checked against the same lists
        BruteForceRecommender reference;
        reference.buildFrom(subset);
        vector<vector<Recommendation>> expected;
        for (int movieID : queries) { expected.push_back(reference.topFor(movieID)); }

        for (const string& approach : approaches) {
            unique_ptr<Recommender> recommender = makeApproach(approach);

            auto buildStart = chrono::steady_clock::now();
            recommender->buildFrom(subset);
            double buildMs = elapsedMs(buildStart);

            vector<double> times;
            int matches = 0;
            size_t current = 0;
            while (current < queries.size()) {
                auto queryStart = chrono::steady_clock::now();
                vector<Recommendation> result = recommender->topFor(queries[current]);
                times.push_back(elapsedMs(queryStart));
                if (result == expected[current]) { matches++; }
                current++;
            }

            double total = 0;
            for (double time : times) { total += time; }
            sort(times.begin(), times.end());

            BenchmarkRow row;
            row.approach = approach;
            row.numRatings = subset.allRatings().size();
            row.numLikes = subset.numLikes();
            row.buildMs = buildMs;
            row.numQueries = queries.size();
            row.avgQueryMs = 0;
            row.p95QueryMs = 0;
            row.matchBruteForcePct = 0;
            if (!queries.empty()) {
                row.avgQueryMs = total / times.size();
                row.p95QueryMs = times[(size_t)(times.size() * 0.95)];
                row.matchBruteForcePct = 100.0 * matches / queries.size();
            }
            results.push_back(row);

            cout << "  " << left << setw(11) << row.approach << " build " << fixed
                 << setprecision(1) << buildMs << " ms, avg query " << setprecision(3)
                 << row.avgQueryMs << " ms" << endl;
        }
        sizeIndex++;
    }
    return results;
}

bool writeBenchmarkCsv(const vector<BenchmarkRow>& results, const string& path) {
    ofstream out(path);
    if (!out) { return false; }
    out << "approach,num_ratings,num_likes,build_ms,num_queries,avg_query_ms,p95_query_ms,"
           "match_bruteforce_pct\n";
    out << fixed << setprecision(4);
    for (const BenchmarkRow& row : results) {
        out << row.approach << ',' << row.numRatings << ',' << row.numLikes << ','
            << row.buildMs << ',' << row.numQueries << ',' << row.avgQueryMs << ','
            << row.p95QueryMs << ',' << row.matchBruteForcePct << '\n';
    }
    return true;
}

void printBenchmark(const vector<BenchmarkRow>& results) {
    cout << left << setw(12) << "Approach" << right << setw(10) << "Ratings" << setw(12)
         << "Build(ms)" << setw(14) << "AvgQuery(ms)" << setw(14) << "P95Query(ms)" << setw(12)
         << "Match BF%" << endl;
    for (const BenchmarkRow& row : results) {
        cout << left << setw(12) << row.approach << right << setw(10) << row.numRatings << fixed
             << setprecision(1) << setw(12) << row.buildMs << setprecision(3) << setw(14)
             << row.avgQueryMs << setw(14) << row.p95QueryMs << setprecision(1) << setw(12)
             << row.matchBruteForcePct << endl;
    }
}

int failures = 0;

//Print PASS or FAIL for one check and remember the failures
void check(bool ok, const string& what) {
    if (ok) { cout << "PASS  " << what << endl; }
    else { cout << "FAIL  " << what << endl; failures++; }
}

//Same shape of data as Member 1's smoke test, plus a 2.5 that must NOT count as a like
Dataset makeTinyDataset() {
    Dataset data;
    data.addMovie(10, "Movie A", "Drama");
    data.addMovie(20, "Movie B", "Drama");
    data.addMovie(30, "Movie C", "Drama");
    data.addMovie(40, "Movie D", "Drama");
    data.addMovie(50, "Movie E", "Drama");

    data.addRating(1, 10, 5.0);
    data.addRating(1, 20, 4.0);
    data.addRating(1, 30, 5.0);

    data.addRating(2, 10, 4.0);
    data.addRating(2, 20, 5.0);
    data.addRating(2, 40, 4.0);

    data.addRating(3, 10, 5.0);
    data.addRating(3, 30, 4.0);
    data.addRating(3, 50, 5.0);
    data.addRating(3, 20, 2.5);
    return data;
}

bool contains(const vector<Recommendation>& list, int movieID) {
    for (const Recommendation& entry : list) {
        if (entry.second == movieID) { return true; }
    }
    return false;
}

//Runs the checks on a tiny dataset, returns 0 if everything passed
int runSelfTests() {
    failures = 0;
    Dataset data = makeTinyDataset();
    vector<string> approaches = approachNames();

    check(data.numLikes() == 9, "2.5 star rating is not counted as a like");
    check(!data.addRating(1, 10, 5.0), "liking the same movie twice is not a new like");

    for (const string& name : approaches) {
        unique_ptr<Recommender> recommender = makeApproach(name);
        recommender->buildFrom(data);

        vector<Recommendation> first = recommender->topFor(10);
        check(first == recommender->topFor(10), name + ": same movie twice gives same top 10");
        check(!contains(first, 10), name + ": input movie is excluded");
        check(first.size() <= 10, name + ": at most 10 results");

        //Popularity ignores the input movie so it can't match this exact list
        if (name != "Popularity") {
            vector<Recommendation> expected = {{2, 20}, {2, 30}, {1, 40}, {1, 50}};
            check(first == expected, name + ": exact top results for movie 10");
        }
    }

    BruteForceRecommender reference;
    reference.buildFrom(data);
    for (const string& name : approaches) {
        if (name == "Popularity" || name == "BruteForce") { continue; }
        unique_ptr<Recommender> recommender = makeApproach(name);
        recommender->buildFrom(data);

        bool allMatch = true;
        for (int movieID = 10; movieID <= 50; movieID += 10) {
            if (recommender->topFor(movieID) != reference.topFor(movieID)) { allMatch = false; }
        }
        check(allMatch, name + ": matches brute force on every movie");
    }

    for (const string& name : approaches) {
        if (name == "Popularity") { continue; }
        unique_ptr<Recommender> recommender = makeApproach(name);
        recommender->buildFrom(data);
        recommender->addLike(4, 10);
        recommender->addLike(4, 50);

        bool found = false;
        for (const Recommendation& entry : recommender->topFor(10)) {
            if (entry.second == 50 && entry.first == 2) { found = true; }
        }
        check(found, name + ": new rating changes recommendations");
    }

    if (failures == 0) { cout << endl << "All checks passed" << endl; }
    else { cout << endl << "Some checks FAILED" << endl; }
    return failures == 0 ? 0 : 1;
}
