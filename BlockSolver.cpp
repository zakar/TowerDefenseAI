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

  enemy_entry = GridHandler::Instance().GetEnemyEntry();
  defend_entry = GridHandler::Instance().GetDefendEntry();

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

  GH.SetBlock(enemy_entry, 'e');
  GH.SetBlock(defend_entry, 'd');

  for (int i = 0; i < enemy_entry.size(); ++i) {
    GH.CalNearBlockable(vector<Vec2>(1, enemy_entry[i]), enemy_block_near[i]);
  }

  for (int i = 0; i < defend_entry.size(); ++i) {
    GH.CalNearBlockable(vector<Vec2>(1, defend_entry[i]), defend_block_near[i]);
  }

  GH.UnSetBlock(enemy_entry);
  GH.UnSetBlock(defend_entry);
}

void BlockSolver::Run()
{
  if (init_tower_cnt == 0) {
    CalRoute();
    Debug();

    printf("%d\n", tower2build.size());

    Vec2 p;
    for (int i = 0; i < tower2build.size(); ++i) {
      p = tower2build[i].position;
      if (p == door_left || p == door_right) {
	printf("%d %d %d %d\n", p.y, p.x, 4, 1);
      } else {
	printf("%d %d %d %d\n", p.y, p.x, 0, 0);
      }
    }
  } else {
    puts("0");
  }
  
  fflush(stdout);
}

void BlockSolver::CalRoute()
{
  string tmp;

  best_score = 0;
  for (int l = 0; l < defend_entry.size(); ++l) {
    
    RouteInit();
  reenter:
    while (RouteIter()) {      

      for (int i = 0; i < defend_block_near.size(); ++i) GH.SetBlock(defend_block_near[i], 't');
      for (int i = 0; i < enemy_block_near.size(); ++i) GH.SetBlock(enemy_block_near[i], 't');
      GH.UnSetBlock(defend_block_near[l]);

      //      GH.Debug();

      if (GH.CalMovePath(goal_dst, vector<Vec2>(1, defend_entry[l]), goal_path, tmp)) goto reenter;
      GH.SetBlock(goal_path, 'e');
      for (int i = 0; i < enemy_entry.size(); ++i) GH.UnSetBlock(enemy_block_near[i]);
      GH.CalNearBlockable(goal_path, goal_near_block);
      GH.SetBlock(goal_near_block, 't');

      //      GH.Debug();

      enemy_all_path.clear();
      for (int i = 0; i < enemy_entry.size(); ++i) {
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
}

int BlockSolver::RouteInit()
{
  mx = 1;
  my = 1;
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
  if (goal_path.size() > best_score) {
    best_score = goal_path.size();
    door_left = left;
    door_right = right;

    tower2build.clear();
    for (int i = 0; i < H; ++i)
      for (int j = 0; j < W; ++j)
	if (GH.grid_info[i][j] == 't')
	  tower2build.push_back(Tower(0, 0, i, j));
  }
}

void BlockSolver::Debug()
{
  RouteClear();

  Vec2 p;
  for (int i = 0; i < tower2build.size(); ++i) {
    p = tower2build[i].position;
    GH.grid_info[p.x][p.y] = 't';
  }

  printf("left %d %d\n", door_left.x, door_left.y);
  printf("right %d %d\n", door_right.x, door_right.y);

  GH.Debug();

  RouteClear();
}
