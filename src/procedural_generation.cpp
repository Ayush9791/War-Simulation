#include <random>
#include <vector>
#include <algorithm>
#include "raytracer.h"
#include "procedural_generation.h"
#include "general_pathtracing.h"
#include "plotter.h"
#include <iomanip>
#include <queue>
using namespace std;

const int TerrainSeed::calculate_cost(const vector<vector<int>>& path, vector<vector<int>>& in_comap)
{
    int total_cost=0;
    vector<vector<int>> actual_path = path;
    actual_path.erase(actual_path.begin());
    actual_path.pop_back();
    for (const vector<int>& coord : actual_path)
    {
	total_cost += in_comap[coord[1]][coord[0]];
    }
    return total_cost+1;
}

TerrainSeed::TerrainSeed(vector<vector<int>> map) :
    map(map), 
    cost_map(vector<vector<int>>(map.size(),vector<int>(map[0].size(), 1))),
    explored(vector<vector<double>>(map.size(), vector<double>(map[0].size(), 0.0))), WIDTH(map[0].size()), HEIGHT(map.size()) {};

vector<vector<int>>& TerrainSeed::get_map() { return map; }

vector<vector<int>>& TerrainSeed::get_cost_map() { return cost_map; }

vector<vector<double>>& TerrainSeed::get_explored() { return explored; }

int TerrainSeed::get_width() { return WIDTH; }

int TerrainSeed::get_height() { return HEIGHT; }

void TerrainSeed::draw(vector<vector<int>> coord, int COST)
{
    for (vector<int> loc : coord)
    {
	cost_map[loc[1]][loc[0]] = COST;
    }
    preheat();
}

bool TerrainSeed::check_diagonality(const vector<int>& a, const vector<int>& b)
{
    bool out = false;
    vector<vector<int>> helper = {{1,1}, {-1,-1}, {1,-1}, {-1, 1}};
    for (vector<int> help : helper)
    {
	if (a[0] + help[0] == b[0] && a[1] + help[1] == b[1]) { return true; }
    }
    return out;
}

void TerrainSeed::swap(vector<int>& out, const vector<int>& a, const vector<int>& b)
{
    vector<int> p1 = {a[0], b[1]};
    vector<int> p2 = {b[0], a[1]};
    (p1 == out) ? out=p2 : out=p1;
}

void TerrainSeed::smoothen(vector<vector<int>>& path)
{
    bool done = false;
    for (int i=2; i < path.size()-2; i++)
    {
	if (check_diagonality(path[i-2], path[i]) && check_diagonality(path[i], path[i+2]) && check_diagonality(path[i+1], path[i-1]))
	{
	    swap(path[i], path[i-1], path[i+1]);
	    done = true;
	}
    }
    if (done)
    {
	smoothen(path);
    }
}

vector<vector<int>> TerrainSeed::random_two_coords()
{
    static random_device rd;
    static mt19937 rng(rd());

    uniform_int_distribution<int> xdist(2, WIDTH - 3);
    uniform_int_distribution<int> ydist(2, HEIGHT - 3);

    vector<int> a = {xdist(rng), ydist(rng)};
    vector<int> b;

    do
    {
        b = {xdist(rng), ydist(rng)};
    }
    while (b == a);

    return {a, b};
}

int TerrainSeed::random_coord(bool row)
{
    vector<int> out;
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> width_sample(0, WIDTH-1);
    uniform_int_distribution<int> height_sample(0, HEIGHT-1);
    return (row) ? width_sample(rng) : height_sample(rng);
}

vector<int> TerrainSeed::random_edge()
{
    vector<int> out;
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> edge(0, 1);
    int edge_loc = edge(rng);
    int side_loc = edge(rng);
    uniform_int_distribution<int> width_sample(0, WIDTH-1);
    uniform_int_distribution<int> height_sample(0, HEIGHT-1);
    if (edge_loc == 0)
    {
	return {(WIDTH-1)*side_loc, height_sample(rng)};
    }
    else
    {
	return {width_sample(rng), (HEIGHT-1)*side_loc};
    }
}

double TerrainSeed::cart_dist(vector<int> target, vector<int> inp)
{
    int x = inp[0];
    int y = inp[1];
    int targ_x = target[0];
    int targ_y = target[1];
    int x_comp = targ_x - x;
    int y_comp = targ_y - y;

    return sqrt((x_comp * x_comp) + (y_comp * y_comp));
}

vector<vector<int>> TerrainSeed::find_perps(const vector<int>& h1, const vector<int>& h2, int offset)
{
    if (h1 == h2) { return {}; }
    int x1 = h1[0];
    int y1 = h1[1];
    int x2 = h2[0];
    int y2 = h2[1];
    int center_x = (x1 + x2) / 2;
    int center_y = (y1 + y2) / 2;
    int dx = x2 - x1;
    int dy = y2 - y1;
    double len = sqrt(dx * dx + dy * dy);
    int px = round((-dy / len) * offset);
    int py = round(( dx / len) * offset);
    if ((px > WIDTH) || (py > HEIGHT))
    {
	px = round((-dy / len));
	py = round(( dx / len));
    }
    if ((px > WIDTH) || (py > HEIGHT)) { return {}; }
    int up_x = center_x + px;
    int up_y = center_y + py;
    int down_x = center_x - px;
    int down_y = center_y - py;
    if (up_x < 0 || up_y < 0 || down_x < 0 || down_y < 0 || up_x >= map[0].size() || down_y >= map.size() || down_x >= map[0].size() || up_y >= map.size()) { return {}; }
    return {{center_x + px, center_y + py}, {center_x - px, center_y - py}, {center_x, center_y}};
}

vector<int> TerrainSeed::find_closest_edge(vector<int> coord)
{
    static random_device rd;
    static mt19937 rng(rd());
    static uniform_int_distribution dist(-1, 1);
    int aesthetic_offset = 3;
    int dx = WIDTH - coord[0] - 1;
    int dy = HEIGHT - coord[1] - 1;
    int ddx = coord[0];
    int ddy = coord[1];
    vector<int> vec = {dx, dy, ddx, ddy};
    int minm = min_element(vec.begin(), vec.end()) - vec.begin();
    int x = (minm == 0) ? WIDTH-1 : (minm == 2) ? 0 : clamp(ddx + dist(rng)*aesthetic_offset, 0, WIDTH-1);
    int y = (minm == 1) ? HEIGHT-1 : (minm == 3) ? 0 : clamp(ddy + dist(rng)*aesthetic_offset, 0, HEIGHT-1);
    return {x, y};
}

vector<vector<int>> TerrainSeed::find_separation(vector<int> x, const vector<int>& h1, const vector<int>& h2, int perp_dist)
{
    vector<vector<int>> coords  = find_perps(h1, h2, perp_dist);
    if (coords.empty()) { return {}; }
    bool farthest = cart_dist(x, coords[1]) > (cart_dist(x, coords[0]));
    vector<int> y = (farthest) ? find_closest_edge(coords[1]) : find_closest_edge(coords[0]);
    return (farthest) ? vector<vector<int>> {x, coords[0], coords[2], coords[1], y} : vector<vector<int>> {x, coords[1], coords[2], coords[0], y};
}

void TerrainSeed::optimize(vector<int> current_coord, const vector<vector<int>>& path, vector<vector<int>>& cost_grid, int expected_cost, int path_idx)
{
    for (int action_idx=0; action_idx < helper.size(); action_idx++)
    {
	vector<int> action = helper[action_idx];
	int next_x = current_coord[0] + action[0];
	int next_y = current_coord[1] + action[1];
	if (path_idx != path.size()-1)
	{
	    if (vector<int>{next_x, next_y} == path[path_idx+1])
	    {
		int new_expected_cost = expected_cost + 1;
		if (cost_grid[next_y][next_x] == 0 || new_expected_cost < cost_grid[next_y][next_x])
		{
		    cost_grid[next_y][next_x] = new_expected_cost;
		    optimize({next_x, next_y}, path, cost_grid, new_expected_cost, path_idx+1);
		}
		else { continue; }
	    }
	}
    }
}

vector<vector<int>> TerrainSeed::render_river(vector<int> start, vector<vector<int>>& cost_grid, vector<vector<int>>& out)
{
    for (int action_idx=0; action_idx < helper.size(); action_idx++)
    {
	vector<int> action = helper[action_idx];
	int next_x = start[0] + action[0];
	int next_y = start[1] + action[1];
	if (next_x < 0 || next_y < 0 || next_x >= map[0].size() || next_y >= map.size()) { continue; }
	    if (cost_grid[next_y][next_x] == cost_grid[start[1]][start[0]] + 1)
	    {
		out.push_back({next_x, next_y});
		render_river({next_x, next_y}, cost_grid, out);
                break;
	    }
    }
    return out;
}

void TerrainSeed::optimize(vector<vector<int>>& path)
{
    vector<vector<int>> cost_grid(map.size(), vector<int>(map[0].size(), 0));
    vector<int>  current_coord = path[0];
    optimize(current_coord, path, cost_grid, 0, 0);
    vector<vector<int>> out = { current_coord };
    path = render_river(current_coord, cost_grid, out);
}

void TerrainSeed::preheat()
{
    explored.assign(cost_map.size(), vector<double>(cost_map[0].size(), 0.0));
    vector<vector<int>> helper_ = {
	{-1, -1}, {0, -1}, {1, -1},
	{-1,  0},          {1,  0},
	{-1,  1}, {0,  1}, {1,  1}
    };
    for (int i=0; i < explored.size(); i++)
    {
	for (int j=0; j < cost_map[0].size(); j++)
	{
	    if (cost_map[i][j] > 1)
	    {
		for (vector<int> delta : helper_)
		{
		    int new_x = j+delta[0];
		    int new_y = i+delta[1];
		    if (new_x < 0 || new_y < 0 || new_x >= cost_map[0].size() || new_y >= cost_map.size()) { continue; }
		    explored[new_y][new_x] = cost_map[i][j];
		}
	    }
	}
    }
}

vector<vector<int>> TerrainSeed::grow(vector<vector<int>> inp, int MOV, bool& success)
{
    vector<vector<int>> out;
    vector<vector<int>> temp = cost_map;
    int len = inp.size();
    for (int k=0, l=1; k < len-1 && l < len; k++, l++)
    {
	vector<vector<int>> outp = recurse(inp[k], inp[l], temp, MOV, explored);
	if (outp.empty())
	{
	    success = false;
	    return {};
	}
        for (vector<int> x : outp)
        {
            temp[x[1]][x[0]] = MOV+1;
        }
	outp.pop_back();
	out.insert(out.end(), outp.begin(), outp.end());
    }
    out.push_back(inp[len-1]);
    bool two_by_two = false;
    for (vector<int> river_tile : out)
    {
        if (twobytwo(river_tile, out)) { continue; }
        else { two_by_two = true; }
    }
    if (two_by_two) { success = false; return {}; }
    success = true;
    return out;
}

vector<vector<int>> TerrainSeed::generate_river(const vector<int>& h1, const vector<int>& h2, int MOV)
{
    bool success = false;
    int WIDTH = map[0].size();
    int HEIGHT = map.size();
    vector<vector<int>> path;
    int retries = 0;
    while (!success && retries < 10)
    {
	vector<int> x = random_edge();
	vector<vector<int>> find_set = find_separation(x, h1, h2, 3);
        if (!find_set.empty()) { path = grow(find_set, MOV, success); }
	if (!path.empty()) { optimize(path); }
        retries++;
    }
    return (!path.empty()) ? path : vector<vector<int>>{};
}

vector<vector<int>> TerrainSeed::generate_river(vector<int> x, const vector<int>& h1, const vector<int>& h2, int MOV, int perp_dist)
{
    bool success = false;
    int WIDTH = map[0].size();
    int HEIGHT = map.size();
    vector<vector<int>> path;
    int retries = 0;
    while (!success && retries < 10)
    {
	vector<vector<int>> find_set = find_separation(x, h1, h2, perp_dist);
        if (!find_set.empty()) { path = grow(find_set, MOV, success); }
	if (!path.empty()) { optimize(path); }
	retries++;
    }
    return (!path.empty()) ? path : vector<vector<int>>{};
}

bool if_in_(vector<vector<int>> in, vector<int> lookup)
{
    for (vector<int> i : in)
    {
        if (i == lookup)
        {
            return true;
        }
    }
    return false;
}

vector<vector<vector<int>>> _helper_ {{{-1, -1}, {0, -1}, {-1,  0}}, {{0, -1}, {1, -1}, {1,  0}}, {{-1,  0}, {-1,  1}, {0,  1}}, {{1,  0} ,{0,  1}, {1,  1}}};

bool twobytwo(vector<int> new_coord, vector<vector<int>> river)
{
    bool final_valid = true;
    int new_x = new_coord[0];
    int new_y = new_coord[1];
    for (vector<vector<int>> coords : _helper_)
    {
	bool valid = false;
	for (vector<int> coord : coords)
	{
	    if (!if_in_(river, {new_x + coord[0], new_y + coord[1]})) { valid = true; }
	    if (valid) { break; }
	}
	final_valid = final_valid && valid;
    }
    return final_valid;
}

vector<int> get_outgrowth(vector<vector<int>> prp, vector<int> tgt)
{
    vector<int> out = prp.back();
    vector<int> prev = prp[prp.size() - 2];

    int dx = out[0] - prev[0];
    int dy = out[1] - prev[1];

    int px = (-dy > 0) - (-dy < 0);
    int py = ( dx > 0) - ( dx < 0);

    int tx = tgt[0] - out[0];
    int ty = tgt[1] - out[1];

    if (px * tx + py * ty < 0)
    {
        px = -px;
        py = -py;
    }

    return {out[0] + px, out[1] + py};
}

vector<vector<vector<int>>> TerrainSeed::mutate_river(const vector<int>& h1, const vector<int>& h2, vector<vector<int>> river, int MOV)
{
    // validate perp
    vector<int> perp = find_perps(h1, h2, 0).back();
    vector<vector<int>> candidates;
    int n = ceil((river.size() * 0.1));
    vector<vector<vector<int>>> final_candidates;
    vector<vector<int>> closest_tile = river;
    vector<int> dists;
    for (vector<int> x : river) { dists.push_back(cart_dist(x, perp)); }
    sort(closest_tile.begin(), closest_tile.end(),
         [&](const vector<int>& a, const vector<int>& b)
             {
                 return cart_dist(a, perp) < cart_dist(b, perp);
             }
        );
    closest_tile = vector<vector<int>>(closest_tile.begin(), closest_tile.begin()+n);
    for (vector<int> river_coord : closest_tile)
    {
        vector<vector<int>> possible_orientation;
        for (vector<int> help : helper)
	{
	    int new_x = river_coord[0] + help[0];
	    int new_y = river_coord[1] + help[1];
            bool final_valid = twobytwo({new_x, new_y}, river);
            if (new_x < 0 || new_y < 0 || new_x >= map[0].size() || new_y >= map.size() || if_in_(river, {new_x, new_y})) { continue; }
            if (final_valid) possible_orientation.push_back({new_x, new_y});
        }
        sort(possible_orientation.begin(), possible_orientation.end(),
         [&](const vector<int>& a, const vector<int>& b)
             {
                 return cart_dist(a, perp) < cart_dist(b, perp);
             }
            );
        if (!possible_orientation.empty()) { candidates.push_back(possible_orientation[0]); }
    }
    
    for (int i=0; i < candidates.size(); i++)
    {
	{
	    bool two_by_two = false;
	    bool in_same_river = false;
	    vector<int> candidate_coord = candidates[i];
	    vector<vector<int>> river_output = generate_river(candidate_coord, h1, h2, MOV, 3);
	    if (river_output.empty()) { continue; }
	    vector<vector<int>> full_stream = river;
	    full_stream.insert(full_stream.end(), river_output.begin(), river_output.end());
	    for (vector<int> river_tile : river_output)
	    {
		if (if_in_(river, river_tile)) { in_same_river = true; }
		if (twobytwo(river_tile, full_stream)) { continue; }
		else { two_by_two = true; }
	    }
	    if (!two_by_two && !in_same_river) { final_candidates.push_back(river_output); }
	}
    }
    return final_candidates;
}


bool TerrainSeed::bridgable(vector<vector<int>> validation_pair, vector<int> river_tile, vector<vector<int>> mainstream)
{
    bool out = true;
    for (vector<int> val_coord : validation_pair)
    {
	int new_x = river_tile[0] + val_coord[0];
	int new_y = river_tile[1] + val_coord[1];
	out = (out && if_in_(mainstream, {new_x, new_y}));
    }
    return out;
}

vector<vector<int>> TerrainSeed::bridgable(vector<vector<int>> river)
{
    vector<vector<int>> output;
    vector<vector<vector<int>>> helper = {{{0, -1}, {-1,  0}}, {{1,  0}, {0,  1}},  {{-1,  0}, {0,  1}}, {{0, -1}, {1,  0}}};
    for (vector<int> river_tiles : river)
    {
	bool out = false;
	for (vector<vector<int>> help : helper)
	{
	    out = (out || bridgable(help, river_tiles, river));
	}
	if (!out)
	{
	    output.push_back(river_tiles);
	}
    }
    return output;
}

vector<vector<int>> TerrainSeed::get_path(vector<int> current, vector<int> target, int MOV)
{
    return recurse(current, target, cost_map, MOV, explored);
}

TerrainEngine::TerrainEngine(vector<vector<int>>& map, vector<vector<int>> guilds, vector<vector<int>> villages, int MOV) : seed(map), board(board_init(guilds, villages, MOV)) {};

TerrainEngine::TerrainEngine(vector<vector<int>>& map, int Nobj, int MINDIST, int MOV) : seed(map)
{
    vector<vector<int>> random_n = random_n_coords(seed, Nobj, MINDIST, MOV);
    if (random_n.empty()) { throw invalid_argument("Not enough space to plot coords!"); }
    int slicer = Nobj;
    vector<vector<int>> guilds(random_n.begin(), random_n.begin()+slicer);
    vector<vector<int>> players(random_n.begin()+slicer, random_n.end());
    if (guilds.size() == players.size())
    {
	board = board_init(guilds, players, MOV);
    }
};

BoardState& TerrainEngine::get_board() { return board; }

TerrainSeed& TerrainEngine::get_seed() { return seed; }

BoardState TerrainEngine::board_init(const vector<vector<int>>& guilds, const vector<vector<int>>& villages, int MOV)
{
    BoardState board;
    //villages.size() x villages.size()
    vector<vector<PieceData>> vlg(villages.size(), vector<PieceData>(villages.size(), {0, 0}));
    vector<vector<PieceData>> ply(guilds.size(), vector<PieceData>(guilds.size(), {0, 0}));
    vector<vector<PieceData>> vxp(villages.size(), vector<PieceData>(guilds.size(), {0, 0}));
    board.village = vlg;
    board.player = ply;
    board.VxP = vxp;
    board.village_locations = villages;
    board.player_locations = guilds;
    seed.draw(villages, MOV);
    seed.draw(guilds, MOV);
    board = simulate(seed, board, seed.get_cost_map(), MOV);
    return board;
}    

void TerrainEngine::update(TerrainSeed& draft, BoardState& new_board, int MOV)
{
    vector<vector<int>> v_locs = new_board.village_locations;
    vector<vector<int>> p_locs = new_board.player_locations;
    vector<vector<int>> cost_map = draft.get_cost_map();
    vector<vector<double>> explored(cost_map.size(), vector<double>(cost_map[0].size(), 0.0));
    for (int v=0; v < new_board.village_locations.size(); v++)
    {
	for (int p=0; p < new_board.player_locations.size(); p++)
	{
	vector<vector<int>> path = recurse(v_locs[v], p_locs[p], cost_map, MOV, explored);
	    if (path.empty()) { new_board.VxP[v][p] = {0, 0}; }
	    else
	    {
		int path_cost = draft.calculate_cost(path, cost_map);
		int moves = ceil(static_cast<double>(path_cost) / static_cast<double>(MOV));
		new_board.VxP[v][p] = {moves, path_cost}; 
	    }
	}
    }

    for (int v1=0; v1 < new_board.village_locations.size(); v1++)
    {
	for (int v2=0; v2 < new_board.village_locations.size(); v2++)
	{
	    if (v1 == v2) { new_board.village[v1][v2] = {0, 0}; continue; }
	vector<vector<int>> path = recurse(v_locs[v1], v_locs[v2], cost_map, MOV, explored);
	    if (path.empty()) { new_board.village[v1][v2] = {0, 0}; }
	    else
	    {
		int path_cost = draft.calculate_cost(path, cost_map);
		int moves = ceil(static_cast<double>(path_cost) / static_cast<double>(MOV));
		new_board.village[v1][v2] = {moves, path_cost};
	    }
	}
    }

    for (int p1=0; p1 < new_board.player_locations.size(); p1++)
    {
	for (int p2=0; p2 < new_board.player_locations.size(); p2++)
	{
	    if (p1 == p2) { new_board.player[p1][p2] = {0, 0}; continue; }
	vector<vector<int>> path = recurse(p_locs[p1], p_locs[p2], cost_map, MOV, explored);
	    if (path.empty()) { new_board.player[p1][p2] = {0, 0}; }
	    else
	    {
		int path_cost = draft.calculate_cost(path, cost_map);
		int moves = ceil(static_cast<double>(path_cost) / static_cast<double>(MOV));
		new_board.player[p1][p2] = {moves, path_cost};
	    }
	}
    }
    board = new_board;
    seed.get_cost_map() = cost_map;
    seed.get_explored() = explored;
}

bool TerrainEngine::spaced(TerrainSeed& draft, const vector<vector<int>>& coords, const vector<int>& new_pt, int MIN_DIST, int MOV)
{
    bool okay = true;
    if (coords.empty()) { return false; }
    for (vector<int> pt : coords)
    {
	okay = (okay && draft.get_path(pt, new_pt, MOV).size() > MIN_DIST+1);
    }
    return okay;
}

vector<vector<int>> TerrainEngine::random_n_coords(TerrainSeed& draft, int teams, int MIN_DIST, int MOV)
{
    int N = teams * 2;
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
	    while (!okay && tries < 10)
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

void print_matrix(const string& name, const vector<vector<PieceData>>& matrix)
{
    cout << '\n' << name << '\n';

    for (const vector<PieceData>& row : matrix)
    {
        for (const PieceData& cell : row)
        {
            cout << '['
                 << setw(2) << cell.moves
                 << ", "
                 << setw(2) << cell.cost
                 << "] ";
        }
        cout << '\n';
    }
}

void print_board_state(const BoardState& board)
{
    print_matrix("Village -> Village  [turns, cost]", board.village);
    print_matrix("Player -> Player    [turns, cost]", board.player);
    print_matrix("Village -> Player   [turns, cost]", board.VxP);
}

BinaryStateSpace TerrainEngine::EvalBinaryState(BoardState& board)
{
    const vector<vector<PieceData>>& vxp = board.VxP;
    if (vxp.empty())
    {
	return NOTA;
    }
    vector<vector<int>> bingrid(vxp.size(), vector<int>(vxp[0].size(), 0));
    for (int i=0; i < vxp.size(); i++)
    {
	for (int j=0; j < vxp[0].size(); j++)
	{
	    if (vxp[i][j].cost == 0 && vxp[i][j].moves == 0) { bingrid[i][j] = 0; }
	    else { bingrid[i][j] = 1; }
	}
    }

    int P = vxp[0].size();
    int V = vxp.size();
    vector<int> village_degree(V, 0);
    vector<int> player_degree(P, 0);
    vector<vector<int>> contenders(V);

    for (int player = 0; player < P; player++)
    {
	for (int village = 0; village < V; village++)
	{
            if (!bingrid[village][player]) { continue; }
            player_degree[player]++;
            village_degree[village]++;
            contenders[village].push_back(player);
	}
    }
    
    // ClearOwn
    if (check(every, village_degree, [](int a){ return (a == 1);}) && check(every, player_degree, [](int a){ return (a == 1);})) { return ClearOwn; }

    // Unfair
    if (check(every, village_degree, [](int a){ return (a == 1);}) && check(any, player_degree, [](int a){ return (a != 1);})) { return Unfair; }
    
    // FullyIsolated
    if (check(every, village_degree, [](int a){ return (a == 0);})) { return FullyIsolated; }
    
    // PartIsolated
    if (check(any, village_degree, [](int a){ return (a == 0);})) { return PartIsolated; }

    // Ownership Balance
    if (check_equality(player_degree)) { return Balanced; }
    else { return Unbalanced; }
    return NOTA;
}

BoardState TerrainEngine::simulate(TerrainSeed& draft, BoardState board, vector<vector<int>>& cost_grid, int MOV)
{
    vector<vector<int>> v_locs = board.village_locations;
    vector<vector<int>> p_locs = board.player_locations;
    vector<vector<double>> explored(cost_grid.size(), vector<double>(cost_grid[0].size(), 0.0));
    for (int v=0; v < board.village_locations.size(); v++)
    {
	for (int p=0; p < board.player_locations.size(); p++)
	{
	vector<vector<int>> path = recurse(v_locs[v], p_locs[p], cost_grid, MOV, explored);
	    if (path.empty()) { board.VxP[v][p] = {0, 0}; }
	    else
	    {
		int path_cost = draft.calculate_cost(path, cost_grid);
		int moves = ceil(static_cast<double>(path_cost) / static_cast<double>(MOV));
		board.VxP[v][p] = {moves, path_cost}; 
	    }
	}
    }

    for (int v1=0; v1 < board.village_locations.size(); v1++)
    {
	for (int v2=0; v2 < board.village_locations.size(); v2++)
	{
	    if (v1 == v2) { board.village[v1][v2] = {0, 0}; continue; }
	vector<vector<int>> path = recurse(v_locs[v1], v_locs[v2], cost_grid, MOV, explored);
	    if (path.empty()) { board.village[v1][v2] = {0, 0}; }
	    else
	    {
		int path_cost = draft.calculate_cost(path, cost_grid);
		int moves = ceil(static_cast<double>(path_cost) / static_cast<double>(MOV));
		board.village[v1][v2] = {moves, path_cost};
	    }
	}
    }

    for (int p1=0; p1 < board.player_locations.size(); p1++)
    {
	for (int p2=0; p2 < board.player_locations.size(); p2++)
	{
	    if (p1 == p2) { board.player[p1][p2] = {0, 0}; continue; }
	vector<vector<int>> path = recurse(p_locs[p1], p_locs[p2], cost_grid, MOV, explored);
	    if (path.empty()) { board.player[p1][p2] = {0, 0}; }
	    else
	    {
		int path_cost = draft.calculate_cost(path, cost_grid);
		int moves = ceil(static_cast<double>(path_cost) / static_cast<double>(MOV));
		board.player[p1][p2] = {moves, path_cost};
	    }
	}
    }
    return board;
}

BoardState TerrainEngine::simulate_init(TerrainSeed& draft, const vector<vector<int>>& guilds, const vector<vector<int>>& villages, int MOV)
{
    BoardState board;
    //villages.size() x villages.size()
    vector<vector<PieceData>> vlg(villages.size(), vector<PieceData>(villages.size(), {0, 0}));
    vector<vector<PieceData>> ply(guilds.size(), vector<PieceData>(guilds.size(), {0, 0}));
    vector<vector<PieceData>> vxp(villages.size(), vector<PieceData>(guilds.size(), {0, 0}));
    board.village = vlg;
    board.player = ply;
    board.VxP = vxp;
    board.village_locations = villages;
    board.player_locations = guilds;
    draft.draw(villages, MOV);
    draft.draw(guilds, MOV);
    board = simulate(draft, board, draft.get_cost_map(), MOV);
    return board;
}  

BoardState TerrainEngine::simulate_init(TerrainSeed& draft, int Nobj, int MINDIST, int MOV)
{
    BoardState sim_board;
    vector<vector<int>> random_n = random_n_coords(draft, Nobj, MINDIST, MOV);
    if (random_n.empty())
    {
        return {};
    }
    int slicer = Nobj;
    vector<vector<int>> villages(random_n.begin(), random_n.begin()+slicer);
    vector<vector<int>> players(random_n.begin()+slicer, random_n.end());
    if (villages.size() == players.size())
    {
	sim_board = simulate_init(draft, players, villages, MOV);
    }
    return sim_board;
}

int TerrainEngine::ActionRNG(int count)
{
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution dist(0, count-1);
    return dist(rng);
}

Action TerrainEngine::Sample_Action(BoardState& board, const TerrainSeed& draft, Habitat& hab)
{
    if (board.player_locations.size() + board.village_locations.size() < 3) { return {};}
    ActionSpace action = static_cast<ActionSpace>(ActionRNG(static_cast<int>(ActionSpace::COUNT)));;
    if (hab.river.empty())
    {
        while (action == ActionSpace::MutateRiver)
        {
            action = static_cast<ActionSpace>(ActionRNG(static_cast<int>(ActionSpace::COUNT)));
        }
    }
    else
        while (action == ActionSpace::GenerateRiver)
        {
            action = static_cast<ActionSpace>(ActionRNG(static_cast<int>(ActionSpace::COUNT)));
        }
    PairKind type = static_cast<PairKind>(ActionRNG(static_cast<int>(PairKind::COUNT)));
    vector<int> P1;
    vector<int> P2;
    switch(type)
    {
    case PairKind::PP:
    {
        P1 = board.player_locations[ActionRNG(board.player_locations.size())];
        do {  P2 = board.player_locations[ActionRNG(board.player_locations.size())]; }
        while (P1 == P2);
        break;
    }
    case PairKind::PV:
    {
        P1 = board.player_locations[ActionRNG(board.player_locations.size())];
        do { P2 = board.village_locations[ActionRNG(board.village_locations.size())]; } 
        while (P1 == P2);
        break;
    }
    case PairKind::VV:
    {
        P1 = board.village_locations[ActionRNG(board.village_locations.size())];
        do { P2 = board.village_locations[ActionRNG(board.village_locations.size())]; }
        while (P1 == P2);
        break;
    }
    case PairKind::COUNT:
    {
        break;
    }}
    return {action, type, {P1, P2}};
}

int TerrainEngine::choose_extension_randomly(const vector<vector<vector<int>>>& ext)
{
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution dist(0, static_cast<int>(ext.size())-1);
    return dist(rng);
}

Observation TerrainEngine::Perform_Action(Habitat& mainenv, Action& action, BoardState& board, TerrainSeed& draft, int MOV)
{
    BoardState old_board = board;
    TerrainSeed old_seed = draft;
    Observation out;
    switch (action.action)
    {
    case ActionSpace::GenerateRiver:
    {
        if (action.args.size() != 2) { break; }
        if (!mainenv.river.empty())
        {
            out.previous_state = EvalBinaryState(old_board);
            out.next_state = EvalBinaryState(board);
            out.type = action.type;
            out.count = 1;
            out.action = action.action;
            break;
        }
        Habitat env;
        vector<vector<int>> sim_cost_grid;
        BoardState new_board;
        env.river = draft.generate_river(action.args[0], action.args[1], MOV);
        if (!env.river.empty())
        {
            sim_cost_grid = draft.get_cost_map();
            for (vector<int> riv_coord : env.river) { sim_cost_grid[riv_coord[1]][riv_coord[0]] = MOV+1; }
            new_board = simulate(draft, board, sim_cost_grid, MOV);
            if (EvalBinaryState(new_board) != EvalBinaryState(old_board))
            {
                mainenv = env;
                draft.get_cost_map() = sim_cost_grid;
                board = new_board;
            }
            out.previous_state = EvalBinaryState(old_board);
            out.next_state = EvalBinaryState(board);
            out.type = action.type;
            out.count = 1;
            out.action = action.action;
        }
        else
        {
            out.previous_state = EvalBinaryState(old_board);
            out.next_state = EvalBinaryState(board);
            out.type = action.type;
            out.count = 1;
            out.action = action.action;
        }
        break;
    }
    case ActionSpace::MutateRiver:        // To do: mutate river picker which filters out those which do not have any river around n tile radius
    {
        if (action.args.size() != 2) { break; }
        if (mainenv.river.empty())
        {
            out.previous_state = EvalBinaryState(old_board);
            out.next_state = EvalBinaryState(board);
            out.type = action.type;
            out.count = 1;
            out.action = action.action;
            break;
        }
        vector<vector<vector<int>>> river_ext;
        vector<vector<int>> sim_cost_grid;
        BoardState new_board;
        vector<vector<int>> full_stream;
        full_stream.insert(full_stream.end(), mainenv.river.begin(), mainenv.river.end());
        for (vector<vector<int>>& exts : mainenv.river_extension)
        {
            full_stream.insert(full_stream.end(), exts.begin(), exts.end());
        }
        river_ext = draft.mutate_river(action.args[0], action.args[1], full_stream, MOV);
        bool accepted = false;
        if (!river_ext.empty())
        {
            for (vector<vector<int>> ext : river_ext)
            {
                sim_cost_grid = draft.get_cost_map();
                for (vector<int> riv_coord : ext) { sim_cost_grid[riv_coord[1]][riv_coord[0]] = MOV+1; }
                new_board = simulate(draft, board, sim_cost_grid, MOV);
                if (EvalBinaryState(new_board) != EvalBinaryState(old_board))
                {
                    mainenv.river_extension.push_back(ext);
                    accepted = true;
                    break;
                }
            }
            if (!accepted)
            {
                vector<vector<int>> ext = river_ext[choose_extension_randomly(river_ext)];
                sim_cost_grid = draft.get_cost_map();
                for (vector<int> riv_coord : ext) { sim_cost_grid[riv_coord[1]][riv_coord[0]] = MOV+1; }
                new_board = simulate(draft, board, sim_cost_grid, MOV);
                mainenv.river_extension.push_back(ext);
            }
        }
        if (!river_ext.empty())
        {
            draft.get_cost_map() = sim_cost_grid;
            board = new_board;
            out.previous_state = EvalBinaryState(old_board);
            out.next_state = EvalBinaryState(board);
            out.type = action.type;
            out.count = 1;
            out.action = action.action;
        }
        else
        {
            out.previous_state = EvalBinaryState(old_board);
            out.next_state = EvalBinaryState(board);
            out.type = action.type;
            out.count = 1;
            out.action = action.action;
        }
        break;
    }

    case ActionSpace::BuildBridge:
    {
        vector<vector<int>> full_stream;
        full_stream.insert(full_stream.end(), mainenv.river.begin(), mainenv.river.end());
        for (vector<vector<int>> river : mainenv.river_extension)
        {
            full_stream.insert(full_stream.end(), river.begin(), river.end());
        }
        vector<vector<int>> bridges = draft.bridgable(full_stream);
        for (vector<int> bridge : bridges)
        {
            vector<vector<int>> dist_map = distance_field(bridge, draft.get_cost_map(), MOV);
            
        }
    }
    
    case ActionSpace::COUNT:
    {
        break;
    }
    }
    return out;
}

PositionBalance TerrainEngine::EvalPosition(int MOV, TerrainSeed& draft, BoardState& board)
{
    int PLAYER = board.player_locations.size();
    int VILLAGE = board.village_locations.size();
    PositionBalance out;
    out.position = vector<vector<vector<int>>>(VILLAGE, vector<vector<int>>(PLAYER, vector<int>(PLAYER, 0)));
    out.combat = vector<vector<int>>(PLAYER, vector<int>(PLAYER, 0));
    vector<vector<int>> village_costs_per_player(PLAYER, vector<int>(VILLAGE, 0));
    for (int ply=0; ply < PLAYER; ply++)
    {
        vector<vector<int>> position = distance_field(board.player_locations[ply], draft.get_cost_map(), MOV);
        for (int vil=0; vil < VILLAGE; vil++)
        {
            vector<int> loc = board.village_locations[vil];
            village_costs_per_player[ply][vil] = position[loc[1]][loc[0]];
        }
        for (int ply2=ply+1; ply2 < PLAYER-1; ply2++)
        {
            vector<int> loc = board.player_locations[ply2];
            out.combat[ply][ply2] = position[loc[1]][loc[0]];
        }
    }
    for (int vil=0; vil < VILLAGE; vil++)
    {
        for (int ply1=0; ply1 < PLAYER-1; ply1++)
        {
            for (int ply2=ply1+1; ply2 < PLAYER; ply2++)
            {
                out.position[vil][ply1][ply2] = abs(village_costs_per_player[ply1][vil] - village_costs_per_player[ply2][vil]);
            }
        }
    }
    return out;
}

void TerrainEngine::simulate(vector<Observation>& episode, vector<Observation>& obs_directory, Habitat& mainenv, TerrainSeed& draft, int Nobj, int MINDIST, int MOV, BinaryStateSpace& desired_state, int global_tries, bool& status)
{
    vector<Observation> old_episode = episode;
    int max_tries = 10;
    int tries = 0;
    TerrainSeed oldseed = draft;
    Habitat oldenv = mainenv;
    BoardState sim_board = simulate_init(draft, Nobj, MINDIST, MOV);
    if (sim_board.VxP.empty()) { return; }
    BinaryStateSpace state = EvalBinaryState(sim_board);
    PositionBalance balanced;
    while(state != desired_state && tries <= max_tries)
    {
        Action action = Sample_Action(sim_board, draft, mainenv);
	Observation obs = Perform_Action(mainenv, action, sim_board, draft, MOV);
        state = EvalBinaryState(sim_board);
        
        bool contains = false;
        episode.push_back(obs);
        if (obs_directory.empty()) { obs_directory.push_back(obs); }
        else
        {
            for (int i=0; i < obs_directory.size(); i++)
            {
                if (obs_directory[i] == obs)
                {
                    obs_directory[i].count++;
                    contains = true;
                }
            }
            if (!contains) { obs_directory.push_back(obs); }
        }
        tries++;
    }
    if (state != desired_state) { mainenv = oldenv; draft = oldseed; episode = old_episode; }  
    if (state != desired_state && global_tries < max_tries) { simulate(episode, obs_directory, mainenv, draft, Nobj, MINDIST, MOV, desired_state, global_tries+1, status); }

    // else push to main
    else if (state == desired_state && global_tries <= max_tries) { status = true; update(draft, sim_board, MOV); }
}

vector<vector<int>> TerrainEngine::distance_field(const vector<int>& start, const vector<vector<int>>& comap, int MOV)
{
    int WIDTH = comap[0].size();
    int HEIGHT =comap.size();
    using Node = tuple<int, int, int>;
    priority_queue<Node, vector<Node>, greater<Node>> open;
    vector<vector<int>> distance(HEIGHT, vector<int>(WIDTH, -1));
    distance[start[1]][start[0]] = 0;
    open.push({0, start[0], start[1]});

    vector<vector<int>> helpers = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};

    while (!open.empty())
    {
        auto [current_cost, x, y] = open.top();
        open.pop();

        for (const vector<int>& helper : helpers)
        {
            int new_x = x + helper[0];
            int new_y = y + helper[1];
            if (new_x < 0 || new_y < 0 || new_x >= HEIGHT || new_y >= WIDTH) { continue; }
            int next_cost = current_cost + comap[new_y][new_y];
            if (comap[new_y][new_x] > MOV)
                continue;
            if (next_cost < distance[new_y][new_x] || distance[new_y][new_x] == -1)
            {
                distance[new_y][new_x] = next_cost;
                open.push({next_cost, new_x, new_y});
            }
        }
    }
    return distance;
}

int main()
{
    // init
    int WIDTH = 15;
    int HEIGHT = 15;
    int MOV = 4;
    int Nobj = 2;
    int MINDIST = 5;
    int epochs = 10;
    vector<vector<int>> map(HEIGHT, vector<int>(WIDTH, 1));
    TerrainEngine engine(map, 2, 5, MOV);
    //engine.update(engine.get_seed(), engine.get_board(), MOV);
    print_board_state(engine.get_board());
    // evolve map
    vector<Observation> obs_directory;
    vector<Snapshot> history;
    while (epochs--)
    {
        bool status = false;
        TerrainSeed draft(map);
        Habitat env;
        BinaryStateSpace desired_state = ClearOwn;
        vector<Observation> episode;
        engine.simulate(episode, obs_directory, env, draft, Nobj, MINDIST, MOV, desired_state, 0, status);
        if (status) { history.push_back({env, draft, engine.get_board(), episode}); }
        cout << epochs << '\n';
    }
    
    Canvas canvas(WIDTH, HEIGHT);

    // Visualization performed by codex because I have no interest in wasting time in plots
    
    vector<const char*> state_names = {"ClearOwn", "Unfair", "FullyIsolated", "PartIsolated", "Balanced", "Unbalanced", "NOTA"};
    vector<const char*> action_names = {"GenerateRiver", "MutateRiver"};
    vector<const char*> pair_names = {"PP", "PV", "VV"};

    cout << "\n=== EPOCH HISTORY ===\n";
    cout << "Legend  🟦 main river  🟪 river extension  🟨 mountain  🟧 forest  🟥 village  🟩 player\n";
    for (int epoch=0; epoch < history.size(); epoch++)
    {
	const Snapshot& snapshot = history[epoch];
	Canvas epoch_canvas(WIDTH, HEIGHT);
	epoch_canvas.plot(snapshot.env.mountains, 4);
	epoch_canvas.plot(snapshot.env.forests, 6);
	epoch_canvas.plot(snapshot.env.river, 1);
	for (const vector<vector<int>>& extension : snapshot.env.river_extension)
	{
	    epoch_canvas.plot(extension, 5);
	}
	epoch_canvas.plot(snapshot.board.village_locations, 2);
	epoch_canvas.plot(snapshot.board.player_locations, 3);

	cout << "\nEpoch " << epoch+1 << "\n";
	if (snapshot.observations.empty())
	{
	    cout << "No actions were needed.\n";
	}
	else
	{
	    cout << "Transitions\n";
	    for (const Observation& obs : snapshot.observations)
	    {
		cout << "  " << state_names[obs.previous_state]
		     << " -- " << action_names[static_cast<int>(obs.action)]
		     << "(" << pair_names[static_cast<int>(obs.type)] << ") --> "
		     << state_names[obs.next_state] << '\n';
	    }
	}
	epoch_canvas.draw();
    }

    cout << "\n=== OBSERVATION FREQUENCIES ===\n";
    for (const Observation& obs : obs_directory)
    {
	cout << obs.count << "  "
	     << state_names[obs.previous_state]
	     << " -- " << action_names[static_cast<int>(obs.action)]
	     << "(" << pair_names[static_cast<int>(obs.type)] << ") --> "
	     << state_names[obs.next_state] << '\n';
    }
    return 0;
}
