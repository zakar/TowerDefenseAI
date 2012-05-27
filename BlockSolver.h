#ifndef _BLOCKSOLVER_
#define _BLOCKSOLVER_

#include "Comm.h"

struct BlockSolver
{
  static BlockSolver& Instance();
  BlockSolver();
  void Init();
  void Run();
  void CalRoute();
  void CalChoice();
  void CalCost(int &cost, vector<Tower> &res, vector<Tower>& tower2build);
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
  int RoutePreDo();
  int RouteAnalysis();

  struct PassedGridInfo
  {
  PassedGridInfo(int x, int y):position(x,y){}
    Vec2 position;
    long long mask;
    int mask_cnt;
    int max_dist;
    bool operator < (const PassedGridInfo &p) const
    {
      return mask_cnt > p.mask_cnt || (mask_cnt == p.mask_cnt && max_dist > p.max_dist);
    }
  };

  int best_score;
  int grid_rank[MAXN][MAXN];
  int grid_mask[MAXN][MAXN];
  vector<PassedGridInfo> grid2build;
  vector<Tower> tower2build;
  
  Vec2 up, down, left, right, mid;
  int mx, my;
  Vec2 enemy_dst;
  Vec2 goal_dst;

  void Debug();
  FILE* fd;
};

#endif
