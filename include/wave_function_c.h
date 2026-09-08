#pragma once
#include <vector>
#include <algorithm>
#include <iostream>
using namespace std;

enum class Tile
{
    RIVER,
    MOUNTAIN,
    FOREST,
    VILLAGE,
    SPAWN,
    COUNT
};

struct Tiles
{
    Tile tile;
    double prob;
};

struct TileSet
{
    vector<Tiles> space;
};


class WaveFunctionCollapse
{
private:
    vector<vector<TileSet>> map;
    vector<int> __init__(const vector<vector<int>>& spawnpoints, int heat_intensity);
    vector<int> rivec;
public:
    WaveFunctionCollapse(int WIDTH, int HEIGHT, vector<vector<int>>& spawn);
    vector<int> get_rivec();
    void EvolveTile(const vector<int>& coord);
    Tile current_tile(const TileSet& x);
    void assign(TileSet& x, Tile tgt);
    void off(TileSet& x, Tile tgt);
    bool twobytwo(vector<int> new_coord);
    bool detect_edge(const vector<int>& coord);
    bool check_river_edge(const vector<vector<int>>& edge_tile);
    const vector<vector<TileSet>>& get_map() const;
    vector<int> random_seed(int w, int h);
    vector<vector<int>> Evolve(vector<vector<int>>& seed);
};
