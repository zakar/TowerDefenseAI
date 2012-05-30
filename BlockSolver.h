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

    //for debug
    Vec2 min_idx, max_idx;
    int min_ti, max_ti;

    bool operator < (const PassedGridInfo &p) const
    {
      return max_can_recharge > p.max_can_recharge ||
	(max_can_recharge == p.max_can_recharge && route_idx > p.route_idx);
    }
  };


  BlockSolver();
  static BlockSolver& Instance();
  void Init(int stage_ver, int stage_lev);
  void Run();
  void CalRoute();
  void CalChoice();
  void CalCost(int &cost, vector<Tower> &res, const vector<Tower>& tower2build);
  void Output(const vector<Tower>& res);

  int mp_tower[MAXN][MAXN];
  vector<Tower> tower_info;
  vector<Enemy> enemy_info;
  int player_life, money, init_tower_cnt, init_enemy_cnt;
  int W, H;

  const static int GOAL_CNT = 2;

  vector<Vec2> enemy_entry;
  vector<Vec2> defend_entry;

  vector< vector<Vec2> > enemy_block_near;
  vector< vector<Vec2> > defend_block_near;
  
  vector<Vec2> goal_path[GOAL_CNT];
  vector<Vec2> goal_near_block[GOAL_CNT];

  vector< vector<Vec2> > enemy_path;
  vector<Vec2> enemy_all_path;
  vector<Vec2> enemy_all_block_near;

  int TowerBestCheck(const vector<Tower>& tw, int life);
  int TowerTryEliminate(const vector<Tower>& tw);

  int RouteClear();
  int RouteInit();
  int RouteIter();
  int RouteAnalysis();
  int RouteCalOpt(vector<PassedGridInfo>& cur_opt, int &cur_score, const vector<Vec2> &grid);
  int RouteCalGridForType2(vector<Vec2> &gridtype2, const vector<Vec2>& grid);
  int RouteCalGrid2Build(vector<Vec2>& grid);

  int grid_rank[MAXN][MAXN];
  vector<PassedGridInfo> opt_grid;
  vector<Vec2> grid2build;
  vector<Vec2> grid_for_type_1;
  vector<Vec2> grid_for_type_2;

  vector<Tower> best_tower2build;

  int best_score;
  int min_cost;
  int stage_ver, stage_lev;
  int iter_count[GOAL_CNT];
  Vec2 up[GOAL_CNT], down[GOAL_CNT], left[GOAL_CNT], right[GOAL_CNT], mid[GOAL_CNT];
  int mx[GOAL_CNT], my[GOAL_CNT];
  Vec2 enemy_dst[GOAL_CNT];
  Vec2 goal_dst[GOAL_CNT];

  Vec2 grid_for_wait;
  Vec2 used_mid[GOAL_CNT];

  void Debug();
  FILE *fd;
};

#endif
