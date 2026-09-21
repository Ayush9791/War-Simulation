#pragma once
#include <vector>
#include <algorithm>
#include <iostream>
#include <random>
using namespace std;

struct Habitat
{
    vector<vector<int>> spawn;
    vector<vector<int>> village;
    vector<vector<int>> river;
    vector<vector<vector<int>>> river_extension;
    vector<vector<int>> mountains;
    vector<vector<int>> forests;
    vector<vector<int>> bridges;
};

enum habitat
{
    SPAWN,
    VILLAGE,
    RIVER,
    RIVEREXT,
    MOUNTAIN,
    FOREST,
    BRIDGE
};

enum BinaryStateSpace
{
    ClearOwn,
    Unfair,
    FullyIsolated,
    PartIsolated,
    Balanced,
    Unbalanced,
    NOTA,
    COUNT
};

struct BoardState
{
    vector<vector<int>> VxV;
    vector<vector<int>> PxP;
    vector<vector<int>> VxP;
    vector<vector<int>> village_locations;
    vector<vector<int>> player_locations;
};

struct speck
{
    double none;
    double forest;
};

enum logic
{
    every,
    any,
};

enum class ActionSpace
{
    GenerateRiver,
    MutateRiver,
    BuildBridge,
    COUNT
};

enum class PairKind { PP, PV, VV, COUNT };

struct Action
{
    ActionSpace action;
    PairKind type;
    vector<vector<int>> args;
};

struct PositionBalance
{
    vector<vector<vector<int>>> position;  // Per village cost for (Player X - Player Y) [V, Pi, Pj]
    vector<vector<int>> combat;            // Distance matrix of each player to another player [Pi, Pj]
};

template <typename T>
int randbetween(T a, T b)
{
    static random_device rd;
    static mt19937 rng(rd());
    uniform_int_distribution dist(a, b);
    return dist(rng);
}

template <typename T, typename Func>
bool check(logic q, const T& a, Func F)
{
    bool out_a = true;
    bool out_b = false;
    if (q == every) { for (const auto& x : a) { out_a = out_a && F(x); } return out_a; }
    else if (q == any) { for (const auto& x : a) { out_b = out_b || F(x); } return out_b; }
    return false;
}

template <typename T>
bool check_equality(const T& a)
{
    bool out_a = true;
    for (int i=1; i < a.size(); i++) { out_a = out_a && (a[i] == a[i-1]); } return out_a;
}

struct river_orientation
{
    vector<int> coordinates;
    vector<vector<int>> orientation;
    int cost;
    vector<int> new_orn_coord;
};

template <typename T>
bool if_in_(vector<vector<T>> in, vector<T> lookup)
{
    for (vector<T> i : in) { if (i == lookup) { return true; } }
    return false;
}

template <typename T>
bool if_in_(vector<T> in, T lookup)
{
    for (T i : in) { if (i == lookup) { return true; } }
    return false;
}

template <typename T>
vector<vector<T>> shuffle(vector<vector<T>>& x)
{
    vector<vector<int>*> ptr;
    vector<vector<T>> out;
    for (vector<int>& i : x)
    {
        ptr.push_back(&i);
    }
    static random_device rd;
    static mt19937 rng(rd()); 
    
    while (!ptr.empty())
    {
        uniform_int_distribution<int> dist(0, ptr.size()-1);
        int idx = dist(rng);
        out.push_back(*ptr[idx]);
        ptr.erase(ptr.begin()+idx);
    }
    return out;
}


bool twobytwo(vector<int> new_coord, vector<vector<int>> river);

class TerrainSeed
{
private:
    vector<vector<int>> map;
    vector<vector<int>> cost_map;
    vector<vector<double>> explored;
    int WIDTH;
    int HEIGHT;
	
public:
    TerrainSeed(vector<vector<int>> map);
    int draw_offset();
    void draw(vector<vector<int>> coord, int MOV);
    const int calculate_cost(const vector<vector<int>>& path, vector<vector<int>>& in_comap);
    bool check_diagonality(const vector<int>& a, const vector<int>& b);
    void swap(vector<int>& out, const vector<int>& a, const vector<int>& b);
    void smoothen(vector<vector<int>>& path);
    vector<vector<int>>& get_map();
    vector<vector<double>>& get_explored();
    vector<vector<int>>& get_cost_map();
    int get_width();
    int get_height();
    double cart_dist(vector<int> target, vector<int> inp);
    vector<int> random_edge();
    vector<vector<int>> random_two_coords();
    int random_coord(bool row);
    vector<int> random_coord();
    vector<vector<int>> find_perps(const vector<int>& h1, const vector<int>& h2, int offset);
    vector<int> find_closest_edge(vector<int> coord);
    vector<vector<int>> distance_field(const vector<int>& start, const vector<vector<int>>& comap, vector<vector<double>>& explored, int MOV);
    void locate(vector<vector<int>>& out, const vector<int>& start, const vector<int>& end, const vector<vector<int>>& dijkstra, int MOV);
    vector<vector<int>> locate(const vector<int>& start, const vector<int>& end, vector<vector<int>>& comap, vector<vector<double>>& explored, int MOV);

    // RIVER
    vector<vector<int>> render_river(vector<int> start, vector<vector<int>>& cost_grid, vector<vector<int>>& out);
    vector<vector<int>> find_separation(vector<int> x, const vector<int>& h1, const vector<int>& h2, int perp_dist);
    void optimize(vector<int> current_coord, const vector<vector<int>>& path, vector<vector<int>>& cost_grid, int expected_cost, int path_idx);
    void optimize(vector<vector<int>>& path);
    void preheat();
    vector<vector<int>> grow(vector<vector<int>> inp, int MOV, bool& success);
    vector<vector<int>> generate_river(const vector<int>& h1, const vector<int>& h2, int MOV);
    vector<vector<int>> generate_river(vector<int> x, const vector<int>& h1, const vector<int>& h2, int MOV, int perp_dist);
    vector<vector<vector<int>>> mutate_river(const vector<int>& h1, const vector<int>& h2, vector<vector<int>> river, int MOV);

    // BRIDGE
    bool bridgable(vector<vector<int>> validation_pair, vector<int> river_tile, vector<vector<int>> mainstream);
    vector<vector<int>> bridgable(vector<vector<int>> river);
    vector<vector<int>> get_path(vector<int> current, vector<int> target, int MOV);
    void build_bridge(const vector<vector<int>>& river, const vector<int>& point, int MOV);

    // FOREST
    bool paint(const speck& prob);
    vector<vector<int>> generate_forest(vector<int> spawnpoint, int radius, int terrain_cost, int MOV);
};

struct Assets
{
    TerrainSeed seed;
    BoardState board;
    Habitat env;
    Assets(vector<vector<int>> map) : seed(map), board{}, env{} {}
};


class TerrainEngine
{
private:
    Assets assets;
    BoardState board_init(const vector<vector<int>>& guilds, const vector<vector<int>>& villages, int MOV);
    bool spaced(TerrainSeed& draft, const vector<vector<int>>& coords, const vector<int>& new_pt, int MIN_DIST, int MOV);
    vector<vector<int>> random_n_coords(TerrainSeed& draft, int N, int MOV);
    
public:
    TerrainEngine(vector<vector<int>>& map, vector<vector<int>> guilds, vector<vector<int>> villages, int MOV);
    TerrainEngine(vector<vector<int>>& map, int Nobj, int MOV);
    Assets& get_asset();
    vector<vector<int>> ValidRiverExtension(Assets& asset, int MOV);

    // NEW GENERATION MACHINERY
    BoardState EvalBoard(Assets& asset, vector<vector<int>>& cost_grid, int MOV);
    bool ValidateIsolation(const vector<vector<int>>& grid);
    bool ValidateBalance(const vector<double>& scores);
    vector<double> AnalyseBoard(BoardState& board, int MOV);
    void Push(Assets& asset, int MOV);
    Assets GenerateRiverSystem(Assets& asset, int MOV);
    Assets Generate(Assets& asset, int MOV);
};
