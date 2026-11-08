//
// Created by cakepiewaffles on 11/8/26.
//
#include "graph.h"

#include <algorithm>
#include <queue>
using namespace std;

//Add a movie and title to the database
void Graph::addMovie(int movieID, const string& title) {
    movieTitles[movieID] = title;
}

//Add a like (graph edge between user and movie) if rating is 4 or above
void Graph::addRating(int userID, int movieID, double rating) {
    if (rating>=4.0) {
        moviesToUsers[movieID].push_back(userID);
        usersToMovies[userID].push_back(movieID);
    }
}

//return top 10 recommendations for a movie best to worst--
//{number of users who liked both movies, movieID for recommended movie}
vector<pair<int, int>> Graph::recommendedMovies(int movieID) {
    //Movie ID and the number of users that liked it
    unordered_map<int, int> movieLikeCount;

    ////???

    //vector of users to transverse
    vector<int> users = moviesToUsers[movieID];

    //transverse each users liked movies
    for (int userID : users) {
        //vector of movies to transverse
        vector<int>& movies = usersToMovies[userID];

        for (int thisMovie: movies) {
            //if original movie don't recommend
            if (thisMovie==movieID) {continue;}
            //otherwise add to count
            {movieLikeCount[thisMovie]++;}
        }
    }

    //Heap time!
    //minheap {likeCount, movieID}
    //a.first = likeCount
    //a.second = movieID
    //if two movies tie, the bigger movieID is demoted--sorry, the other guy was here first
    //lambda function so I don't have to make a helper
    auto compare = [](const pair<int, int>& a, const pair<int, int>& b) {
        //If there's no tie, just return true if a is bigger
        if (a.first!=b.first) {return a.first > b.first;}
        //break the tie with the movieIDs
        return a.second < b.second;
    };

    //Make a priority que
    //priority_queue<DataType, Container, Comparator> name;
    //DataType = pair<int, int> where we have {likeCount, movieID}
    //Container = vector<pair<int, int>> which is the vector ranking
    //Comparator = decltype(compare) this is just telling C++ to use the datatype of our comparator
    //where are comparator is a lambda function -- so we don't have to do any more work with that
    //name = minHeap(compare): uses our comparator to make the heap

    priority_queue<pair<int, int>, vector<pair<int, int>>, decltype(compare)> minHeap(compare);

    for (const auto& entry : movieLikeCount) {
        //put each pair in the entry into a separate variable for ease of access
        int thisMovie = entry.first;
        int count = entry.second;

        //and combine again
        pair<int, int> candidate = {count, thisMovie};

        //if the minHeap has less than 10 entries, push this movie onto it
        if (minHeap.size() <10){minHeap.push(candidate);}
        //candidate is worse than current worse
        else if (compare(minHeap.top(), candidate)){continue;}
        else if (candidate.first > minHeap.top().first ||
            (candidate.first == minHeap.top().first &&
                candidate.second < minHeap.top().second)) {
            minHeap.pop();
            minHeap.push(candidate);
            //get rid of old put in new
        }
    }

    vector<pair<int, int>> results;
    //put everything into the results vector worse to best
    while (!minHeap.empty()){results.push_back(minHeap.top()); minHeap.pop();}
    //reverse to get best to worst
    reverse(results.begin(), results.end());

    //BEST TO WORSE!!
    return results;
}

//Getter
string Graph::getMovieTitle(int movieID) {
    return movieTitles[movieID];
}

