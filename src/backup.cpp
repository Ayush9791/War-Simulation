BinaryStateSpace TerrainEngine::EvalBinaryState(BoardState& board)
{
    const vector<vector<int>>& vxp = board.VxP;
    if (vxp.empty())
    {
	return NOTA;
    }
    vector<vector<int>> bingrid(vxp.size(), vector<int>(vxp[0].size(), 0));
    for (int i=0; i < vxp.size(); i++)
    {
	for (int j=0; j < vxp[0].size(); j++)
	{
	    if (vxp[i][j] == 0) { bingrid[i][j] = 0; }
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
    vector<vector<int>> path;
    vector<vector<double>> explored(cost_grid.size(), vector<double>(cost_grid[0].size(), 0.0));
    for (int v=0; v < board.village_locations.size(); v++)
    {
	vector<int> vec = board.village_locations[v];
	path = distance_field(vec, cost_grid, MOV);
	for (int p=0; p < board.player_locations.size(); p++)
	{
	    vector<int> vec = board.player_locations[p];
	    board.VxP[v][p] = path[vec[1]][vec[0]];
	}
    }

    for (int v1=0; v1 < board.village_locations.size(); v1++)
    {
	vector<int> vec = board.village_locations[v1];
	path = distance_field(vec, cost_grid, MOV);
	for (int v2=0; v2 < board.village_locations.size(); v2++)
	{
	    vector<int> vec = board.village_locations[v2];
	    board.village[v1][v2] = path[vec[1]][vec[0]];
	}
    }

    for (int p1=0; p1 < board.player_locations.size(); p1++)
    {
	vector<int> vec = board.player_locations[p1];
	path = distance_field(vec, cost_grid, MOV);
	for (int p2=0; p2 < board.player_locations.size(); p2++)
	{
	    vector<int> vec = board.player_locations[p2];
	    board.player[p1][p2] = path[vec[1]][vec[0]];
	}
    }
    return board;
}

BoardState TerrainEngine::simulate_init(TerrainSeed& draft, const vector<vector<int>>& guilds, const vector<vector<int>>& villages, int MOV)
{
    BoardState board;
    //villages.size() x villages.size()
    vector<vector<int>> vlg(villages.size(), vector<int>(villages.size(), 0));
    vector<vector<int>> ply(guilds.size(), vector<int>(guilds.size(), 0));
    vector<vector<int>> vxp(villages.size(), vector<int>(guilds.size(), 0));
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

Observation TerrainEngine::Perform_Action(Habitat& mainenv, Action& action, BoardState& board, TerrainSeed& draft, int MOV, PositionBalance& pos)
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
            out.pos = pos;
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
            mainenv = env;
            draft.get_cost_map() = sim_cost_grid;
            board = new_board;
            out.previous_state = EvalBinaryState(old_board);
            out.next_state = EvalBinaryState(board);
            out.type = action.type;
            out.count = 1;
            out.pos = EvalPosition(MOV, draft, board);
            out.action = action.action;
        }
        else
        {
            out.previous_state = EvalBinaryState(old_board);
            out.next_state = EvalBinaryState(board);
            out.pos = EvalPosition(MOV, draft, board);
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
            out.pos = EvalPosition(MOV, draft, board);
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
            out.pos = EvalPosition(MOV, draft, board);
            out.type = action.type;
            out.count = 1;
            out.action = action.action;
        }
        else
        {
            out.previous_state = EvalBinaryState(old_board);
            out.next_state = EvalBinaryState(board);
            out.pos = EvalPosition(MOV, draft, board);
            out.type = action.type;
            out.count = 1;
            out.action = action.action;
        }
        break;
    }

    case ActionSpace::BuildBridge:
    {
        if (mainenv.river.empty() && mainenv.river_extension.empty())
            break;
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
        for (int ply2=0; ply2 < PLAYER; ply2++)
        {
            vector<int> loc = board.player_locations[ply2];
            out.combat[ply][ply2] = position[loc[1]][loc[0]];
        }
    }
    for (int vil=0; vil < VILLAGE; vil++)
    {
        for (int ply1=0; ply1 < PLAYER; ply1++)
        {
            for (int ply2=0; ply2 < PLAYER; ply2++)
            {
                if (village_costs_per_player[ply1][vil] != -1 && village_costs_per_player[ply2][vil] != -1)
                    out.position[vil][ply1][ply2] = abs(village_costs_per_player[ply1][vil] - village_costs_per_player[ply2][vil]);
                else
                    out.position[vil][ply1][ply2] = -1;
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
