#pragma once
#include <vector>
#include <algorithm>
#include <iostream>
using namespace std;

struct PieceData
{
    int moves;
    int cost;
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
    vector<vector<PieceData>> village;
    vector<vector<PieceData>> player;
    vector<vector<PieceData>> VxP;
    vector<vector<int>> village_locations;
    vector<vector<int>> player_locations;
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

struct Observation
{
    BinaryStateSpace previous_state;
    ActionSpace action;
    PairKind type;
    BinaryStateSpace next_state;
    int count;
    bool operator==(const Observation& other) const
    {
	return previous_state == other.previous_state &&
	action == other.action &&
	type == other.type &&
	next_state == other.next_state;
    }
};

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

struct Habitat
{
    vector<vector<int>> river;
    vector<vector<vector<int>>> river_extension;
    vector<vector<int>> mountains;
    vector<vector<int>> forests;
    vector<vector<int>> bridges;
};

struct river_orientation
{
    vector<int> coordinates;
    vector<vector<int>> orientation;
    int cost;
    vector<int> new_orn_coord;
};

bool if_in_(vector<vector<int>> in, vector<int> lookup);
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
	vector<vector<int>> find_perps(const vector<int>& h1, const vector<int>& h2, int offset);
	vector<int> find_closest_edge(vector<int> coord);
	vector<vector<int>> render_river(vector<int> start, vector<vector<int>>& cost_grid, vector<vector<int>>& out);
	vector<vector<int>> find_separation(vector<int> x, const vector<int>& h1, const vector<int>& h2, int perp_dist);
	void optimize(vector<int> current_coord, const vector<vector<int>>& path, vector<vector<int>>& cost_grid, int expected_cost, int path_idx);
	void optimize(vector<vector<int>>& path);
	void preheat();
	vector<vector<int>> grow(vector<vector<int>> inp, int MOV, bool& success);
	vector<vector<int>> generate_river(const vector<int>& h1, const vector<int>& h2, int MOV);
	vector<vector<int>> generate_river(vector<int> x, const vector<int>& h1, const vector<int>& h2, int MOV, int perp_dist);
	vector<vector<vector<int>>> mutate_river(const vector<int>& h1, const vector<int>& h2, vector<vector<int>> river, int MOV);
	bool bridgable(vector<vector<int>> validation_pair, vector<int> river_tile, vector<vector<int>> mainstream);
	vector<vector<int>> bridgable(vector<vector<int>> river);
	vector<vector<int>> get_path(vector<int> current, vector<int> target, int MOV);
};


// DONT FORGET TO PREHEAT THE HEATMAP BEFORE RIVER GENERATION!!!!

class TerrainEngine
{
    private:
	TerrainSeed seed;
	BoardState board;
	BoardState board_init(const vector<vector<int>>& guilds, const vector<vector<int>>& villages, int MOV);
    bool spaced(TerrainSeed& draft, const vector<vector<int>>& coords, const vector<int>& new_pt, int MIN_DIST, int MOV);
    vector<vector<int>> random_n_coords(TerrainSeed& draft, int N, int MIN_DIST, int MOV);
	
    public:
	TerrainEngine(vector<vector<int>>& map, vector<vector<int>> guilds, vector<vector<int>> villages, int MOV);
	TerrainEngine(vector<vector<int>>& map, int Nobj, int MINDIST, int MOV);
	BoardState& get_board();
	TerrainSeed& get_seed();
	BoardState simulate_init(TerrainSeed& draft, const vector<vector<int>>& guilds, const vector<vector<int>>& villages, int MOV);
	BoardState simulate_init(TerrainSeed& draft, int Nobj, int MINDIST, int MOV);
	BoardState simulate(TerrainSeed& draft, BoardState board, vector<vector<int>>& cost_grid, int MOV);
    void simulate(vector<Observation>& episode, vector<Observation>& obs_directory, Habitat& mainenv, TerrainSeed& draft, int Nobj, int MINDIST, int MOV, BinaryStateSpace& desired_state, int global_tries, bool& status);
	void update(TerrainSeed& draft, BoardState& new_board, int MOV);
	BinaryStateSpace EvalBinaryState(BoardState& board);
    PositionBalance EvalPosition(int MOV, TerrainSeed& draft, BoardState& board);
	int ActionRNG(int count);
    Action Sample_Action(BoardState& board, const TerrainSeed& draft, Habitat& hab);
    Observation Perform_Action(Habitat& mainenv, Action& action, BoardState& board, TerrainSeed& draft, int MOV);
    int choose_extension_randomly(const vector<vector<vector<int>>>& ext);
    vector<vector<int>> distance_field(const vector<int>& start, const vector<vector<int>>& comap, int MOV);
};

struct Snapshot
{
    Habitat env;
    TerrainSeed seed;
    BoardState board;
    vector<Observation> observations;
};
