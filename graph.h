//
// Created by cakepiewaffles on 11/8/26.
//

#ifndef PROJECT2_GRAPH_H
#define PROJECT2_GRAPH_H
//#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>

using namespace std;

/*
 *Bi-directional bipartite graph (yes I'm a math major and took graph theory)
 *The two groups of vertices are movies and users
 *We will use two adjacency lists to store only the edges that exist between the two groups
 *We can use moviesIDs and userIDs as keys due to the database we are using
 *When I say a user "liked" a movie, I mean they rated it a 4.0 or above
*/

class Graph {
private:

    //Goes from the userID to the movies that user liked
    unordered_map<int, vector<int>> usersToMovies;
    //Goes from the movieID to the users that liked it
    unordered_map<int, vector<int>> moviesToUsers;

    //Goes from movieID to movieTitle
    unordered_map<int, string> movieTitles;

public:
    //Add a movie and title to the database--just connects movieID to title
    void addMovie(int movieID, const string& title);

    //Add a like (graph edge between user and movie) if rating is 4 or above
    void addRating(int userID, int movieID, double rating);

    //return top 10 recommendations for a movie best to worst--
    //{number of users who liked both movies, movieID for recommended movie}
    vector<pair<int, int>> recommendedMovies(int movieID);

    //Getter
    string getMovieTitle(int movieID);

};

#endif //PROJECT2_GRAPH_H
