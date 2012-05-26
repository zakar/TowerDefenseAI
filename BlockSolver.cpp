#include "BlockSolver.h"

#define GH GridHandler::Instance()

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

  enemy_block_near.clear();
  defend_block_near.clear();
  goal_path.clear();
  goal_near_block.clear();
  enemy_path.clear();
  enemy_all_path.clear();
  enemy_all_block_near.clear();

  for (int i = 0; i < enemy_entry.size(); ++i) {
    enemy_block_near.push_back(vector<Vec2>());
    enemy_path.push_back(vector<Vec2>());
  }
  for (int i = 0; i < defend_entry.size(); ++i) {
    defend_block_near.push_back(vector<Vec2>());
  }

  for (int i = 0; i < enemy_entry.size(); ++i) {
    GH.CalNearBlockable(vector<Vec2>(1, enemy_entry[i]), enemy_block_near[i]);
  }

  for (int i = 0; i < defend_entry.size(); ++i) {
    GH.CalNearBlockable(vector<Vec2>(1, defend_entry[i]), defend_block_near[i]);
  }

  enemy_entry = GridHandler::Instance().GetEnemyEntry();
  defend_entry = GridHandler::Instance().GetDefendEntry();
}

void BlockSolver::Run()
{
  if (init_tower_cnt == 0) {
    CalRoute();
  }

  Debug();

  // for (;;) {
  //   RouteAnalysis();
  //   MatchChecker::Instance().Init(enemy_info, tower2build, player_life);
  //   MatchChecker::Instance().Run();
  //   if (MatchChecker::Instance().IsWin()) break;
  // }
}

void BlockSolver::CalRoute()
{
  string tmp;

  best_score = 0;
  for (int l = 0; l < defend_entry.size(); ++l) {   
    RouteInit();

    while (RouteIter()) {      
      RouteClear();

      for (int i = 0; i < defend_block_near.size(); ++i) GH.SetBlock(defend_block_near[i], 't');
      for (int i = 0; i < enemy_block_near.size(); ++i) GH.SetBlock(enemy_block_near[i], 't');
      GH.UnSetBlock(defend_block_near[l]);

      GH.CalMovePath(goal_dst, vector<Vec2>(1, defend_entry[l]), goal_path, tmp);
      GH.CalNearBlockable(goal_path, goal_near_block);

      for (int i = 0; i < enemy_entry.size(); ++i) GH.UnSetBlock(enemy_block_near[i]);
      //顺序保证goal_path block成功
      GH.SetBlock(goal_path, 'E');
      GH.SetBlock(goal_near_block, 't');

      enemy_all_path.clear();
      for (int i = 0; i < enemy_entry.size(); ++i) {
	GH.CalMovePath(enemy_entry[i], vector<Vec2>(1, enemy_dst), enemy_path[i], tmp);
	GH.GetUnion(enemy_all_path, enemy_path[i]);
      }

      enemy_all_block_near.clear();
      GH.CalNearBlockable(enemy_all_path, enemy_all_block_near);
      GH.SetBlock(enemy_all_path, 'e');
      GH.SetBlock(enemy_all_block_near, 't');

      RouteAnalysis();
    }
  }
}

int BlockSolver::RouteClear()
{
  for (int i = 0; i < H; ++i)
    for (int j = 0; j < W; ++j) 
      if (GH.grid_info[i][j] != '1') GH.grid_info[i][j] = '0';
}

int BlockSolver::RouteInit()
{
  mx = 1;
  my = 1;
}

int BlockSolver::RouteIter()
{
  while (mx < H && my < W) {
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
      break;
    }

    ++my;
    if (my == W) { my = 1; ++mx; }
  }

  return 0;
}

int BlockSolver::RouteAnalysis()
{
  GH.UnSetBlock(mid);
  GH.UnSetBlock(left);
  GH.UnSetBlock(right);
  GH.SetBlock(mid, 'E');

  if (GH.CalAllEnemyMovePath()) return -1;

  int x, y;
  int cur_score = 0;
  vector<Spot> cur_build;

  for (int i = 0; i < H; ++i)
    for (int j = 0; j < W; ++j) {
      if (GH.grid_info[i][j] != 't') continue;
      Spot p = Spot(i, j);
      for (int l = 0; l < 8; ++l) {
	x = p.position.x + dir[l][0];
	y = p.position.y + dir[l][1];
	if (0 <= x && x < W && 0 <= y && y < H) {
	  if (GH.grid_info[x][y] == 'e') p.score += 1, cur_score += 1;
	  if (GH.grid_info[x][y] == 'E') p.score += 10, cur_score += 2;
	}
      }
      cur_build.push_back(p);
    }

  if (cur_score > best_score) {
    best_score = cur_score;
    grid2build = cur_build;
    grid2build.push_back(Spot(left.x, left.y));
    grid2build.push_back(Spot(right.x, right.y));
  }
}

void BlockSolver::Debug()
{
  RouteClear();
  Spot p;

  for (int i = 0; i < grid2build.size(); ++i) {
    p = grid2build[i];
    GH.grid_info[p.position.x][p.position.y] = 't';
  }
  GH.Debug();

  RouteClear();
}
