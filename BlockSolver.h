#ifndef _BLOCKSOLVER_
#define _BLOCKSOLVER_

#include "Comm.h"

struct BlockSolver
{
  static BlockSolver& Instance();
  void Init();
  void Run();
  void CalRoute();

  int mp_tower[MAXN][MAXN];
  vector<Tower> tower_info;
  vector<Enemy> enemy_info;
  int player_life, money, init_tower_cnt, init_enemy_cnt;
  int W, H;

  vector<Vec2> enemy_entry;
  vector<Vec2> defend_entry;

  vector< vector<Vec2> > enemy_block_near;
  vector< vector<Vec2> > defend_block_near;
  
  vector<Vec2> goal_path;
  vector<Vec2> goal_near_block;

  vector< vector<Vec2> > enemy_path;
  vector<Vec2> enemy_all_path;
  vector<Vec2> enemy_all_block_near;


  int RouteClear();
  int RouteInit();
  int RouteIter();
  int RoutePreDo();
  int RouteAnalysis();

  struct Spot
  {
    Spot(){}
    Spot(int x, int y):position(x,y),score(0){}
    Vec2 position;
    int score;
    bool operator < (const Spot &p) const
    {
      return score > p.score;
    }
  };

  vector<Spot> grid2build;
  int best_score;

  vector<Tower> tower2build;
  
  Vec2 up, down, left, right, mid;
  int mx, my;
  Vec2 enemy_dst;
  Vec2 goal_dst;

  void Debug();
};

#endif
