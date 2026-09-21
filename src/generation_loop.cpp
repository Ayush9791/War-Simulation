#include <random>
#include <vector>
#include "plotter.h"
using namespace std;
template <typename T>
using Vec = vector<T>;
template <typename T>
using Vec2D = vector<vector<T>>;
template <typename T>
using Vec3D = vector<vector<vector<T>>>;

struct speck
{
    double none;
    double forest;
};

Vec<int> random_coord(int WIDTH, int HEIGHT)
{
    vector<int> out;
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> width_sample(0, WIDTH-1);
    uniform_int_distribution<int> height_sample(0, HEIGHT-1);
    return {width_sample(rng), height_sample(rng)};
}

bool paint(const speck& prob)
{
    random_device rd;
    mt19937 rng(rd());
    bernoulli_distribution decide(prob.forest);
    return decide(rng);
}

int RNG(int interval)
{
    vector<int> out;
    random_device rd;
    mt19937 rng(rd());
    uniform_int_distribution<int> sample(0, interval-1);
    return sample(rng);
}

Vec2D<int> helper_ = {
                  {0, -1},        
	{-1,  0},          {1,  0},
                  {0,  1}
    };

vector<vector<int>> helper = {
	{-1, -1},          {1, -1},
      
	{-1,  1},          {1,  1}
    };
    
Vec2D<int> evolve(int WIDTH, int HEIGHT, Vec<int> start, int density)
{
    Vec2D<speck> inp(HEIGHT, Vec(WIDTH, speck{1, 0}));
    if (start.empty())
    {
        Vec<int> random = random_coord(WIDTH, HEIGHT);
        inp[random[1]][random[0]] = speck{-1, 1};
    }
    else { inp[start[1]][start[0]] = speck{-1, 1}; }
    
    vector<vector<int>> helper = {
	{-1, -1},          {1, -1},
      
	{-1,  1},          {1,  1}
    };
    for (int i=0; i < WIDTH; i++)
    {
        for (int j=0; j < HEIGHT; j++)
        {
            int out = 0;
            if (inp[j][i].none == 1)
            {
                for (int help=0; help < helper.size(); help++)
                {
                    int new_x = i+helper[help][0];
                    int new_y = j+helper[help][1];
                    if (new_x < 0 || new_y < 0 || new_x >= WIDTH || new_y >= HEIGHT) { continue; }
                    if (inp[new_y][new_x].forest == 1) { out++; }
                }
                if (out >= 2) { inp[j][i].forest =  0.8; inp[j][i].none = 1 - inp[j][i].forest ;}
                if (out == 1) { inp[j][i].forest =  1.0; inp[j][i].none = 1 - inp[j][i].forest ;}
                if (out == 0) { inp[j][i].forest =  0.2; inp[j][i].none = 1 - inp[j][i].forest ;}
            }
        }
    }

    for (int i=0; i < WIDTH; i++)
    {
        for (int j=0; j < HEIGHT; j++)
        {
            if (inp[j][i].forest == -1 || inp[j][i].none == -1) { continue; }
            if (inp[j][i].forest == 0 && inp[j][i].none == 1) { continue; }
            if (paint(inp[j][i])) { inp[j][i].forest = 1; inp[j][i].none = -1; }
            else { inp[j][i].forest = -1; inp[j][i].none = 1; }
        }
    }
    
    Vec2D<int> out;
    for (int i=0; i < inp[0].size(); i++)
    {
        for (int j=0; j < inp.size(); j++)
        {
            if (inp[j][i].forest == 1) { out.push_back({i, j}); }
        }
    }
    return out;
}


int main()
{
    int WIDTH = 5;
    int HEIGHT = 5;
    int density = 1;
    Vec2D<int> output = evolve(WIDTH, HEIGHT, {}, density);
    Canvas canvas(WIDTH, HEIGHT);
    canvas.plot(output, 6);
    canvas.draw();
    return 0;
}
