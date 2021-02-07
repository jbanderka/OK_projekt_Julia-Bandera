#include <iostream>
#include <vector>
#include <random>
#include "graph.h"

#define NUM_OF_INSTANCES 100

using namespace std;

int main() {
    cout << "Podaj ilosc wierzcholkow grafu: " << endl;
    int number_of_v = 0;
    cin >> number_of_v;
    vector<class Graph> all_graphs;
    random_device rd;
    int summary_cost = 0;
    for(int instance = 0; instance < NUM_OF_INSTANCES; instance++) {
        default_random_engine random_engine(rd());
        Graph graph(number_of_v, random_engine);
        ACOalgorithm(graph);
        all_graphs.push_back(graph);
        cout << "Instancja " << instance + 1 << " - koszt sciezki: " << graph.best_solution.cost << endl;
        summary_cost += graph.best_solution.cost;
    }
    cout << "Sredni koszt: " << summary_cost / NUM_OF_INSTANCES << endl;
    return 0;
}
