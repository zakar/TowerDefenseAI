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

    const vector<Vec2>& entry = GridHandler::Instance().GetEnemyEntry();
    for (size_t j = 0; j < entry.size(); ++j) {
      if (y == entry[j].x && x == entry[j].y) {
	enemy_info[i].occur_entry = j;
	break;
      }
    }
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
    enemy_path.clear();
    enemy_all_path.clear();
    enemy_all_block_near.clear();
    opt_grid.clear();

    for (int i = 0; i < GOAL_CNT; ++i) {
      goal_path[i].clear();
      goal_near_block[i].clear();
    }
    
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
    //    Debug();

    RouteClear();
    for (size_t i = 0; i < grid2build.size(); ++i) {
      Vec2 &p = grid2build[i];
      GH.grid_info[p.x][p.y] = 't';
    }

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
  int entry_status;
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

    entry_status = (1 << enemy_entry.size()) - 1;

    for (int lev = 0; lev <= 4; ++lev) {
      for (int idx = 0; idx <= LIMIT[lidx]; ++idx) {
	
	for (size_t i = 0; i < enemy_entry.size(); ++i) {
	  if ((int)opt_grid[i].size() <= idx) continue;

	  if (entry_status & (1 << i)) {

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

	entry_status = 0;
	const vector<MatchChecker::EnemyInfo>& ef = MatchChecker::Instance().GetEnemyInfo();
	for (size_t i = 0; i < ef.size(); ++i) {
	  if (ef[i].remain_life > 0) entry_status |= (1 << ef[i].info.occur_entry);
	}
      }
    }

  nextlimit:
    if (++lidx >= LIM_CNT) break;
  }

  if (!MatchChecker::Instance().IsWin()) while(1);

  //  printf("min_cost: %d\n", min_cost);
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

      //goal[0] to defend_entry
      if (GH.CalMovePath(goal_dst[0], vector<Vec2>(1, defend_entry[l]), goal_path[0], tmp)) goto reenter;
      GH.SetBlock(goal_path[0], 'e');
      GH.CalNearBlockable(goal_path[0], goal_near_block[0]);
      GH.SetBlock(goal_near_block[0], 't');
      
      //goal[i] to goal[i+1]
      for (int k = 1; k < GOAL_CNT; ++k) {
	if (GH.CalMovePath(goal_dst[k], vector<Vec2>(1, enemy_dst[k-1]), goal_path[k], tmp)) goto reenter;
	GH.SetBlock(goal_path[k], 'e');
	GH.CalNearBlockable(goal_path[k], goal_near_block[k]);
	GH.SetBlock(goal_near_block[k], 't');
      }
      //

      // unblock enemy_near --- block defend_near --- block goal_near
      for (size_t i = 0; i < enemy_entry.size(); ++i) GH.UnSetBlock(enemy_block_near[i]);
      for (size_t i = 0; i < defend_block_near.size(); ++i) 
	if (i != l) GH.SetBlock(defend_block_near[i], 't');
      for (int i = 0; i < GOAL_CNT; ++i) GH.SetBlock(goal_near_block[i], 't');

      enemy_all_path.clear();
      for (size_t i = 0; i < enemy_entry.size(); ++i) {
	if (GH.CalMovePath(enemy_entry[i], vector<Vec2>(1, enemy_dst[GOAL_CNT-1]), enemy_path[i], tmp)) goto reenter;
	GH.GetUnion(enemy_all_path, enemy_path[i]);
      }
      
      enemy_all_block_near.clear();
      GH.SetBlock(enemy_all_path, 'e');
      GH.CalNearBlockable(enemy_all_path, enemy_all_block_near);
      GH.UnSetBlock(enemy_all_path);
      GH.SetBlock(enemy_all_block_near, 't');

      for (int i = 0; i < GOAL_CNT; ++i) {
	GH.UnSetBlock(goal_path[i]);
	GH.UnSetBlock(mid[i]);
      }
      GH.UnSetBlock(enemy_all_path);

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
  iter_count[0] = 0;
  iter_count[1] = 0;
  for (int i = 0; i < GOAL_CNT; ++i) mx[i] = my[i] = 1;
  return 0;
}

int BlockSolver::RouteIter()
{
  RouteClear();

  const int LIMIT[GOAL_CNT] = { 1000, 50 };

  while (mx[0] < H && my[0] < W) {

    int idx = GOAL_CNT - 1;
    for (; idx >= 0; --idx) {
      if (mx[idx] < H && my[idx] < W && iter_count[idx] <= LIMIT[idx]) {
	++my[idx];
	if (my[idx] == W) { my[idx] = 1; ++mx[idx]; }
	++iter_count[idx];
	break;
      } else {
	mx[idx] = 1;   
	my[idx] = 1;
	iter_count[idx] = 0;
      }
    }

    int flag = 1;
    for (int i = 0; i < GOAL_CNT; ++i) {
      mid[i] = Vec2(mx[i], my[i]);
      up[i] = mid[i] + Vec2(-1, 0);
      down[i] = mid[i] + Vec2(1, 0);
      left[i] = mid[i] + Vec2(0, -1);
      right[i] = mid[i] + Vec2(0, 1);
    
      if (GH.CheckBuildable(mid[i]) && GH.CheckBuildable(up[i]) && GH.CheckBuildable(down[i]) &&
	  GH.CheckBuildable(left[i]) && GH.CheckBuildable(right[i])) {
      
	GH.SetBlock(mid[i], 't');
	GH.SetBlock(left[i], 't');
	GH.SetBlock(right[i], 't');
	goal_dst[i] = down[i];
	enemy_dst[i] = up[i];
      } else {
	for (int j = 0; j < i; ++j) {
	  GH.UnSetBlock(mid[j]);
	  GH.UnSetBlock(left[j]);
	  GH.UnSetBlock(right[j]);
	}
	flag = 0;
	break;
      }
    }

    if (flag) break;
  }

  return my[0] < W && mx[0] < H;
}

int BlockSolver::RouteAnalysis()
{
  int cur_score = 0;
  for (int i = 0; i < GOAL_CNT; ++i) cur_score += goal_path[i].size();
  
  if (cur_score > best_score) {
    best_score = cur_score;
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
	p.max_can_recharge = (max_ti - min_ti)/10; //this is useful
	p.route_idx = max_ti;  //this is useful
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
