#include "inverted_index.h"

#include <algorithm>

using namespace std;

void CoWatchIndex::addMovie(int movieID, const string& title) {
    movieTitles[movieID] = title;
}

string CoWatchIndex::getMovieTitle(int movieID) {
    auto movie = movieTitles.find(movieID);
    if (movie == movieTitles.end()) {
        return "";
    }
    return movie->second;
}

void CoWatchIndex::addRating(int userID, int movieID, double rating) {
    if (!(rating >= 4.0)) {
        return;
    }

    vector<int>& movies = usersToMovies[userID];
    if (find(movies.begin(), movies.end(), movieID) != movies.end()) {
        return;
    }

    if (tableBuilt) {
        for (int otherID : movies) {
            int smaller = min(movieID, otherID);
            int larger = max(movieID, otherID);
            coWatch[smaller][larger]++;
        }
        movies.insert(lower_bound(movies.begin(), movies.end(), movieID), movieID);
        vector<int>& users = moviesToUsers[movieID];
        users.insert(lower_bound(users.begin(), users.end(), userID), userID);
    } else {
        movies.push_back(movieID);
        moviesToUsers[movieID].push_back(userID);
    }
}

void CoWatchIndex::buildTable() {
    if (tableBuilt) {
        return;
    }
    coWatch.clear();

    for (auto& movie : moviesToUsers) {
        sort(movie.second.begin(), movie.second.end());
    }

    for (auto& user : usersToMovies) {
        vector<int>& movies = user.second;
        sort(movies.begin(), movies.end());

        for (size_t i = 0; i < movies.size(); i++) {
            for (size_t j = i + 1; j < movies.size(); j++) {
                coWatch[movies[i]][movies[j]]++;
            }
        }
    }
    tableBuilt = true;
}

vector<int> CoWatchIndex::usersWhoLikedBoth(int movie1, int movie2) {
    vector<int> common;
    auto first = moviesToUsers.find(movie1);
    auto second = moviesToUsers.find(movie2);
    if (first == moviesToUsers.end() || second == moviesToUsers.end()) {
        return common;
    }

    const vector<int>& users1 = first->second;
    const vector<int>& users2 = second->second;
    size_t i = 0;
    size_t j = 0;

    while (i < users1.size() && j < users2.size()) {
        if (users1[i] == users2[j]) {
            common.push_back(users1[i]);
            i++;
            j++;
        } else if (users1[i] < users2[j]) {
            i++;
        } else {
            j++;
        }
    }
    return common;
}

vector<pair<int, int>> CoWatchIndex::recommendedMovies(int movieID) {
    vector<pair<int, int>> results;
    if (moviesToUsers.find(movieID) == moviesToUsers.end()) {
        return results;
    }

    for (const auto& movie : moviesToUsers) {
        int otherID = movie.first;
        if (otherID == movieID) {
            continue;
        }

        int smaller = min(movieID, otherID);
        int larger = max(movieID, otherID);
        auto row = coWatch.find(smaller);
        if (row == coWatch.end()) {
            continue;
        }
        auto count = row->second.find(larger);
        if (count != row->second.end()) {
            results.push_back({count->second, otherID});
        }
    }

    sort(results.begin(), results.end(),
         [](const pair<int, int>& a, const pair<int, int>& b) {
             if (a.first != b.first) {
                 return a.first > b.first;
             }
             return a.second < b.second;
         });

    if (results.size() > 10) {
        results.resize(10);
    }
    return results;
}
