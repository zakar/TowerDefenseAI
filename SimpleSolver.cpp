#include "SimpleSolver.h"

SimpleSolver& SimpleSolver::Instance()
{
  static SimpleSolver ins;
  return ins;
}

void SimpleSolver::Init()
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
}

void SimpleSolver::Run()
{
  int x, y, cost;
  vector<Tower> result;
  vector<SimpleSolver::Point> towers;
  vector<Vec2> passed_grid;

  GridHandler::Instance().CalAllEnemyMovePath();
  GridHandler::Instance().GetEnemyPassedGrid(enemy_info, passed_grid);
  GridHandler::Instance().SetEnemyBlock(passed_grid);

  for (int i = 0; i < passed_grid.size(); ++i) {
    for (int l = 0; l < 8; ++l) {
      x = passed_grid[i].x + dir[l][0];
      y = passed_grid[i].y + dir[l][1];
      if (GridHandler::Instance().CheckBuildable(x, y) && mp_tower[x][y] == -1) {
	mp_tower[x][y] = 0;
	towers.push_back(SimpleSolver::Point(x, y, INF));
      }
    }
  }
  for (int i = 0; i < GridHandler::Instance().H; ++i) {
    for (int j = 0; j < GridHandler::Instance().W; ++j) {
      if (GridHandler::Instance().CheckBuildable(i, j) && mp_tower[i][j] == -1) {
	mp_tower[i][j] = 0;
	towers.push_back(SimpleSolver::Point(i, j, INF));
      }
    }
  }
  
  // vector<Vec2> &enemy_pos = GridHandler::Instance().GetEnemyEntry();
  // vector<Vec2> &defend_pos = GridHandler::Instance().GetDefendEntry();
  
  for (int l = 0; l < towers.size(); ++l) {
    for (int i = 0; i < passed_grid.size(); ++i)
      towers[l].score = min(towers[l].score, passed_grid[i].LengthSq(Vec2(towers[l].x, towers[l].y)));
  }

  sort(towers.begin(), towers.end());

  int rk[3];
  for (int limit = 1; limit <= towers.size(); limit += 1) {
    rk[0] = 0; rk[1] = 2; rk[2] = 1;
    for (int l = 0; l < 12; ++l) {
      for (int l0 = 0; l0 <= 4; ++l0)
	for (int l1 = 0; l1 <= 4; ++l1)
	  for (int l2 = 0; l2 <= 4; ++l2) {
	    result.clear();
	    for (int i = 0; i < limit; ++i) {
	      if (towers[i].score <= 1) {
		result.push_back(Tower(l0, rk[0], towers[i].x, towers[i].y));
		continue;
	      } 
	      if (towers[i].score <= 10) {
		result.push_back(Tower(l1, rk[1], towers[i].x, towers[i].y));
		continue;
	      }
	      if (towers[i].score <= 100) {
		result.push_back(Tower(l2, rk[2], towers[i].x, towers[i].y));
	      }
	    }
	    result.insert(result.end(), tower_info.begin(), tower_info.end());
	    
	    MatchChecker::Instance().Init(enemy_info, result, player_life);
	    MatchChecker::Instance().Run();
	    if (MatchChecker::Instance().IsWin()) goto label;
	  }
      swap(rk[rand()%3], rk[rand()%3]);
    }
  }

 label:
  if (!MatchChecker::Instance().IsWin()) while(1);

  GridHandler::Instance().SetTowerBlock(result);

  //  GridHandler::Instance().Debug();
  // printf("remain_money %d\n", money);

  GridHandler::Instance().UnSetEnemyBlock(passed_grid);
  GridHandler::Instance().UnSetTowerBlock(result);

  fprintf(stdout, "%d\n", result.size());
  for (int i = 0; i < result.size(); ++i) result[i].Print();
  fflush(stdout);
}
