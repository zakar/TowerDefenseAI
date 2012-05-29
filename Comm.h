#ifndef _COMM_
#define _COMM_

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib>
using namespace std;


const int MAXN = 51;
const int INF = 0x3f3f3f3f;
const int dir[8][2] = { {0,1}, {-1,1}, {-1,0}, {-1,-1},
			{0,-1}, {1,-1}, {1,0}, {1,1} };

struct Vec2
{
  Vec2(){}
  Vec2(int x, int y):x(x),y(y){}
  bool operator==(const Vec2& p) const;
  bool operator!=(const Vec2& p) const;
  bool operator<(const Vec2& p) const;
  Vec2 operator+(const Vec2& p) const;
  int LengthSq(const Vec2 p);
  void Debug() const;

  int x, y;
};

struct Tower
{
  Tower(){}
  Tower(int lev, int ty, int x, int y):level(lev),type(ty),position(x,y){}
  static int BuildCost(int level, int type);
  int BuildCost() const;
  int Attack() const;
  int Range() const;
  int ReCharge() const;
  int StopTime() const;
  int CheckInRange(const Vec2& p);
  int CheckDiff(const Tower& tw);
  
  void Print();

  static int POW(int x, int ti);

  int level, type;
  Vec2 position;
};

struct Enemy
{
  Enemy(){}
  Enemy(int T, int L, int S, int R, int x, int y):occur_time(T),life(L),move_time(S),
						  input_rank(R),position(x,y){}
  int occur_time, life, move_time;
  int input_rank;
  int occur_entry;
  Vec2 position;
};

struct GridHandler
{
  static GridHandler& Instance();
  void Init();
  vector<Vec2>& GetEmptyGrid();
  vector<Vec2>& GetEnemyEntry();
  vector<Vec2>& GetDefendEntry();

  static void GetUnion(vector<Vec2>& lhs, const vector<Vec2>& rhs);
  int CalMovePath(const Vec2& src, const vector<Vec2>& dst, vector<Vec2>& path, string& instruction, int det = 2);

  vector<Vec2>& GetEnemyMovePath(const Vec2& enemy_src);
  string GetEnemyMoveInstruction(const Vec2& enemy_src);
  int CalAllEnemyMovePath();

  void SetBlock(const vector<Vec2>& grid, char c);
  void SetBlock(const Vec2& grid, char c);
  void UnSetBlock(const vector<Vec2>& grid);
  void UnSetBlock(const Vec2& grid);

  int CalNearBlockable(const vector<Vec2>& near, vector<Vec2>& path);

  int CheckBlockable(int x, int y);
  int CheckPassable(int x, int y);
  int CheckBuildable(int x, int y);
  int CheckBlockable(const Vec2& grid);
  int CheckPassable(const Vec2& grid);
  int CheckBuildable(const Vec2& grid);
  

  void Debug();

  int W, H;
  char grid_info[MAXN][MAXN];

  vector<Vec2> enemy_entry;
  vector<Vec2> empty_entry;
  vector<Vec2> defend_entry;

  vector< vector<Vec2> > enemy_move_path;
  vector<string> enemy_instruction;
};

struct MatchChecker
{
  struct EnemyInfo {
    EnemyInfo(const Enemy& info, const vector<Vec2>& path, const string& instruction) {
      this->path = path;
      this->info = info;
      this->instruction = instruction;
      ins_len = instruction.size();
      cur_position = info.position;
      wait_time = info.occur_time;
      remain_life = info.life;
      ins_idx = -1;
    }
    int CalWaitTime();
    int Action();
    int InGoal();

    int wait_time;
    int remain_life;
    Enemy info;
    vector<Vec2> path;
    Vec2 cur_position;
    int ins_idx;
    string instruction;
    int ins_len;
  };

  struct TowerInfo
  {
    TowerInfo(const Tower& info, int enemy_size) {
      enemy_enter_time.clear();
      enemy_enter_time = vector<int>(enemy_size, INF);
      this->info = info;
      wait_time = 0;
      target = -1;
      attack_cnt = 0;
    }
    int FindTarget(const vector<EnemyInfo> &cur_enemy);

    Tower info;
    int wait_time;
    int target;
    int attack_cnt;
    vector<int> enemy_enter_time;
  };

  static MatchChecker &Instance();
  void Init(const vector<Enemy> & enemy, const vector<Tower> &tower, int player_life);
  void Run();
  int IsWin();

  int GetEnemyLifeStatu();

  int cur_time;
  int player_life;
  int remain_enemy;
  vector<EnemyInfo> cur_enemy;
  vector<TowerInfo> cur_tower;


  const vector<EnemyInfo>& GetEnemyInfo();
  const vector<TowerInfo>& GetTowerInfo();
};


#endif
