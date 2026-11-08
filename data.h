#ifndef PROJECT2_DATA_H
#define PROJECT2_DATA_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace std;

struct Rating {
    int userID;
    int movieID;
    double rating;
    long long timestamp;
};

struct Movie {
    int movieID;
    string title;
    string genres;
};

const double LIKE_THRESHOLD = 3.0;

class Dataset {
private:
    vector<Rating> ratings;
    unordered_map<int, Movie> movies;
    unordered_map<int, int> movieLikeCount;
    unordered_set<long long> likedPairs;

    static long long pairKey(int userID, int movieID);

public:
    bool load(const string& dataDir, string& error);
    void addMovie(int movieID, const string& title, const string& genres);
    bool addRating(int userID, int movieID, double rating, long long timestamp = 0);
    Dataset subsample(int percent) const;
    vector<int> searchTitle(const string& query) const;
    bool hasMovie(int movieID) const;
    string title(int movieID) const;
    int likeCount(int movieID) const;
    vector<int> moviesWithLikes(int minLikes) const;

    const vector<Rating>& allRatings() const { return ratings; }
    const unordered_map<int, Movie>& allMovies() const { return movies; }
    size_t numLikes() const { return likedPairs.size(); }
};

using Recommendation = pair<int, int>;

class Recommender {
public:
    virtual ~Recommender() = default;
    virtual string label() const = 0;
    virtual void buildFrom(const Dataset& data) = 0;
    virtual vector<Recommendation> topFor(int movieID) = 0;
    virtual void addLike(int userID, int movieID) = 0;
};

unique_ptr<Recommender> makeApproach(const string& name);

vector<string> approachNames();

class BruteForceRecommender : public Recommender {
private:
    unordered_map<int, vector<int>> usersToMovies;

public:
    string label() const override { return "BruteForce"; }
    void buildFrom(const Dataset& data) override;
    vector<Recommendation> topFor(int movieID) override;
    void addLike(int userID, int movieID) override;
};

class PopularityRecommender : public Recommender {
private:
    unordered_map<int, int> movieLikeCount;

public:
    string label() const override { return "Popularity"; }
    void buildFrom(const Dataset& data) override;
    vector<Recommendation> topFor(int movieID) override;
    void addLike(int userID, int movieID) override;
};

vector<Recommendation> topTen(const unordered_map<int, int>& counts, int excludeMovieID);

#endif
