#ifndef GRAFY_OK_GRAPH_H
#define GRAFY_OK_GRAPH_H


#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>

#define TIME 2     // czas pracy dzialania algorytmu dla kazdej instancji (kazdego grafu)

#define MINDEG 1   // minimalny stopien wierzcholka grafu
#define MAXDEG 6    // maksymalny stopien wierzcholka grafu

#define MIN_W 1     // minimalna waga krawedzi w grafie
#define MAX_W 100   // maksymalna waga krawedzi w grafie

#define X 5         // 'kara' za dodanie krawedzi do sciezki o wadze wiekszej niz waga nastepnej krawedzi

#define NUM_OF_ANTS 100  // ilosc mrowek w kazdej iteracji ACO
#define EVAPORATION 0.1 // parowanie feromonow (gdy pomnozone przez 100, to w skali 1 - 100%)
#define SMOOTHENING 0.04    // warunek wygladzenia - jesli obliczona wartosc mniejsza niz ta, wymagane jest wygladzenie feromonow

using namespace std;

struct S {          // struktura odzwierciedlajaca znaleziona sciezke w grafie
    vector<int> path;       // wektor przechowujacy indeksy kolejnych wierzcholkow sciezki
    vector<int> costList;   // wektor przechowujacy wagi kolejnych krawedzi w sciezce
    int cost = 0;           // sumaryczny koszt sciezki
};

class Graph {
    int vertices;
public:
    vector<vector<int>> adjList;    // lista sasiedztwa
    vector<vector<int>> weight;     // macierz przechowujaca wagi krawedzi
    vector<struct S> paths;         // wektor przechowujay sciezki grafu
    vector<vector<double>> feromones;  // macierz feromonów na krawędziach
    S best_solution;                // najlepsze rozwiazanie (sciezka) w grafie

    Graph(int v, default_random_engine random_engine) {
        vertices = v;
        vector<int> tempV;
        vector<int> tempW;
        vector<double> tempF;
        tempW.assign(vertices, 0);
        tempF.assign(vertices, 0);
        for(int i = 0; i < vertices; i++) {     // utworzenie pustej listy sasiedztwa i macierzy wag wypelnionej zerami
            adjList.push_back(tempV);
            weight.push_back(tempW);
            feromones.push_back(tempF);
        }
        uniform_int_distribution<int> num(0, vertices - 1);
        uniform_int_distribution<int> edgeW(MIN_W, MAX_W);
        vector<int> unused;     // wektor przechowujacy wierzcholki, ktore nie sa zawarte w sciezce
        unused.reserve(vertices);
        for(int i = 0; i < vertices; i++){
            unused.push_back(i);
        }
        bool all_v_connected = false;
        int v1 = num(random_engine);
        unused.erase(find(unused.begin(), unused.end(), v1));
        int v2;
        while(!all_v_connected) {
            bool used = true;
            do {
                v2 = num(random_engine);   // wylosowanie sasiada wierzcholka prev
                for(int i : unused) {    // sprawdzenie czy dany wierzcholek byl juz uzyty
                    if(i == v2) {
                        used = false;
                        break;
                    }
                }
            }
            while(used);
            int w = edgeW(random_engine);   // wylosowanie wagi nowej krawedzi
            addEdge(v1, v2, w);      // dodanie krawedzi o wadze 'w' pomiedzy 'v1' a 'v2'
            v1 = v2;                 // v2 staje sie v1 i teraz bedzie poszukiwana krawedz dla v2 (juz teraz v1)
            for(auto i = unused.begin(); i < unused.end(); i++)
                if(*i == v2)
                    unused.erase(i);    // usuniecie indeksu wierzcholka z wektora wierzcholkow nieuzytych
            if(unused.empty()) {
                all_v_connected = true;
            }
        }
        uniform_int_distribution<int> deg(MINDEG, MAXDEG);
        for(int i = 0; i < vertices; i++) {        // dodanie krawedzi
            int degree = deg(random_engine);    // dla kazdego wierzcholka losowany jest jego stopien
            for(int j = adjList[i].size(); j < degree; j++) {
                int neighbour;
                bool already;
                do {
                    neighbour = num(random_engine);
                    already = edgeExists(i, neighbour);
                }
                while((neighbour == i) || (adjList[neighbour].size() == MAXDEG) || already);
                // poszukiwanie wierzcholka o innym indeksie niz aktualny, ze stopniem mniejszym niz max. i niemajacego juz krawedzi z aktualnym wierzcholkiem
                int w = edgeW(random_engine);   // wylosowanie wagi nowej krawedzi
                addEdge(i, neighbour, w);      // dodanie krawedzi o wadze 'w' pomiedzy 'i' a 'neighbour'
            }
        }
        for(auto & i : adjList) {       // sortowanie sasiadow w liscie sasiedztwa kazdego wierzcholka
            sort(i.begin(), i.end());
        }
    }

    bool edgeExists(int a, int b) {     // metoda sprawdza czy istnieje juz krawedz miedzy podanymi wierzcholkami
        for(int i = 0; i < adjList[a].size(); i++) {
            if(adjList[a][i] == b)
                return true;
        }
        return false;
    }

    void addEdge(int a, int b, int w) {     // metoda tworzy krawedz o wadze 'w' pomiedzy wierzcholkami 'a' i 'b'
        adjList[a].push_back(b);
        adjList[b].push_back(a);
        weight[a][b] = w;
        weight[b][a] = w;
    }

    bool feromones_present(int v1) {        // metoda sprawdza czy na jakiejkolwiek krawedzi podanego wierzcholka sa feromony
        int v2;
        for(int i = 0; i < adjList[v1].size(); i++) {
            v2 = adjList[v1][i];
            if(feromones[v1][v2] != 0)
                return true;
        }
        return false;
    }

    int draw_next(int vertex, default_random_engine random_engine) {    // wylosowanie kolejnego wierzcholka do sciezki wykorzystujac feromony
        int all_feromones = 0, probability, next = adjList[vertex][0];
        double min_f = feromones[vertex][adjList[vertex][0]];   // najmniejsza ilosc feromonow na krawedziach wierzcholka 'vertex'
        double max_f = feromones[vertex][adjList[vertex][0]];   // najwieksza ilosc feromonow na krawedziach wierzcholka 'vertex'
        vector<double> chances_to_use_v;    // wektor przechowujacy najpierw ilosc feromonow na krawedziach, pozniej procent szans na wylosowanie danej krawedzi
        for(int i = 0; i < adjList[vertex].size(); i++) {
            int n = adjList[vertex][i];
            all_feromones += feromones[vertex][n];  // sumowanie liczby feromonow na krawedziach wierzcholka 'vertex'
            chances_to_use_v.push_back(feromones[vertex][n]);
            if(feromones[vertex][n] < min_f && feromones[vertex][n] != 0)
                min_f = feromones[vertex][n];
            if(feromones[vertex][n] > max_f)
                max_f = feromones[vertex][n];
        }
        if(min_f / max_f < SMOOTHENING) {  // wygladzanie feromonow jesli min_f stanowi mniej niz podana ilość procent max_f
            double all_f = 0;
            for(int i = 0; i < chances_to_use_v.size(); i++) {
                double f_part = chances_to_use_v[i] / all_feromones;
                chances_to_use_v[i] = (1 - f_part) * chances_to_use_v[i];
                all_f += chances_to_use_v[i];
            }
            all_feromones = all_f;
        }
        int sum_p = 0;
        for(int i = 0; i < chances_to_use_v.size(); i++) {
            probability = 100 * chances_to_use_v[i] / all_feromones;
            chances_to_use_v[i] = probability;
            sum_p += probability;
        }
        uniform_int_distribution<int> get_p(1, sum_p);
        int p = get_p(random_engine);
        int start = 1, end = 0;
        for(int i = 0; i < adjList[vertex].size(); i++) {   // sprawdzenie, ktora krawedz zostala wylosowana
            end += chances_to_use_v[i];
            if(p >= start && p <= end) {
                next = adjList[vertex][i];
                break;
            }
            start += chances_to_use_v[i];
        }
        chances_to_use_v.clear();
        return next;
    }

    void findPath(default_random_engine random_engine, int chance_f) {
        vector<int> unused;     // wektor przechowujacy wierzcholki, ktore nie sa zawarte w sciezce
        unused.reserve(vertices);
        for(int i = 0; i < vertices; i++){
            unused.push_back(i);
        }
        uniform_int_distribution<int> v(0, vertices - 1);   // losowanie indeksu pierwszego wierzcholka
        int first = v(random_engine);
        S solution;
        solution.path.push_back(first);
        unused.erase(find(unused.begin(), unused.end(), first));
        uniform_int_distribution<int> chance(1, 100);
        bool all_used = false;
        while(!all_used) {
            int prev = solution.path[solution.path.size() - 1]; // do prev przypisywany ostatni wierzcholek sciezki
            int vertex;
            bool using_feromones = false;
            if(feromones_present(prev) && chance_f != 0) {
                int ch = chance(random_engine);
                if(ch <= chance_f) {    // wykorzystywana macierz feromonów
                    using_feromones = true;
                    vertex = draw_next(prev, random_engine);
                }
            }
            if(!using_feromones) {
                uniform_int_distribution<int> index(0, adjList[prev].size() - 1);
                vector<int> temp;   // bedzie przechowywal sasiadow prev, ktore znajduja sie juz na sciezce
                bool neighbours_used = false, used;
                do {
                    int ind = index(random_engine);
                    vertex = adjList[prev][ind];   // wylosowanie sasiada wierzcholka prev
                    used = true;
                    for(int i : unused) {    // sprawdzenie czy dany wierzcholek byl juz uzyty
                        if(i == vertex) {
                            used = false;
                            break;
                        }
                    }
                    bool found = false;
                    for(int i : temp)    // sprawdzenie czy dany wierzcholek jest juz w wektorze temp
                        if(i == vertex)
                            found = true;
                    if(!found) temp.push_back(vertex); // dodanie wierzcholka do temp jesli jeszcze sie tam nie znajdowal
                    sort(temp.begin(), temp.end());
                    //jesli dany wierzcholek i inni sasiedzi prev sa juz na sciezce
                    if(used && equal(temp.begin(), temp.end(), adjList[prev].begin()))
                        neighbours_used = true;
                }
                while(used && !neighbours_used);
                temp.clear();
                if(neighbours_used && adjList[prev].size() != 1) { // jesli wszyscy sasiedzi prev sa juz na sciezce
                    int ind = index(random_engine);
                    vertex = adjList[prev][ind];
                    // dopoki kandydat na kolejny element sciezki nie jest rowny przedostniemu wierzcholkowi
                    // zapobiega niepotrzebnym zapetleniom w sciezce
                }
            }
            solution.path.push_back(vertex);    // wierzcholek umieszczany jest na sciezce
            for(auto i = unused.begin(); i < unused.end(); i++)
                if(*i == vertex)
                    unused.erase(i);    // usuniecie indeksu wierzcholka z wektora wierzcholkow nieuzytych
            solution.costList.push_back(weight[prev][vertex]); // waga nowej krawedzi dodawana jest do listy kosztów
            if(unused.empty()) {
                all_used = true;
            }
        }
        calculate_cost(solution);
        paths.push_back(solution);
    }

    static void calculate_cost(S &sol) { //obliczenie calkowitego kosztu sciezki
        sol.cost = 0;
        unsigned int last = sol.costList.size() - 1;
        for(int i = 0; i < sol.costList.size(); i++) {
            if(sol.costList[i] == sol.costList[last]) {
                sol.cost += sol.costList[i];
            }
            else {
                int j = i + 1;
                if(sol.costList[i] > sol.costList[j])   // jesli waga dodawanej krawedzi jest wieksza niz waga kolejnej
                    sol.cost += X * sol.costList[i];
                else
                    sol.cost += sol.costList[i];
            }
        }
    }

    void first_feromone_distribution() {    // dystrybucja feromonow po pierwszej iteracji
        for(int j = 0; j < best_solution.path.size() - 1; j++) {
            int v1 = best_solution.path[j];
            int v2 = best_solution.path[j + 1];
            feromones[v1][v2] += 1;
            feromones[v2][v1] += 1;
        }
        feromone_evaporation();  // parowanie feromonow
    }

    void distribute_feromones() {       // dystrybucja feromonow
        int ind = best_path_index();
        int best_cost = best_solution.cost;
        if(paths[ind].cost <= best_cost) {
            double f = double(best_cost) / double(paths[ind].cost);
            for(int j = 0; j < paths[ind].path.size() - 1; j++) {
                int v1 = paths[ind].path[j];
                int v2 = paths[ind].path[j + 1];
                feromones[v1][v2] += f;
                feromones[v2][v1] += f;
            }
            feromone_evaporation();  // parowanie feromonow
        }
    }

    void feromone_evaporation() {    // parowanie feromonow
        double not_evaporated = 1 - EVAPORATION;
        for(int i = 0; i < feromones.size(); i++)
            for(int j = 0; j < feromones[i].size(); j++)
                feromones[i][j] *= not_evaporated;
    }

    int best_path_index() {     // metoda zwraca indeks najlepszej sciezki w danej iteracji (nie bierze pod uwage best_solution)
        unsigned int index = 0;
        int best_cost = adjList.size() * MAX_W * X;
        for(int i = 0; i < paths.size(); i++)
            if(paths[i].cost < best_cost) {
                best_cost = paths[i].cost;
                index = i;
            }
        return index;
    }

    ~Graph() = default;
};


void ACOalgorithm (Graph &g) {
    random_device rd;
    int using_feromones_chance = 0;
    auto start = std::chrono::steady_clock::now();
    for(int a = 0; ; ++a) {
        if(a == 0) {
            for(int i = 0; i < NUM_OF_ANTS; i++) {
                default_random_engine random_engine(rd());
                g.findPath(random_engine, using_feromones_chance);
            }
            int ind = g.best_path_index();
            g.best_solution = g.paths[ind];     // najlepszym rozwiazaniem instancji bedzie najlepsze rozwiazanie znalezione w pierwszej iteracji
            g.first_feromone_distribution();
        }
        else {
            if(using_feromones_chance < 100)
                using_feromones_chance += 1;    // zwiekszanie szansy na skorzystanie z feromonow po kazdej iteracji
            for(int i = 0; i < NUM_OF_ANTS; i++) {
                default_random_engine random_engine(rd());
                g.findPath(random_engine, using_feromones_chance);
            }
            int ind = g.best_path_index();
            if(g.paths[ind].cost <  g.best_solution.cost)
                g.best_solution = g.paths[ind]; // jesli znaleziono lepsze rozwiazanie, zostaje ono przypisane do best_solution
            g.distribute_feromones();
        }
        g.paths.clear();
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - start;
        if(elapsed_seconds.count() > TIME)
            break;      // wyjscie z petli po przekroczeniu czasu ulepszania jednej instancji
    }
}


#endif //GRAFY_OK_GRAPH_H
