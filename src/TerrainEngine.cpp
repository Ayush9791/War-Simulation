#include <random>
#include <vector>
#include <algorithm>
#include "procedural_generation.h"
#include "plotter.h"

TerrainEngine::TerrainEngine(vector<vector<int>>& map, vector<vector<int>> guilds, vector<vector<int>> villages, int MOV) : assets(map)
{
    assets.board = board_init(guilds, villages, MOV);
};

TerrainEngine::TerrainEngine(vector<vector<int>>& map, int Nobj, int MOV) : assets(map)
{
    vector<vector<int>> random_n = random_n_coords(assets.seed, Nobj*2, MOV);
    if (random_n.empty()) { throw invalid_argument("Not enough space to plot coords!"); }
    int slicer = Nobj;
    vector<vector<int>> guilds(random_n.begin(), random_n.begin()+slicer);
    vector<vector<int>> players(random_n.begin()+slicer, random_n.end());
    if (guilds.size() == players.size())
	assets.board = board_init(guilds, players, MOV);
};

Assets& TerrainEngine::get_asset() { return assets; }

BoardState TerrainEngine::board_init(const vector<vector<int>>& guilds, const vector<vector<int>>& villages, int MOV)
{
    BoardState board;
    //villages.size() x villages.size()
    vector<vector<int>> vlg(villages.size(), vector<int>(villages.size(), 0));
    vector<vector<int>> ply(guilds.size(), vector<int>(guilds.size(), 0));
    vector<vector<int>> vxp(villages.size(), vector<int>(guilds.size(), 0));
    board.VxV = vlg;
    board.PxP = ply;
    board.VxP = vxp;
    board.village_locations = villages;
    board.player_locations = guilds;
    assets.seed.draw(villages, MOV);
    assets.seed.draw(guilds, MOV);
    assets.board = board;
    assets.board = EvalBoard(assets, assets.seed.get_cost_map(), MOV);
    return assets.board;
}     

BoardState TerrainEngine::EvalBoard(Assets& asset, vector<vector<int>>& cost_grid, int MOV)
{
    vector<vector<int>> v_locs = asset.board.village_locations;
    vector<vector<int>> p_locs = asset.board.player_locations;
    vector<vector<int>> path;
    vector<vector<double>> explored(cost_grid.size(), vector<double>(cost_grid[0].size(), 0.0));
    for (int v=0; v < asset.board.village_locations.size(); v++)
    {
	vector<int> vec = asset.board.village_locations[v];
	path = asset.seed.distance_field(vec, cost_grid, explored, MOV);
	for (int p=0; p < asset.board.player_locations.size(); p++)
	{
	    vector<int> vec = asset.board.player_locations[p];
	    asset.board.VxP[v][p] = path[vec[1]][vec[0]];
	}
    }

    for (int v1=0; v1 < asset.board.village_locations.size(); v1++)
    {
	vector<int> vec = asset.board.village_locations[v1];
	path = asset.seed.distance_field(vec, cost_grid, explored, MOV);
	for (int v2=0; v2 < asset.board.village_locations.size(); v2++)
	{
	    vector<int> vec = asset.board.village_locations[v2];
	    asset.board.VxV[v1][v2] = path[vec[1]][vec[0]];
	}
    }

    for (int p1=0; p1 < asset.board.player_locations.size(); p1++)
    {
	vector<int> vec = asset.board.player_locations[p1];
	path = asset.seed.distance_field(vec, cost_grid, explored, MOV);
	for (int p2=0; p2 < asset.board.player_locations.size(); p2++)
	{
	    vector<int> vec = asset.board.player_locations[p2];
	    asset.board.PxP[p1][p2] = path[vec[1]][vec[0]];
	}
    }
    return asset.board;
}

bool TerrainEngine::ValidateIsolation(const vector<vector<int>>& grid)
{
    bool flag = true;

    if (grid.empty())
        return false;
    
    for (int ply=0; ply < grid[0].size(); ply++)
    {
        for (int vil=0; vil < grid.size(); vil++) {
            if (grid[vil][ply] == -1)
                flag = false; return flag; }
    }
    return flag;
}

bool TerrainEngine::ValidateBalance(const vector<double>& scores)
{
    bool flag = true;
    if (scores.empty())
        return false;
    
    double tol = *min_element(scores.begin(), scores.end()) * 1.1;
    for (double i : scores)
        if (i > tol)
            return false;
    return flag;
}

vector<double> TerrainEngine::AnalyseBoard(BoardState& board, int MOV)
{
    vector<vector<int>> intmap = board.VxP;
    
    // Convert to MOV map
    for (int ply=0; ply < intmap[0].size(); ply++)
    {
        for (int vil=0; vil < intmap.size(); vil++)
        {
            if (intmap[vil][ply] != -1) { intmap[vil][ply] = ceil(static_cast<double>(intmap[vil][ply]) / MOV); }
        }
    }
    
    // Spread Map
    vector<vector<int>> spread_map;
    for (vector<int>& v : intmap)
    {
        vector<int> row;
        int mn = -69;
        for (int x=0; x < v.size(); x++)
        {
            if (v[x] < 0) { continue; }
            if (mn == -69) { mn = v[x]; }
            if (mn > v[x]) { mn = v[x]; }
        }
        for (int x : v) { if (x != -1) row.push_back(x - mn); else row.push_back(-1); }
        spread_map.push_back(row);
    }

    // Village Degrees
    vector<int> village_deg(spread_map.size(), 0);
    for (int v=0; v < spread_map.size(); v++)
    {
        for (int x=0; x < spread_map[v].size(); x++) {
            if (spread_map[v][x] == 0) { village_deg[v]++; } }
    }

    // Player Degree
    vector<int> player_deg(spread_map[0].size(), 0);
    for (int p=0; p < spread_map[0].size(); p++)
    {
        for (int x=0; x < spread_map[p].size(); x++) {
            if (spread_map[x][p] == 0) { player_deg[p]++; } }
    }

    // Need a formula to derive claim score inversely from cost [intmap]
    vector<vector<double>> claimap(spread_map.size(), vector<double>(spread_map[0].size(), 0));
    for (int ply=0; ply < claimap[0].size(); ply++)
    {
        for (int vil=0; vil < claimap.size(); vil++)
        {
            if (intmap[vil][ply] == 0) { continue ; }
            claimap[vil][ply] = 1.0 / intmap[vil][ply];
        }
    }
    
    // Player-wise imbalance scoring system : Higher score --> more advantage --> more imbalance
    vector<double> score(spread_map[0].size(), 0.0);
    for (int ply=0; ply < spread_map[0].size(); ply++)
    {
        for (int vil=0; vil < spread_map.size(); vil++)
        {
            if (spread_map[vil][ply] == 0)
            {       
                if (player_deg[ply] == 1 && village_deg[vil] > 1) {
                    score[ply] += claimap[vil][ply] / 2.0;
                }
                else if (player_deg[ply] > 1 && village_deg[vil] == 1) {
                    score[ply] += claimap[vil][ply] * 3.0;
                }
                else if ((player_deg[ply] > 1 && village_deg[vil] > 1) ||
                         (player_deg[ply] == 1 && village_deg[vil] == 1)) {
                    score[ply] += claimap[vil][ply] * 1.0;
                }
            }
        }
    }
    return score;
}

void TerrainEngine::Push(Assets& asset, int MOV)
{
    vector<vector<int>> v_locs = asset.board.village_locations;
    vector<vector<int>> p_locs = asset.board.player_locations;
    vector<vector<int>> cost_map = asset.seed.get_cost_map();
    vector<vector<int>> path;
    vector<vector<double>> explored(cost_map.size(), vector<double>(cost_map[0].size(), 0.0));
    for (int v=0; v < asset.board.village_locations.size(); v++)
    {
        vector<int> vec = asset.board.village_locations[v];
        path = asset.seed.distance_field(vec, cost_map, explored, MOV);
	for (int p=0; p < asset.board.player_locations.size(); p++)
	{
            vector<int> vec = asset.board.player_locations[p];
            asset.board.VxP[v][p] = path[vec[1]][vec[0]];
	}
    }

    for (int v1=0; v1 < asset.board.village_locations.size(); v1++)
    {
        vector<int> vec = asset.board.village_locations[v1];
        path = asset.seed.distance_field(vec, cost_map, explored, MOV);
	for (int v2=0; v2 < asset.board.village_locations.size(); v2++)
	{
            vector<int> vec = asset.board.village_locations[v2];
            asset.board.VxV[v1][v2] = path[vec[1]][vec[0]];
        }
    }

    for (int p1=0; p1 < asset.board.player_locations.size(); p1++)
    {
        vector<int> vec = asset.board.player_locations[p1];
        path = asset.seed.distance_field(vec, cost_map, explored, MOV);
	for (int p2=0; p2 < asset.board.player_locations.size(); p2++)
	{
            vector<int> vec = asset.board.player_locations[p2];
            path = asset.seed.distance_field(vec, cost_map, explored, MOV);
            asset.board.PxP[p1][p2] = path[vec[1]][vec[0]];
	}
    }
    assets.board = asset.board;
    assets.seed.get_cost_map() = cost_map;
    assets.seed.get_explored() = explored;
    assets.env = asset.env;
}

bool TerrainEngine::spaced(TerrainSeed& draft, const vector<vector<int>>& coords, const vector<int>& new_pt, int MIN_DIST, int MOV)
{
    bool okay = true;
    if (coords.empty()) { return false; }
    for (vector<int> pt : coords)
    {
        vector<vector<int>> dijkstra = draft.distance_field(pt, draft.get_cost_map(), draft.get_explored(), MOV);
	okay = (okay && dijkstra[new_pt[1]][new_pt[0]] > MIN_DIST+1);
    }
    return okay;
}

vector<vector<int>> TerrainEngine::random_n_coords(TerrainSeed& draft, int N, int MOV)
{
    int MIN_DIST = 6;
    static random_device rd;
    static mt19937 rng(rd()); 
    uniform_int_distribution<int> xdist(2, draft.get_width() - 3);
    uniform_int_distribution<int> ydist(2, draft.get_height() - 3);
    vector<vector<int>> coords;
    for (int pts=0; pts < N; pts++)
    {
	if (!coords.empty())
	{
	    bool okay = false;
            int tries = 0;
	    while (!okay && tries < 100)
	    {
		vector<int> new_pt = {xdist(rng), ydist(rng)};
		okay = spaced(draft, coords, new_pt, MIN_DIST, MOV);
		if (okay) { coords.push_back(new_pt); break;}
                tries++;
	    }
	    if (!okay) { return {}; }
	}
	else { coords.push_back({xdist(rng), ydist(rng)}); }
    }
    return coords;
}

vector<vector<int>> TerrainEngine::ValidRiverExtension(Assets& asset, int MOV)
{
    BoardState board = EvalBoard(asset, asset.seed.get_cost_map(), MOV);
    vector<vector<int>> village;
    vector<vector<int>> player;
    for (int vil1=0; vil1 < board.VxV.size(); vil1++)
    {
        bool all = true;
        for (int vil2=0; vil2 < board.VxV[vil1].size(); vil2++)
            if (vil1 != vil2) { all = all && (board.VxV[vil1][vil2] == -1); }
        
        for (int ply=0; ply < board.VxP[vil1].size(); ply++)
            all = all && (board.VxP[vil1][ply] == -1);
        
        if (!all) { village.push_back(board.village_locations[vil1]); }
    }
    
    for (int ply1 = 0; ply1 < board.PxP.size(); ply1++)
    {
        bool all = true;
        for (int ply2 = 0; ply2 < board.PxP[ply1].size(); ply2++)
            if (ply1 != ply2) { all = all && (board.PxP[ply1][ply2] == -1); }
        
        for (int vil = 0; vil < board.VxP.size(); vil++)
            all = all && (board.VxP[vil][ply1] == -1);

        if (!all) { player.push_back(board.player_locations[ply1]); }
    }
    
    village.insert(village.end(), player.begin(), player.end());
    return village;
}

Assets TerrainEngine::GenerateRiverSystem(Assets& asset, int MOV)
{
    // STEP 1: BUILD RIVERS
    bool done = false;
    int tries = 0;
    Assets out = asset;
    int x = asset.board.village_locations.size() + asset.board.player_locations.size();
    int a = x - 2;
    int b = floor( a / 2 );
    int branches = randbetween(b, a);
    Assets temp = asset;
    while (!done && tries < 100)
    {
        tries++;
        vector<vector<int>> locs = asset.board.village_locations;
        locs.insert(locs.end(), asset.board.player_locations.begin(), asset.board.player_locations.end());
        int N = locs.size();
        if (N <= 1) { return temp; }
        if (temp.env.river.empty())
        {
            Assets temp_temp = temp;
            vector<vector<int>> sim_cost_grid = temp_temp.seed.get_cost_map();
            int a = randbetween(0, N-1);
            int b = randbetween(0, N-1);
            while (a == b)
            {
                a = randbetween(0, N-1);
                b = randbetween(0, N-1);
            }
            temp_temp.env.river = temp_temp.seed.generate_river(locs[a], locs[b], MOV);
            for (vector<int> riv_coord : temp_temp.env.river) { sim_cost_grid[riv_coord[1]][riv_coord[0]] = MOV+1; }
            BoardState temp_board = EvalBoard(temp_temp, sim_cost_grid, MOV);
            BoardState old_board = EvalBoard(temp, temp.seed.get_cost_map(), MOV);
            bool valid_river = false;
            
            for (int x=0; x < temp_board.VxP.size(); x++)
                for (int y=0; y < temp_board.VxP[0].size(); y++)
                    if (temp_board.VxP[x][y] == -1 && old_board.VxP[x][y] != -1) { valid_river = true; break;}
            
            if (!temp_temp.env.river.empty() && valid_river)
            {
                sim_cost_grid = temp_temp.seed.get_cost_map();
                for (vector<int> riv_coord : temp_temp.env.river) { sim_cost_grid[riv_coord[1]][riv_coord[0]] = MOV+1; }
                temp_temp.board = EvalBoard(temp_temp, sim_cost_grid, MOV);
                temp_temp.seed.get_cost_map() = sim_cost_grid;
                temp = temp_temp;
            }
        }
        if (!temp.env.river.empty() && temp.env.river_extension.size() < branches)
        {
            vector<vector<vector<int>>> river_ext;
            Assets temp_temp = temp;
            vector<vector<int>> sim_cost_grid = temp_temp.seed.get_cost_map();
            vector<vector<int>> full_stream;
            full_stream.insert(full_stream.end(), temp.env.river.begin(), temp.env.river.end());
            for (vector<vector<int>>& exts : temp.env.river_extension)
            {
                full_stream.insert(full_stream.end(), exts.begin(), exts.end());
            }
            vector<vector<int>> valid_coords = ValidRiverExtension(temp, MOV);
            int M = valid_coords.size();
            if (M <= 1) { return temp; }
            int ext_tries = 0;
            while (river_ext.empty() && ext_tries < 10)
            {
                ext_tries++;
                int a = randbetween(0, M-1);
                int b = randbetween(0, M-1);
                while (a == b)
                {
                    a = randbetween(0, M-1);
                    b = randbetween(0, M-1);
                }
                river_ext = temp_temp.seed.mutate_river(valid_coords[a], valid_coords[b], full_stream, MOV);
            }
            if (!river_ext.empty())
            {
                bool valid_ext = false;
                BoardState new_board;
                BoardState old_board;
                vector<vector<int>> ext;
                for (int i=0; i < river_ext.size(); i++)
                {
                    ext = river_ext[i];
                    sim_cost_grid = temp_temp.seed.get_cost_map();
                    for (vector<int> riv_coord : ext) { sim_cost_grid[riv_coord[1]][riv_coord[0]] = MOV+1; }
                    new_board = EvalBoard(temp_temp, sim_cost_grid, MOV);
                    old_board = EvalBoard(temp, temp.seed.get_cost_map(), MOV);
                    for (int x=0; x < new_board.VxP.size(); x++)
                        for (int y=0; y < new_board.VxP[0].size(); y++)
                            if (new_board.VxP[x][y] == -1 && old_board.VxP[x][y] != -1) { valid_ext = true; break;}
                    if (valid_ext) { break; }
                }
                if (valid_ext)
                {
                    temp_temp.board = new_board;
                    temp_temp.seed.get_cost_map() = sim_cost_grid;
                    temp_temp.env.river_extension.push_back(ext);
                    temp = temp_temp;
                }
            }
        }
        done = !temp.env.river.empty() && temp.env.river_extension.size() >= branches;
        if (done) { out = temp; cout << "Tries: " << tries << '\n';}
        if (tries == 100) { out = temp; cout << "Tries exhausted!" << '\n'; }
    }
    return out;
}

Assets TerrainEngine::Generate(Assets& asset, int MOV)
{
    // STEP 1: BUILD RIVERS
    Assets out = GenerateRiverSystem(asset, MOV);

    // STEP 2: ELIMINATE -1s USING BRIDGING
    while(ValidateBoard(AnalyseBoard(out.board, MOV), out.board.VxP))
    {
        
    }

    // STEP 3: EQUALIZE SCORES USING FORESTS AND MOUNTAINS
}

int main()
{
    const int WIDTH = 50;
    const int HEIGHT = 50;
    const int MOV = 4;
    const int TEAMS = 10;
    
    vector<vector<int>> map(HEIGHT, vector<int>(WIDTH, 1));
    TerrainEngine engine(map, TEAMS, MOV);

    Assets temp = engine.Generate(engine.get_asset(), MOV);

    Canvas canvas(WIDTH, HEIGHT);
    canvas.plot(temp.env.river, 1);

    for (const vector<vector<int>>& extension : temp.env.river_extension)
        canvas.plot(extension, 1);

    canvas.plot(temp.board.village_locations, 2);
    canvas.plot(temp.board.player_locations, 3);
    canvas.draw();
}
