
#include <iostream>
#include "graph.h"

int main() {

    //Here's a small test!!

    Graph graph;

    // Add movies
    graph.addMovie(10, "Movie A");
    graph.addMovie(20, "Movie B");
    graph.addMovie(30, "Movie C");
    graph.addMovie(40, "Movie D");
    graph.addMovie(50, "Movie E");

    // User 1's ratings
    graph.addRating(1, 10, 5.0);
    graph.addRating(1, 20, 4.0);
    graph.addRating(1, 30, 5.0);

    // User 2's ratings
    graph.addRating(2, 10, 4.0);
    graph.addRating(2, 20, 5.0);
    graph.addRating(2, 40, 4.0);

    // User 3's ratings
    graph.addRating(3, 10, 5.0);
    graph.addRating(3, 30, 4.0);
    graph.addRating(3, 50, 5.0);

    // Get all recommendations if someone likes movie 10 (Movie A)
    vector<pair<int, int>> recommendations10 = graph.recommendedMovies(10);

    //Display recommendations if you like Movie 10
    cout<<"Recommendations if you like " << graph.getMovieTitle(10) <<endl;

    for (const auto& recommendation : recommendations10) {
        int count = recommendation.first;
        int movieID = recommendation.second;

        cout << graph.getMovieTitle(movieID) << " | Shared Likes: " << count << endl;
    }

    cout << endl;

    // Get all recommendations if someone likes movie 20 (Movie B)
    vector<pair<int, int>> recommendations20 = graph.recommendedMovies(20);

    //Display recommendations if you like Movie 20
    cout<<"Recommendations if you like " << graph.getMovieTitle(20) <<endl;

    for (const auto& recommendation : recommendations20) {
        int count = recommendation.first;
        int movieID = recommendation.second;

        cout << graph.getMovieTitle(movieID) << " | Shared Likes: " << count << endl;
    }

    cout << endl;

    // Get all recommendations if someone likes movie 30 (Movie C)
    vector<pair<int, int>> recommendations30 = graph.recommendedMovies(10);

    //Display recommendations if you like Movie 30
    cout<<"Recommendations if you like " << graph.getMovieTitle(30) <<endl;

    for (const auto& recommendation : recommendations30) {
        int count = recommendation.first;
        int movieID = recommendation.second;

        cout << graph.getMovieTitle(movieID) << " | Shared Likes: " << count << endl;
    }
}



