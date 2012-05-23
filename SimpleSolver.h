#ifndef _SIMPLESOLVER_
#define _SIMPLESOLVER_

#include "Comm.h"

struct SimpleSolver
{
  struct Point
  {
    Point(){}
    Point(int x, int y, int s):x(x),y(y),score(s){}
    int x, y;
    int score;

    bool operator<(const Point &p) const
    {
      return score < p.score;
    }
  };

  static SimpleSolver& Instance();
  void Init();
  void Run();

  int mp_tower[MAXN][MAXN];
  vector<Tower> tower_info;
  vector<Enemy> enemy_info;
  int player_life, money, init_tower_cnt, init_enemy_cnt;
};

#endif
