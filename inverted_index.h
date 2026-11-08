#ifndef PROJECT2_INVERTED_INDEX_H
#define PROJECT2_INVERTED_INDEX_H

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class CoWatchIndex {
private:
    std::unordered_map<int, std::vector<int>> usersToMovies;
    std::unordered_map<int, std::vector<int>> moviesToUsers;
    std::unordered_map<int, std::unordered_map<int, int>> coWatch;
    std::unordered_map<int, std::string> movieTitles;
    bool tableBuilt = false;

public:
    void addMovie(int movieID, const std::string& title);
    void addRating(int userID, int movieID, double rating);
    void buildTable();
    std::vector<int> usersWhoLikedBoth(int movie1, int movie2);
    std::vector<std::pair<int, int>> recommendedMovies(int movieID);
    std::string getMovieTitle(int movieID);
};

#endif
