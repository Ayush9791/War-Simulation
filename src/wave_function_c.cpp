#include "wave_function_c.h"
#include "plotter.h"
#include <vector>
#include <algorithm>
#include <iostream>
#include <random>
using namespace std;

vector<vector<int>> helper = {{1,0}, {-1,0}, {0,1}, {0,-1}};

vector<vector<int>> get_ring(vector<int> tile, int thickness)
{
    vector<vector<int>> thick_ring;
    for (int dx = -thickness; dx <= thickness; dx++)
        for (int dy = -thickness; dy <= thickness; dy++)
            thick_ring.push_back({tile[0] + dx, tile[1] + dy});
    return thick_ring;
}

TileSet get_new_tileset()
{
    TileSet out;
    for (int i=0; i < static_cast<int>(Tile::COUNT); i++)
    {
        Tiles sq;
        sq.tile = static_cast<Tile>(i);
        sq.prob = 0.2;
        out.space.push_back(sq);
    }
    return out;
}

Tile WaveFunctionCollapse::current_tile(const TileSet& x)
{
    for (int i=0; i < x.space.size(); i++)
    {
        if (x.space[i].prob == 1.0) { return x.space[i].tile; }
    }
    return Tile::COUNT;
}

void WaveFunctionCollapse::assign(TileSet& x, Tile tgt)
{
    for (int i=0; i < x.space.size(); i++)
    {
        x.space[i].prob = 0.0;
        if (x.space[i].tile == tgt) { x.space[i].prob = 1.0; }
    }
}

void WaveFunctionCollapse::off(TileSet& x, Tile tgt)
{
    double tgt_prob = x.space[static_cast<int>(tgt)].prob;
    x.space[static_cast<int>(tgt)].prob = 0.0;
    for (int i=0; i < x.space.size(); i++)
    {
        x.space[i].prob += (x.space[i].prob / (1-tgt_prob)) * tgt_prob;
    }
}

bool if_in_(vector<vector<int>> in, vector<int> lookup)
{
    for (vector<int> i : in) { if (i == lookup) { return true; } } return false;
}

vector<vector<vector<int>>> twobytwo_helper {{{-1, -1}, {0, -1}, {-1,  0}}, {{0, -1}, {1, -1}, {1,  0}}, {{-1,  0}, {-1,  1}, {0,  1}}, {{1,  0} ,{0,  1}, {1,  1}}};

bool WaveFunctionCollapse::twobytwo(vector<int> new_coord)
{
    bool final_valid = true;
    int x = new_coord[0];
    int y = new_coord[1];
    for (vector<vector<int>> coords : twobytwo_helper)
    {
	bool valid = false;
	for (vector<int> coord : coords)
	{
            int new_x = x + coord[0];
            int new_y = y + coord[1];
	    if (new_x < 0 || new_y < 0 || new_x >= map[0].size() || new_y >= map.size()) { continue; }
	    if (current_tile(map[new_y][new_x]) != Tile::RIVER) { valid = true; }
	    if (valid) { break; }
	}
	final_valid = final_valid && valid;
    }
    return final_valid;
}

vector<int> WaveFunctionCollapse::random_seed(int w, int h)
{
    int WIDTH = map[0].size();
    int HEIGHT = map.size();
    vector<int> out;
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> width_sample(w, WIDTH-w-1);
    uniform_int_distribution<int> height_sample(h, HEIGHT-h-1);
    return {width_sample(rng), height_sample(rng)};
}

vector<int> WaveFunctionCollapse::__init__(const vector<vector<int>>& spawnpoints, int heat_intensity)
{
    int WIDTH = ceil(map[0].size() * 0.4);
    int HEIGHT = ceil(map.size() * 0.4);
    if (spawnpoints.size() % 2 != 0) {return {};}
    for (int i=0; i < spawnpoints.size(); i++)
    {
        int x = spawnpoints[i][0];
        int y = spawnpoints[i][1];
        vector<vector<int>> ring_helper = get_ring({x, y}, heat_intensity);
        for (const vector<int>& help : ring_helper)
        {
            int rx = help[0];
            int ry = help[1];
            if (i > (spawnpoints.size() / 2) - 1) { assign(map[y][x], Tile::VILLAGE); assign(map[ry][rx], Tile::VILLAGE); }
            else { assign(map[y][x], Tile::SPAWN); assign(map[ry][rx], Tile::SPAWN); }
        }
    }
    vector<int> river_vec;
    while (true)
    {
        river_vec = random_seed(WIDTH, HEIGHT);
        if (current_tile(map[river_vec[1]][river_vec[0]]) == Tile::COUNT) { assign(map[river_vec[1]][river_vec[0]], Tile::RIVER); break; }
    }
    return river_vec;
}

bool WaveFunctionCollapse::detect_edge(const vector<int>& coord)
{
    if (coord[0] == 0 || coord[1] == 0 || coord[0] == map[0].size()-1 || coord[1] == map.size()-1) { return true; }
    { return false; }
}

vector<int> WaveFunctionCollapse::get_rivec()
{
    return rivec;
}

bool WaveFunctionCollapse::check_river_edge(const vector<vector<int>>& edge_tile)
{
    bool check = false;
    for (vector<int> vec : edge_tile)
    {
        check = check || (current_tile(map[vec[1]][vec[0]]) == Tile::RIVER);
    }
    return check;
}

WaveFunctionCollapse::WaveFunctionCollapse(int WIDTH, int HEIGHT, vector<vector<int>>& spawn) : map(vector<vector<TileSet>>(HEIGHT, vector<TileSet>(WIDTH, get_new_tileset())))
{
    rivec = __init__(spawn, 1);
};

const vector<vector<TileSet>>& WaveFunctionCollapse::get_map() const
{
    return map;
}

void WaveFunctionCollapse::EvolveTile(const vector<int>& coord)
{
    // River :
    int x = coord[0];
    int y = coord[1];
    bool edge = detect_edge({x, y});
    bool check = true;
    vector<vector<int>> edge_tile;
    if (map[y][x].space[static_cast<int>(Tile::RIVER)].prob != 0.0)
    {
        // Rule 1: If no river tile around, RIVER not possible
        for (vector<int> help : helper)
        {
            int new_x = x + help[0];
            int new_y = y + help[1];
            if (new_x < 0 || new_y < 0 || new_x >= map[0].size() || new_y >= map.size())
                continue;
            edge_tile.push_back({new_x, new_y});
            check = check && current_tile(map[new_y][new_x]) != Tile::RIVER;
        }
        // Rule 2: no 2x2
        if (check || twobytwo({x, y})) { off(map[y][x], Tile::RIVER); }
        // Rule 3: If a river end is not attached to edge and all its ends have prob 0, prob of current tile = 1
        if (edge && !check)
        {
            if (!check_river_edge(edge_tile))
            {
                assign(map[y][x], Tile::RIVER);
                for (vector<int> vec : edge_tile) { off(map[vec[1]][vec[0]], Tile::RIVER); }
            }
            else
                off(map[y][x], Tile::RIVER);
        }
    }
}

bool contains(vector<int> vec, int x)
{
    bool out = false;
    for (int a : vec) { if (x == a) { return true; } }
    return false;
}

bool coin_flip()
{
    vector<int> out;
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution dist(0, 1);
    return dist(rng);
}

bool diverge(double prob)
{
    random_device rd;
    mt19937 rng(rd());
    bernoulli_distribution dist(prob);
    return dist(rng);
}

vector<vector<int>> WaveFunctionCollapse::Evolve(vector<vector<int>>& seed)
{
    vector<vector<int>> out;
    for (int i=0; i < seed.size(); i++)
    {
        int x = seed[i][0];
        int y = seed[i][1];
        // RIVER
        if (current_tile(map[y][x]) == Tile::RIVER)
        {
            bool decided = false;
            bool edge = false;
            bool empty = true;
            vector<vector<int>> check_edges;
            for (vector<int> help : helper)
            {
                int new_x = x + help[0];
                int new_y = y + help[1];
                if (new_x < 0 || new_y < 0 || new_x >= map[0].size() || new_y >= map.size())
                    continue;
                check_edges.push_back({new_x, new_y});
            }
            for (vector<int> tile : check_edges)
            {
                int new_x = tile[0];
                int new_y = tile[1];
                if (coin_flip() && !decided && twobytwo({new_x, new_y}) && current_tile(map[new_y][new_x]) == Tile::COUNT) { decided = true; assign(map[new_y][new_x], Tile::RIVER); out.push_back({new_x, new_y}); }
                else
                {
                    if (current_tile(map[new_y][new_x]) != Tile::RIVER)
                        off(map[new_y][new_x], Tile::RIVER);
                    if (diverge(0.02))
                        decided = false;
                }
                empty = empty && (current_tile(map[new_y][new_x]) == Tile::COUNT);
            }
            if (empty)
            {
                int new_y = check_edges.back()[1];
                int new_x = check_edges.back()[0];
                assign(map[y][x], Tile::RIVER);
                out.push_back({new_x, new_y});
            }
        }

        // Forest
    }
    return out;
}

int main()
{
    const int WIDTH = 20;
    const int HEIGHT = 15;
    vector<vector<int>> points= {
        {3, 3}, {16, 11},
        {3, 11}, {16, 3}
    };
    
    WaveFunctionCollapse wave(WIDTH, HEIGHT, points);

    int epoch = 0;
    vector<vector<int>> seed = {wave.get_rivec()};
    while (epoch < 10)
    {
        seed = wave.Evolve(seed);
        epoch++;
    }

    Canvas canvas(WIDTH, HEIGHT);
    const vector<vector<TileSet>>& map = wave.get_map();
    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            switch (wave.current_tile(map[y][x]))
            {
                case Tile::RIVER:    canvas.plot(x, y, 1); break;
                case Tile::MOUNTAIN: canvas.plot(x, y, 4); break;
                case Tile::FOREST:   canvas.plot(x, y, 6); break;
                case Tile::VILLAGE:  canvas.plot(x, y, 2); break;
                case Tile::SPAWN:    canvas.plot(x, y, 3); break;
                case Tile::COUNT:   break;
            }
        }
    }
    canvas.draw();
}
