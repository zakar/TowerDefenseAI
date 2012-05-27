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
    mp_tower[y][x] = (a << 2) | c;
    tower_info.push_back(Tower(a, c, y, x));
  }
  
  enemy_info.clear();
  for (int i = 0; i < init_enemy_cnt; ++i) {
    fscanf(stdin, "%d%d%d%d%d", &x, &y, &t, &l, &s);
    enemy_info.push_back(Enemy(t, l, s, i, y, x));
  }

  fscanf(stdin, " END");

  /**************************************************************/
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

  for (size_t i = 0; i < enemy_entry.size(); ++i) {
    enemy_block_near.push_back(vector<Vec2>());
    enemy_path.push_back(vector<Vec2>());
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

void BlockSolver::Run()
{
  if (init_tower_cnt == 0) {
    CalRoute();
    //    Debug();
  }

  CalChoice();
}

void BlockSolver::CalChoice()
{
  RouteClear();

  for (size_t i = 0; i < grid2build.size(); ++i) {
    Vec2 &p = grid2build[i].position;
    GH.grid_info[p.x][p.y] = 't';
  }

  //  GH.Debug();

  GH.CalAllEnemyMovePath();

  int cost, min_cost = INF;
  vector<Tower> res, best_res;

  const int SIZE[4] = { 64, 64, 64, 64 };

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < SIZE[i]; ++j) {
      tower2build.clear();
      int limit = min(j, (int)grid2build.size());

      for (int l = 0; l < limit; ++l) 
	tower2build.push_back(Tower(i, 1, grid2build[l].position.x, grid2build[l].position.y));

      for (size_t l = limit; l < grid2build.size(); ++l)
	tower2build.push_back(Tower(0, 0, grid2build[l].position.x, grid2build[l].position.y));

      MatchChecker::Instance().Init(enemy_info, tower2build, min(2, player_life));
      MatchChecker::Instance().Run();
      if (MatchChecker::Instance().IsWin()) {
	CalCost(cost, res, tower2build);
	if (cost < min_cost) {
	  min_cost = cost;
	  best_res = res;
	}
      }
    }
  }

  Output(best_res);
}

void BlockSolver::CalCost(int &cost, vector<Tower> &res, vector<Tower>& tower2build)
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

  fprintf(fd, "%d\n", res.size());
  for (size_t i = 0; i < res.size(); ++i) 
    fprintf(fd, "%d %d %d %d\n", res[i].position.y, res[i].position.x, res[i].level, res[i].type);
  
  fflush(fd);

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

      for (size_t i = 0; i < defend_block_near.size(); ++i) GH.SetBlock(defend_block_near[i], 't');
      for (size_t i = 0; i < enemy_block_near.size(); ++i) GH.SetBlock(enemy_block_near[i], 't');
      GH.UnSetBlock(defend_block_near[l]);

      //      GH.Debug();

      if (GH.CalMovePath(goal_dst, vector<Vec2>(1, defend_entry[l]), goal_path, tmp)) goto reenter;
      GH.SetBlock(goal_path, 'e');
      for (size_t i = 0; i < enemy_entry.size(); ++i) GH.UnSetBlock(enemy_block_near[i]);
      GH.CalNearBlockable(goal_path, goal_near_block);
      GH.SetBlock(goal_near_block, 't');

      //      GH.Debug();

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

      //      GH.Debug();

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

    memset(grid_rank, 0xff, sizeof(grid_rank));
    memset(grid_mask, 0, sizeof(grid_mask));
    for (size_t i = 0; i < enemy_entry.size(); ++i) {
      vector<Vec2>& path = GH.GetEnemyMovePath(enemy_entry[i]);
      
      for (size_t j = 0; j < path.size(); ++j) {
	int x = path[j].x;
	int y = path[j].y;
	grid_rank[x][y] = max(grid_rank[x][y], (int)i);
	grid_mask[x][y] = (1 << i);
      }
    }

    for (int i = 0; i < H; ++i) {
      for (int j = 0; j < W; ++j) {
	if (GH.grid_info[i][j] != 't') continue;
	PassedGridInfo p(i, j);
	p.mask = 0;
	int min_ti = INF, max_ti = 0;

	for (int l = 0; l < 8; ++l) {
	  int x = p.position.x + dir[l][0];
	  int y = p.position.y + dir[l][1];
	  if (0 <= x && x < H && 0 <= y && y < W) {
	    if (grid_rank[x][y] != -1) {
	      min_ti = min(min_ti, grid_rank[x][y]);
	      max_ti = max(max_ti, grid_rank[x][y]);
	    }
	    p.mask |= grid_mask[x][y];
	  }
	}

	p.max_dist = max(0, max_ti - min_ti);
	p.mask_cnt = __builtin_popcount(p.mask);
	grid2build.push_back(p);
      }
    }
  }

  sort(grid2build.begin(), grid2build.end());
  
  return 0;
}

void BlockSolver::Debug()
{
  RouteClear();

  for (size_t i = 0; i < grid2build.size(); ++i) {
    Vec2 &p = grid2build[i].position;
    GH.grid_info[p.x][p.y] = 't';
  }

  GH.Debug();

  RouteClear();
}
