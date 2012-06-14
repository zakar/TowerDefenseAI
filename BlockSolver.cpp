#include "BlockSolver.h"
#include "GA_BuildTower.h"
#include <assert.h>

#define GH GridHandler::Instance()

BlockSolver::BlockSolver()
{
}

BlockSolver& BlockSolver::Instance()
{
  static BlockSolver ins;
  return ins;
}

void BlockSolver::Init(int stage_ver, int stage_lev)
{
  this->stage_ver = stage_ver;
  this->stage_lev = stage_lev;

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
    assert(stage_lev == 0);
    H = GH.H;
    W = GH.W;

    enemy_entry = GridHandler::Instance().GetEnemyEntry();
    defend_entry = GridHandler::Instance().GetDefendEntry();

    grid2build.clear();
    grid_for_type_1.clear();
    grid_for_type_2.clear();

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
  }

  CalChoice();
}

void BlockSolver::CalChoice()
{
  RouteClear();
  for (size_t i = 0; i < grid2build.size(); ++i) {
    const Vec2 &p = grid2build[i];
    assert(GH.grid_info[p.x][p.y] == '0');
    GH.grid_info[p.x][p.y] = 't';
  }
  GH.CalAllEnemyMovePath();

  min_cost = INF;
  best_tower2build.clear();

#ifdef _BUILD_TOWER_GREED_
  BuildTowerByGreed();
#endif

#ifdef _BUILD_TOWER_GA_
  vector<Vec2> cur_grid;
  int gene_cnt = min((int)opt_grid.size(), 5);
  for (int i = 0; i < gene_cnt; ++i) cur_grid.push_back(opt_grid[i].position);
  GA_BuildTower::Instance().SetArgs(10, gene_cnt, 10, enemy_info);
  GA_BuildTower::Instance().Init(cur_grid, vector<int>(gene_cnt, 1), vector<int>(gene_cnt, 1));
  GA_BuildTower::Instance().Run();
  vector<Tower> cur_tw = GA_BuildTower::Instance().GetResult();
  TowerBestCheck(cur_tw, 1);
#endif

  TowerTryEliminate(best_tower2build);
}

void BlockSolver::BuildTowerByGreed()
{
  vector<Tower> init_tower;
  vector<Tower> tower2build;

  tower2build.clear();
  for (size_t i = 0; i < grid2build.size(); ++i) {
    const Vec2 &p = grid2build[i];
    tower2build.push_back(Tower(0, 0, p.x, p.y));
  }
  if (TowerBestCheck(tower2build, 1)) {
    return;
  }

  init_tower.clear();
  for (size_t i = 0; i < grid_for_type_1.size(); ++i) {
    const Vec2 &p = grid_for_type_1[i];
    init_tower.push_back(Tower(0, 0, p.x, p.y));
  }
  for (size_t i = 0; i < grid_for_type_2.size(); ++i) {
    const Vec2 &p = grid_for_type_2[i];
    init_tower.push_back(Tower(1, 2, p.x, p.y));
  }
  if (TowerBestCheck(init_tower, 1)) {
    return;
  }

  const int LIMIT = 3125;
  int t2lev[5];

  for (int state = 0;;++state) {
    if (state >= LIMIT) {
      ++t2lev[4];
    } else {
      for (int tmp = state, i = 0; i < 5; ++i) {
	t2lev[i] = tmp%5;
	tmp /= 5;
      }
    }

    tower2build = init_tower;

    int idx = 0;
    for (int lev = 4; lev >= 0; --lev) {
      for (int len = 0; len < t2lev[lev]; ++len) {
	if ((int)opt_grid.size() <= idx) continue;
	const Vec2 &p = opt_grid[idx].position;
	Tower tw = Tower(lev, 1, p.x, p.y);
	for (size_t j = 0; j < tower2build.size(); ++j) {
	  if (tower2build[j].position == tw.position && tower2build[j].CheckDiff(tw)) {
	    tower2build.erase(tower2build.begin()+j);
	    tower2build.push_back(tw);
	    break;
	  }
	}
	++idx;
      }
    }

    if (TowerBestCheck(tower2build, 1)) break;
  }
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
	if (mty < ty) {
	  cost += tower2build[i].BuildCost();
	  res.push_back(tower2build[i]);
	}
      } else if (mlev < lev) {
	cost += tower2build[i].BuildCost() - Tower::BuildCost(mlev, mty);
	res.push_back(tower2build[i]);
      }
    }
  }
}

void BlockSolver::Output(const vector<Tower>& res)
{
  printf("%d\n", res.size());
  for (size_t i = 0; i < res.size(); ++i) 
    printf("%d %d %d %d\n", res[i].position.y, res[i].position.x, res[i].level, res[i].type);

  fflush(stdout);
}

void BlockSolver::CalRoute()
{
  string tmp;

  best_score = 0;
  for (size_t l = 0; l < defend_entry.size(); ++l) {

    int flag = 1;
    for (int k = 0; k < 4; ++k) {
      int x = defend_entry[l].x + dir[k][0];
      int y = defend_entry[l].y + dir[k][1];
      if (0 <= x && x < H && 0 <= y && y < W) {
	for (size_t ll = 0; ll < defend_entry.size(); ++ll) 
	  if (defend_entry[ll] == Vec2(x, y)) { flag = 0; break; }
      }
      if (!flag) break;
    }

    if (!flag) continue;

    RouteInit();
  reenter:
    while (RouteIter()) {      

      for (size_t i = 0; i < defend_block_near.size(); ++i) 
	if (i != l) { 
	  GH.SetBlock(defend_block_near[i], 't'); 
	}

      for (size_t i = 0; i < enemy_block_near.size(); ++i) GH.SetBlock(enemy_block_near[i], 't');

      if (GH.CalMovePath(goal_dst[0], vector<Vec2>(1, defend_entry[l]), goal_path[0], tmp)) goto reenter;
      GH.SetBlock(goal_path[0], 'e');
      GH.CalNearBlockable(goal_path[0], goal_near_block[0]);
      GH.SetBlock(goal_near_block[0], 't');
      
      for (int k = 1; k < GOAL_CNT; ++k) {
	if (GH.CalMovePath(goal_dst[k], vector<Vec2>(1, enemy_dst[k-1]), goal_path[k], tmp)) goto reenter;
	GH.SetBlock(goal_path[k], 'e');
	GH.CalNearBlockable(goal_path[k], goal_near_block[k]);
	GH.SetBlock(goal_near_block[k], 't');
      }

      for (size_t i = 0; i < enemy_entry.size(); ++i) GH.UnSetBlock(enemy_block_near[i]);
      for (size_t i = 0; i < defend_block_near.size(); ++i) 
	if (i != l) {
	  GH.SetBlock(defend_block_near[i], 't');
	}
	
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

  const int LIMIT[GOAL_CNT] = { 1000, 45 };

  while (mx[0] < H && my[0] < W) {

    int idx = GOAL_CNT - 1;
    for (; idx >= 0; --idx) {
      if (mx[idx] < H && my[idx] < W && iter_count[idx] <= LIMIT[idx]) {
	my[idx] += 2; 
	if (my[idx] >= W) { my[idx] = 1; ++mx[idx]; }
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

int BlockSolver::RouteCalGrid2Build(vector<Vec2>& grid)
{
  for (size_t i = 0; i < enemy_block_near.size(); ++i) GH.UnSetBlock(enemy_block_near[i]);
  for (size_t i = 0; i < defend_block_near.size(); ++i) GH.UnSetBlock(defend_block_near[i]);
  GH.SetBlock(enemy_all_block_near, 't');
  for (int i = 0; i < GOAL_CNT; ++i) GH.SetBlock(goal_near_block[i], 't');
  for (int i = 0; i < GOAL_CNT; ++i) {
    GH.UnSetBlock(goal_path[i]);
    GH.UnSetBlock(mid[i]);
    GH.SetBlock(left[i], 't'); 
    GH.SetBlock(right[i], 't');
  }
  GH.UnSetBlock(enemy_all_path);
  
  grid.clear();
  for (int i = 0; i < H; ++i)
    for (int j = 0; j < W; ++j) {
      if (GH.grid_info[i][j] == 't')  grid.push_back(Vec2(i, j));
    }

  return 0;
}

int BlockSolver::RouteAnalysis()
{
  int cur_score;
  vector<PassedGridInfo> cur_opt;
  vector<Vec2> cur_grid;
  vector<Vec2> cur_grid_for_type_2;
  vector<Vec2> cur_grid_for_type_1;

  RouteCalGrid2Build(cur_grid);
  RouteCalGridForType2(cur_grid_for_type_2, cur_grid);

  cur_grid_for_type_1 = cur_grid;

  for (size_t i = 0; i < cur_grid_for_type_2.size(); ++i) {
    for (size_t j = 0; j < cur_grid_for_type_1.size(); ++j) {
      if (cur_grid_for_type_1[j] == cur_grid_for_type_2[i]) { 
	cur_grid_for_type_1.erase(cur_grid_for_type_1.begin() + j); 
	break; 
      }
    }
  }

  RouteCalOpt(cur_opt, cur_score, cur_grid_for_type_1);

  if (cur_score > best_score) {
    best_score = cur_score;
    grid2build = cur_grid;
    opt_grid = cur_opt;
    grid_for_type_1 = cur_grid_for_type_1;
    grid_for_type_2 = cur_grid_for_type_2;
    for (int i = 0; i < GOAL_CNT; ++i) used_mid[i] = mid[i];
  }
  
  return 0;
}

int BlockSolver::RouteCalOpt(vector<PassedGridInfo>& cur_opt, int &cur_score, const vector<Vec2> &grid)
{
  cur_score = 0;

  int cur_len = 0;
  memset(grid_rank, 0xff, sizeof(grid_rank));
  for (int l = GOAL_CNT-1; l >= 0; --l) {
    int path_len = goal_path[l].size();

    grid_rank[mid[l].x][mid[l].y] = cur_len++;
    for (int i = 0; i < path_len; ++i) {
      Vec2 &p = goal_path[l][i];
      grid_rank[p.x][p.y] = cur_len + i;
    }
    cur_len += path_len;
  }

  int avg_wait = 10;
  for (size_t i = 0; i < enemy_info.size(); ++i) 
    if (enemy_info[i].life > 1000)
      avg_wait = max(avg_wait, 101 / enemy_info[i].move_time);

  cur_opt.clear();
  for (size_t i = 0; i < grid.size(); ++i) {
    const Vec2 &p = grid[i];
    PassedGridInfo pp(p.x, p.y);
    pp.min_ti = INF;
    pp.max_ti = -1;
    int x, y;

    for (int l = 0; l < 8; ++l) {
      for (int det = 1; det <= 2; ++det) {
	if ((l&1) && det == 2) continue;
	x = p.x + dir[l][0] * det;
	y = p.y + dir[l][1] * det;
	if (0 <= x && x < H && 0 <= y && y < W && grid_rank[x][y] != -1) {
	  if (pp.min_ti > grid_rank[x][y]) {
	    pp.min_ti = grid_rank[x][y];
	    pp.min_idx = Vec2(x, y);
	  }
	  if (pp.max_ti < grid_rank[x][y]) {
	    pp.max_ti = grid_rank[x][y];
	    pp.max_idx = Vec2(x, y);
	  }
	}
      }
    }
    
    if (pp.min_ti != INF) {
      assert(pp.position != pp.min_idx);
      assert(pp.position != pp.max_idx);
      pp.max_can_recharge = (pp.max_ti - pp.min_ti)/10;
      pp.route_idx = pp.min_ti;
      cur_opt.push_back(pp);
      cur_score += Tower::POW(5, pp.max_can_recharge);
    }
  }

  sort(cur_opt.begin(), cur_opt.end());

  return 0;
}

int BlockSolver::RouteCalGridForType2(vector<Vec2> &gridtype2, const vector<Vec2>& grid)
{
  gridtype2.clear();

  vector<Vec2> cur_grid;
  for (int i = goal_path[0].size()-2; i >= 0; i -= 1) {
    Vec2 &p = goal_path[0][i];
    int cnt = 0;
    cur_grid.clear();
    
    for (size_t j = 0; j < grid.size(); ++j) {
      Tower tw(1, 2, grid[j].x, grid[j].y);
      if (tw.CheckInRange(p)) {
	cur_grid.push_back(tw.position);
	++cnt;
      }
      if (cnt >= 11) { grid_for_wait = p; break; }
    }
    if (cnt >= 11) { gridtype2.insert(gridtype2.end(), cur_grid.begin(), cur_grid.end()); break; }
  }
  
  return 0;
}

void BlockSolver::Debug()
{
  RouteClear();

  for (size_t i = 0; i < grid2build.size(); ++i) {
    const Vec2 &p = grid2build[i];
    GH.grid_info[p.x][p.y] = 't';
  }

  for (size_t i = 0; i < grid_for_type_2.size(); ++i) {
    const Vec2 &p = grid_for_type_2[i];
    GH.grid_info[p.x][p.y] = 'b';
  }

  
  GH.Debug();

  RouteClear();
}

int BlockSolver::TowerBestCheck(const vector<Tower>& tw, int life)
{
  int cost;
  vector<Tower> res;
  MatchChecker::Instance().Init(enemy_info, tw, life);
  MatchChecker::Instance().Run();

  if (MatchChecker::Instance().IsWin()) {
    CalCost(cost, res, tw);
    if (cost < min_cost) {
      min_cost = cost;
      best_tower2build = tw;
      return 1;
    }
  }

  return 0;
}

int BlockSolver::TowerTryEliminate(const vector<Tower>& tw)
{
  RouteClear();
  for (size_t i = 0; i < grid2build.size(); ++i) {
    Vec2 &p = grid2build[i];
    GH.grid_info[p.x][p.y] = 't';
  }

  vector<Tower> cur_tw = tw;

  for (int l = cur_tw.size()-1; l >= 0; --l) {
    Tower p = cur_tw[l];
    int x = p.position.x;
    int y = p.position.y;
    if (mp_tower[x][y] != -1) continue; 
      
    assert(GH.grid_info[x][y] == 't');
    GH.grid_info[x][y] = '0';
      
    cur_tw.erase(cur_tw.begin() + l);
    if (GH.CalAllEnemyMovePath() == 0) {
      MatchChecker::Instance().Init(enemy_info, cur_tw, 1);
      MatchChecker::Instance().Run();
      if (!MatchChecker::Instance().IsWin()) {
	cur_tw.push_back(p);
	assert(GH.grid_info[x][y] == '0');
	GH.grid_info[x][y] = 't';
      }
    }
  }

  for (int l = cur_tw.size()-1; l >= 0; --l) {
    Tower p = cur_tw[l];
    int x = p.position.x;
    int y = p.position.y;
    if (mp_tower[x][y] != -1) continue; 
      
    if (cur_tw[l].level == 0) continue;
    cur_tw[l].level -= 1;

    if (GH.CalAllEnemyMovePath() == 0) {
      MatchChecker::Instance().Init(enemy_info, cur_tw, 1);
      MatchChecker::Instance().Run();
      if (!MatchChecker::Instance().IsWin()) {
	cur_tw[l].level += 1;
      }
    }
  }
    
  vector<Tower> res;
  int cost;

  CalCost(cost, res, cur_tw);
  Output(res);
  
  return 0;
}

