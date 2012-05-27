#ifndef _BLOCKSOLVER_
#define _BLOCKSOLVER_

#include "Comm.h"

struct BlockSolver
{
  struct PassedGridInfo
  {
  PassedGridInfo(int x, int y):position(x,y){}
    Vec2 position;
    int max_can_recharge;
    int route_idx;
    bool operator < (const PassedGridInfo &p) const
    {
      return max_can_recharge > p.max_can_recharge ||
	(max_can_recharge == p.max_can_recharge && route_idx > p.route_idx);
    }
  };


  BlockSolver();
  static BlockSolver& Instance();
  void Init();
  void Run();
  void CalRoute();
  void CalChoice();
  void CalCost(int &cost, vector<Tower> &res, const vector<Tower>& tower2build);
  void Output(vector<Tower>& res);

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
  int RouteAnalysis();
  int RouteCalOpt(vector<PassedGridInfo>& opt, const vector<Vec2>& path);

  int grid_rank[MAXN][MAXN];
  vector< vector<PassedGridInfo> > opt_grid;
  vector<Vec2> grid2build;

  vector<Tower> tower2build;

  int best_score;
  Vec2 up, down, left, right, mid;
  int mx, my;
  Vec2 enemy_dst;
  Vec2 goal_dst;

  void Debug();
  FILE *fd;
};

#endif
