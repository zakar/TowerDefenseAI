#include "BlockSolver.h"
#include <assert.h>

#define GH GridHandler::Instance()

BlockSolver::BlockSolver()
{
  fd = fopen("../bak/res.txt", "w");
}

BlockSolver& BlockSolver::Instance()
{
  static BlockSolver ins;
  return ins;
}

void BlockSolver::Init()
{
  int x, y, a, c, l, t, s;

  fscanf(stdin, "%d%d%d%d", &player_life, &money, &init_tower_cnt, &init_enemy_cnt);

  memset(mp_tower, 0xff, sizeof(mp_tower));
  tower_info.clear();
  for (int i = 0; i < init_tower_cnt; ++i) {
    fscanf(stdin, "%d%d%d%d", &x, &y, &a, &c);
    mp_tower[y][x] = ((a << 2) | c);
    tower_info.push_back(Tower(a, c, y, x));
  }
  
  enemy_info.clear();
  for (int i = 0; i < init_enemy_cnt; ++i) {
    fscanf(stdin, "%d%d%d%d%d", &x, &y, &t, &l, &s);
    enemy_info.push_back(Enemy(t, l, s, i, y, x));
  }

  fscanf(stdin, " END");

  /**************************************************************/

  if (init_tower_cnt == 0) {
    H = GH.H;
    W = GH.W;

    enemy_entry = GridHandler::Instance().GetEnemyEntry();
    defend_entry = GridHandler::Instance().GetDefendEntry();

    enemy_block_near.clear();
    defend_block_near.clear();
    goal_path.clear();
    goal_near_block.clear();
    enemy_path.clear();
    enemy_all_path.clear();
    enemy_all_block_near.clear();
    opt_grid.clear();
    
    for (size_t i = 0; i < enemy_entry.size(); ++i) {
      enemy_block_near.push_back(vector<Vec2>());
      enemy_path.push_back(vector<Vec2>());
      opt_grid.push_back(vector<BlockSolver::PassedGridInfo>());
    }
    for (size_t i = 0; i < defend_entry.size(); ++i) {
      defend_block_near.push_back(vector<Vec2>());
    }
    
    GH.SetBlock(enemy_entry, 'e');
    GH.SetBlock(defend_entry, 'd');
    
    for (size_t i = 0; i < enemy_entry.size(); ++i) {
      GH.CalNearBlockable(vector<Vec2>(1, enemy_entry[i]), enemy_block_near[i]);
    }

    for (size_t i = 0; i < defend_entry.size(); ++i) {
      GH.CalNearBlockable(vector<Vec2>(1, defend_entry[i]), defend_block_near[i]);
    }
    
    GH.UnSetBlock(enemy_entry);
    GH.UnSetBlock(defend_entry);
  }
}

void BlockSolver::Run()
{
  if (init_tower_cnt == 0) {
    CalRoute();

    RouteClear();
    for (size_t i = 0; i < grid2build.size(); ++i) {
      Vec2 &p = grid2build[i];
      GH.grid_info[p.x][p.y] = 't';
    }
    //GH.Debug();
    GH.CalAllEnemyMovePath();
  }

  CalChoice();
}

void BlockSolver::CalChoice()
{

  // for (size_t i = 0; i < enemy_entry.size(); ++i) {
  //   printf("idx: %d  opt_size: %d\n", i, opt_grid[i].size());
  // }
  //   for (size_t j = 0; j < opt_grid[i].size(); ++j)  printf("axis: %d %d\n", opt_grid[i][j].position.x, 
  // 							    opt_grid[i][j].position.y);
  // }

  int cost, min_cost = INF;
  vector<Tower> res, best_res;

  const int LIM_CNT = 6;
  const int LIMIT[LIM_CNT] = { 4, 8, 16, 32, 64, 128 };

  int lidx = 0;
  for (;;) {

    tower2build.clear();
    for (size_t i = 0; i < grid2build.size(); ++i) {
      Vec2 &p = grid2build[i];
      tower2build.push_back(Tower(0, 0, p.x, p.y));
    }

    MatchChecker::Instance().Init(enemy_info, tower2build, min(1, player_life));
    MatchChecker::Instance().Run();
    if (MatchChecker::Instance().IsWin()) {
      CalCost(cost, res, tower2build);
      if (cost < min_cost) {
	min_cost = cost;
	best_res = res;
      }
    }

    for (int lev = 0; lev <= 4; ++lev) {
      for (int idx = 0; idx <= LIMIT[lidx]; ++idx) {
	
	//	int enemy_life_status
	for (size_t i = 0; i < enemy_entry.size(); ++i) {
	  if ((int)opt_grid[i].size() <= idx) continue;
	  Vec2 &p = opt_grid[i][idx].position;
	  Tower tw = Tower(lev, 1, p.x, p.y);
	  for (size_t j = 0; j < tower2build.size(); ++j) {
	    if (tower2build[j].position == tw.position && tower2build[j].CheckDiff(tw)) {
	      tower2build.erase(tower2build.begin()+j);
	      tower2build.push_back(tw);
	      break;
	    }
	  }
	}

	MatchChecker::Instance().Init(enemy_info, tower2build, min(2, player_life));
	MatchChecker::Instance().Run();
	if (MatchChecker::Instance().IsWin()) {
	  CalCost(cost, res, tower2build);
	  if (cost < min_cost) {
	    min_cost = cost;
	    best_res = res;
	  }
	  goto nextlimit;
	}
      }
    }

  nextlimit:
    if (++lidx >= LIM_CNT) break;
  }

  if (!MatchChecker::Instance().IsWin()) while(1);

  //printf("min_cost: %d\n", min_cost);
  Output(best_res);
}

void BlockSolver::CalCost(int &cost, vector<Tower> &res, const vector<Tower>& tower2build)
{
  res.clear();
  cost = 0;
  for (size_t i = 0; i < tower2build.size(); ++i) {
    int x = tower2build[i].position.x;
    int y = tower2build[i].position.y;
    int lev = tower2build[i].level;
    int ty = tower2build[i].type;

    int mlev = mp_tower[x][y] >> 2;
    int mty = mp_tower[x][y] & 3;
    if (mp_tower[x][y] == -1) {
      cost += tower2build[i].BuildCost();
      res.push_back(tower2build[i]);
    } else  {
      if (mty != ty) {
	cost += tower2build[i].BuildCost();
	res.push_back(tower2build[i]);
      } else if (mlev < lev) {
	cost += tower2build[i].BuildCost() - Tower::BuildCost(mlev, mty);
	res.push_back(tower2build[i]);
      }
    }
  }
}

void BlockSolver::Output(vector<Tower>& res)
{
  printf("%d\n", res.size());
  for (size_t i = 0; i < res.size(); ++i) 
    printf("%d %d %d %d\n", res[i].position.y, res[i].position.x, res[i].level, res[i].type);

  // fprintf(fd, "%d\n", res.size());
  // for (size_t i = 0; i < res.size(); ++i) 
  //   fprintf(fd, "%d %d %d %d\n", res[i].position.y, res[i].position.x, res[i].level, res[i].type);
  
  // fflush(fd);
  fflush(stdout);
}

void BlockSolver::CalRoute()
{
  string tmp;

  best_score = 0;
  for (size_t l = 0; l < defend_entry.size(); ++l) {
    
    RouteInit();
  reenter:
    while (RouteIter()) {      

      for (size_t i = 0; i < defend_block_near.size(); ++i) 
	if (i != l) GH.SetBlock(defend_block_near[i], 't');

      for (size_t i = 0; i < enemy_block_near.size(); ++i) GH.SetBlock(enemy_block_near[i], 't');

      if (GH.CalMovePath(goal_dst, vector<Vec2>(1, defend_entry[l]), goal_path, tmp)) goto reenter;
      GH.SetBlock(goal_path, 'e');
      for (size_t i = 0; i < enemy_entry.size(); ++i) GH.UnSetBlock(enemy_block_near[i]);
      GH.CalNearBlockable(goal_path, goal_near_block);
      GH.SetBlock(goal_near_block, 't');

      enemy_all_path.clear();
      for (size_t i = 0; i < enemy_entry.size(); ++i) {
	if (GH.CalMovePath(enemy_entry[i], vector<Vec2>(1, enemy_dst), enemy_path[i], tmp)) goto reenter;
	GH.GetUnion(enemy_all_path, enemy_path[i]);
      }
      
      enemy_all_block_near.clear();
      GH.SetBlock(enemy_all_path, 'e');
      GH.CalNearBlockable(enemy_all_path, enemy_all_block_near);
      GH.UnSetBlock(enemy_all_path);
      GH.SetBlock(enemy_all_block_near, 't');

      GH.UnSetBlock(goal_path);
      GH.UnSetBlock(enemy_all_path);
      GH.UnSetBlock(mid);

      if (GH.CalAllEnemyMovePath()) {
	continue;
      }

      RouteAnalysis();
    }
  }
}

int BlockSolver::RouteClear()
{
  for (int i = 0; i < H; ++i)
    for (int j = 0; j < W; ++j) 
      if (GH.grid_info[i][j] != '1') GH.grid_info[i][j] = '0';

  return 0;
}

int BlockSolver::RouteInit()
{
  mx = 1;
  my = 1;
  return 0;
}

int BlockSolver::RouteIter()
{
  RouteClear();

  while (mx < H && my < W) {

    ++my;
    if (my == W) { my = 1; ++mx; }

    mid = Vec2(mx, my);
    up = mid + Vec2(1, 0);
    down = mid + Vec2(-1, 0);
    left = mid + Vec2(0, -1);
    right = mid + Vec2(0, 1);
    
    if (GH.CheckBuildable(mid) && GH.CheckBuildable(up) && GH.CheckBuildable(down) &&
	GH.CheckBuildable(left) && GH.CheckBuildable(right)) {
      
      GH.SetBlock(mid, 't');
      GH.SetBlock(left, 't');
      GH.SetBlock(right, 't');
      goal_dst = down;
      enemy_dst = up;

      break;
    }
  }

  return my < W && mx < H;
}

int BlockSolver::RouteAnalysis()
{
  if ((int)goal_path.size() > best_score) {
    best_score = goal_path.size();
    grid2build.clear();

    for (int i = 0; i < H; ++i)
      for (int j = 0; j < W; ++j) {
	if (GH.grid_info[i][j] == 't')  grid2build.push_back(Vec2(i, j));
      }

    for (size_t i = 0; i < enemy_entry.size(); ++i) {
      RouteCalOpt(opt_grid[i], GH.GetEnemyMovePath(enemy_entry[i]));
    }
  }
  
  return 0;
}

int BlockSolver::RouteCalOpt(vector<PassedGridInfo>& opt, const vector<Vec2>& path)
{
  memset(grid_rank, 0xff, sizeof(grid_rank));
  opt.clear();
    
  for (size_t j = 0; j < path.size(); ++j) {
    int x = path[j].x;
    int y = path[j].y;
    grid_rank[x][y] = (int)j;
  }
  
  for (int i = 0; i < H; ++i) {
    for (int j = 0; j < W; ++j) {
      if (GH.grid_info[i][j] != 't') continue;
      PassedGridInfo p(i, j);
      int min_ti = INF, max_ti = -1;
      
      for (int l = 0; l < 8; ++l) {
	int x = p.position.x + dir[l][0];
	int y = p.position.y + dir[l][1];
	if (0 <= x && x < H && 0 <= y && y < W) {
	  if (grid_rank[x][y] != -1) {
	    min_ti = min(min_ti, grid_rank[x][y]);
	    max_ti = max(max_ti, grid_rank[x][y]);
	  }
	}
      }
      
      if (min_ti != INF) {
	p.max_can_recharge = max_ti - min_ti;
	p.route_idx = max_ti;
	opt.push_back(p);
      }
    }
  }

  sort(opt.begin(), opt.end());

  return 0;
}

void BlockSolver::Debug()
{
  RouteClear();

  for (size_t i = 0; i < grid2build.size(); ++i) {
    Vec2 &p = grid2build[i];
    GH.grid_info[p.x][p.y] = 't';
  }

  GH.Debug();

  RouteClear();
}
