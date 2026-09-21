#include <random>
#include <vector>
#include <algorithm>
#include "raytracer.h"
#include "procedural_generation.h"
#include "general_pathtracing.h"
#include "game_data.h"
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

vector<vector<int>> TerrainSeed::distance_field(const vector<int>& start, const vector<vector<int>>& comap, int MOV)
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
            if (new_x < 0 || new_y < 0 || new_x >= WIDTH || new_y >= HEIGHT) { continue; }
            int next_cost = current_cost + comap[new_y][new_x];
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

vector<vector<int>> TerrainSeed::locate(vector<vector<int>>& out, const vector<int>& start, const vector<int>& end, const vector<vector<int>>& dijkstra, int MOV)
{
    vector<vector<int>> helpers = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
    int x = start[0]; int y = start[1];
    helpers = shuffle(helpers);
    for (const vector<int>& coord : helpers)
    {
        int new_x = x + coord[0]; int new_y = y + coord[1];
        if (new_x < 0 || new_y < 0 || new_x >= dijkstra.size() || new_y >= dijkstra[0].size() ||
            dijkstra[new_y][new_x] != dijkstra[y][x]+1)
            continue;
        if (dijkstra[new_y][new_x] == dijkstra[y][x]+1)
            out.push_back({new_x, new_y});
        if (vector<int>({new_x, new_y}) != end)
            locate(out, {new_x, new_y}, end, dijkstra, MOV); break;
    }
    return out;
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

void TerrainSeed::build_bridge(const vector<vector<int>>& river, const vector<int>& point, int MOV)
{
    // if invalid suggestion, commit no changes?
    if (if_in_(river, point))
    {
        if (cost_map[point[1]][point[0]] < MOV+1)
            throw invalid_argument("Mismatch between river and comap. Maybe stale cost_map?");
        cost_map[point[1]][point[0]] = 1;
    }
}

vector<vector<int>> TerrainSeed::get_path(vector<int> current, vector<int> target, int MOV)
{
    return recurse(current, target, cost_map, MOV, explored);
}

bool TerrainSeed::paint(const speck& prob)
{
    random_device rd;
    mt19937 rng(rd());
    bernoulli_distribution decide(prob.forest);
    return decide(rng);
}


// patch --> relative coords --> absolute coords
vector<vector<int>> TerrainSeed::generate_forest(vector<int> spawnpoint, int radius, int terrain_cost, int MOV)
{
    int DIAM = (2 * radius) + 1;
    vector<vector<speck>> inp(DIAM, vector<speck>(DIAM, speck{1, 0}));
    inp[radius][radius] = speck{-1, 1};
    
    vector<vector<int>> helper = {
	{-1, -1},          {1, -1},
      
	{-1,  1},          {1,  1}
    };
    for (int i=0; i < DIAM; i++)
    {
        for (int j=0; j < DIAM; j++)
        {
            int out = 0;
            if (inp[j][i].none == 1)
            {
                for (int help=0; help < helper.size(); help++)
                {
                    int new_x = i+helper[help][0];
                    int new_y = j+helper[help][1];
                    if (new_x < 0 || new_y < 0 || new_x >= DIAM || new_y >= DIAM) { continue; }
                    if (inp[new_y][new_x].forest == 1) { out++; }
                }
                if (out >= 2) { inp[j][i].forest =  0.8; inp[j][i].none = 1 - inp[j][i].forest ;}
                if (out == 1) { inp[j][i].forest =  1.0; inp[j][i].none = 1 - inp[j][i].forest ;}
                if (out == 0) { inp[j][i].forest =  0.2; inp[j][i].none = 1 - inp[j][i].forest ;}
            }
        }
    }

    for (int i=0; i < DIAM; i++)
    {
        for (int j=0; j < DIAM; j++)
        {
            if (inp[j][i].forest == -1 || inp[j][i].none == -1) { continue; }
            if (inp[j][i].forest == 0 && inp[j][i].none == 1) { continue; }
            if (paint(inp[j][i])) { inp[j][i].forest = 1; inp[j][i].none = -1; }
            else { inp[j][i].forest = -1; inp[j][i].none = 1; }
        }
    }
    
    vector<vector<int>> out;
    for (int i=0; i < inp[0].size(); i++)
    {
        for (int j=0; j < inp.size(); j++)
        {
            int actual_x = i+spawnpoint[0]-radius;
            int actual_y = j+spawnpoint[1]-radius;
            if (inp[j][i].forest == 1 && cost_map[actual_y][actual_x]+terrain_cost <= MOV) { out.push_back({actual_x, actual_y}); }
        }
    }
    return out;
}
