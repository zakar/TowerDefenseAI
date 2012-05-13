#include <cstdio>
#include <cstring>
#include <algorithm>
#include <vector>
#include <cmath>
using namespace std;

const int MAXN = 51;
const int INF = 0x3f3f3f3f;
const double FITNESS_INF = 1e50;

struct Vec2
{
  Vec2(){}
  Vec2(int x, int y):x(x),y(y){}
  bool operator==(const Vec2& p) const {
    return x == p.x && y == p.y;
  }
  bool operator<(const Vec2& p) const {
    return x < p.x || (x == p.x && y < p.y);
  }
  int LengthSq(const Vec2 p) {
    return (p.x-x)*(p.x-x) + (p.y-y)*(p.y-y);
  }
  int x, y;
};

struct Tower
{
  Tower(){}
  Tower(int lev, int ty, int x, int y):level(lev),type(ty),position(x,y){}
  int RequireCost(int lev, int ty);
  int Attack();
  int Range();
  int ReCharge();
  int StopTime();
  int CheckInRange(const Vec2& p);

  static int POW(int x, int ti) {
    int res = 1;
    for(; ti; --ti) res *= x;
    return res;
  }

  int level, type;
  Vec2 position;
};

struct Enemy
{
  Enemy(){}
  Enemy(int T, int L, int S, int x, int y):occur_time(T),life(L),move_time(S),position(x,y){}
  int occur_time, life, move_time;
  Vec2 position
};

struct Fitness
{
  int can_win, need_money;
  bool operator<(const Fitness& p) const {
    return can_win < p.can_win || (can_win == p.can_win && need_money > p.need_money);
  }
};

struct GeneProcess
{
  struct Gene
  {
    Gene(){}
    vector<Tower> population;
    Fitness fitness;
  };

  static GeneProcess& Instance();
  void Process();
  void OutputResult();

  void Init();
  void EvaluateFitness();
  void KeepTheBest();
  void Elitist();
  void Select();
  void Crossover();
  void Mutate();
 
  int max_generation;
  int current_generation;
  int population_cnt;
  
  vector<Gene> populations;
  Gene best_population;
};

struct FitnessHandler
{
  static FitnessHandler& Instance();
  void Init();
  vector<Tower>& GetOrigTowers();
  int GetNeedMoney(const vector<Tower>& towers);
  Fitness GetFitness(const vector<Tower>& towers);

  int L, M, T, E;
  int tower_info[MAXN][MAXN];
  vector<Tower> orig_towers;
  vector<Enemy> enemys;
};

struct GridHandler
{
  static GridHandler& Instance();
  void Init();
  vector<Vec2>& GetEmptyGrid();
  vector<Vec2>& GetEnemyEntry();
  void CalEnemyMovePath(const Vec2& enemy_src, vector<Vec2>& path);
  void CalEnemysMovePath();
  vector<Vec2>& GetEnemyMovePath(const Vec2& enemy_src);
  void SetTowerBlock(const vector<Tower>& towers);
  void UnSetTowerBlock(const vector<Tower>& towers);

  inline int CheckValid(int x, int y) {
    0 <= x && x < W && 0 <= y && y < H && grid_info[x][y] == '0';
  }

  int W, H;
  char grid_info[MAXN][MAXN];

  vector<Vec2> enemy_entry;
  vector< vector<Vec2> > enemy_move_path;
  vector<Vec2> empty_entry;
  vector<Vec2> defend_entry;
};

int Tower::RequireCost(int lev, int ty)
{
  int old_cost = ty == 2 ? 20*(1+lev) : (10+5*ty)*POW(3+ty, lev);
  int new_cost = type == 2 ? 20*(1+level) : (10+5*type)*POW(3+type, level);

  if (ty != type) return new_cost;
  return new_cost - old_cost;
}

int Tower::Attack()
{
  if (type == 0) return 10;
  if (type == 1) return 20*POW(5, level);
  return 3*(level+1);
}

int Tower::Range()
{
  if (type == 0) return 3+level;
  if (type == 1) return 2;
  return 2+leve;
}

int Tower::ReCharge()
{
  if (type == 0) return 10-level;
  if (type == 1) return 100;
  return 20;
}

int Tower::StopTime()
{
  if (type == 2) return 2;
  return 0;
}

int Tower::CheckInRange(const Vec2& p)
{
  int range = Range();
  return range * range >= p.LengthSq(position);
}

GeneProcess& GeneProcess::Instance()
{
  static GeneProcess ins;
  return ins;
}

void GeneProcess::Process()
{
  Init();
  Evaluation();
  KeepTheBest();
  for (; current_generation < max_generation; ++current_generation) {
    Select();
    Crossover();
    Mutate();
    Evaluation();
    Elitist();
  }
}

void GeneProcess::Init()
{
  GridHandler::Instance().SetTowerBlock(FitnessHandler::Instance().GetOrigTowers());
  GridHandler::Instance().CalEnemysMovePath();
  GridHandler::Instance().UnSetTowerBlock(FitnessHandler::Instance().GetOrigTowers());

  vector<Vec2> target;
  vector<Vec2> enemy_entry = GridHandler::Instance().GetEnemyEntry();
  for (int i = 0; i < enemy_entry.size(); ++i) {
    vector<Vec2>& cur = GridHandler::Instance().GetEnemyMovePath(enemy_entry[i]);
    target.insert(target.end(), cur.begin(), cur.end());
  } 
  sort(target.begin(), target.end());
  int tmp_size = unique(target.begin(), target.end()) - target.begin();
  target.erase(target.begin()+tmp_size, target.end());
  
  populations.clear();
  population_cnt = 10;
  for (int i = 0; i < population_cnt; ++i) {
    populations.push_back(Gene());
    Gene &gen = *(populations.end()-1);
    
    
  }
}

void GeneProcess::EvaluateFitness();
void GeneProcess::KeepTheBest();
void GeneProcess::Elitist();
void GeneProcess::Select();
void GeneProcess::Crossover();
void GeneProcess::Mutate();
void GeneProcess::OutputResult();
FitnessHandler& FitnessHandler::Instance()
{
  static FitnessHandler ins;
  return ins;
}

void FitnessHandler::Init()
{
  scanf("%d%d%d%d", &L, &M, &T, &E);
  int x, y, a, c, t, l, s;
  
  memset(tower_info, 0, sizeof(tower_info));
  orig_towers.clear();
  for (int i = 0; i < T; ++i) {
    scanf("%d%d%d%d", &x, &y, &a, &c);
    tower_info[x][y] = (a << 2) + c;
    orig_towers.push_back(Tower(a, c, x, y, a));
  }

  enemys.clear();
  for (int i = 0; i < E;  ++i) {
    scanf("%d%d%d%d%d", &x, &y, &t, &l, &s);
    enemys.push_back(Enemy(t, l, s, x, y));
  }
  
  scanf("END");
}

vector<Tower>& FitnessHandler::GetOrigTowers()
{
  return orig_towers;
}

Fitness FitnessHandler::GetFitness(const vector<Tower>& towers)
{
  // Fitness result;
  // result.need_money = GetNeedMoney(towers);
  // result.can_win = 1;

  // int life = M; 
  // int interval;
  // int tsize = towers.size();
  // int esize = enemy.size();
  // vector<int> remain_recharge;
  // vector<int> remain_movetime;
  // vector<int> enemy_path_idx;
  // vector<Enemy> remain_enemy;
  
  // for (int i = 0; i < tsize; ++i) {
  //   remain_recharge.push_back(towers[i].ReCharge());
  // }
  // for (int i = 0; i < esize; ++i) {
  //   remain_movetime.push_back(enemys[i].S);
  //   enemy_path_idx.push_back(0);
  //   remain_enemy.push_back(enemys[i]);
  // }

  // while (life > 0 && esize) {
  //   interval = INF;
  //   for (int i = 0; i < tsize; ++i) 
  //     interval = min(interval, remain_recharge[i]);
  //   for (int i = 0; i < esize; ++i) 
  //     interval = min(interval, remain_movetime[i]);

  //   for (int i = 0; i < tsize; ++i) 
  //     remain_recharge[i] -= interval;
  //   for (int i = 0; i < esize; ++i) 
  //     remain_movetime[i] -= interval;

  //   for (int i = 0; i < esize; ++i) {
  //     if (remain_movetime[i] > 0) continue;
  //     ++enemy_path_idx[i];
  //   }

  //   for (int i = 0; i < tsize; ++i) {
      
  //   }
    
  // }
  
  // return result;
}

int FitnessHandler::GetNeedMoney(const vector<Tower>& towers)
{
  int result = 0;
  int x, y;
  for (int i = 0; i < towers.size(); ++i) {
    x = towers[i].position.x;
    y = towers[i].position.y;
    result += towers[i].RequireCost(tower_info[x][y] >> 2, tower_info[x][y] & 3);
  }
  return result;
}

GridHandler& GridHandler::Instance()
{
  static GridHandler ins;
  return ins;
}

void GridHandler::InitGridInfo()
{
  scanf("%d%d", &W, &H);
  for (int i = 0; i < H; ++i) scanf("%s", grid_info[i]);

  empty_entry.clear();
  enemy_entry.clear();
  defend_entry.clear();
  for (int i = 0; i < H; ++i) {
    for (int j = 0; j < W; ++j) {
      if (grid_info[i][j] == 's') enemy_entry.push_back(Vec2(i, j));
      if (grid_info[i][j] == 'g') defend_entry.push_back(Vec2(i, j));
      if (grid_info[i][j] == '0') empty_entry.push_back(Vec2(i, j));
    }
  }
}

vector<Vec2>& GridHandler::GetEmptyGrid()
{
  return empty_entry;
}

 vector<Vec2>& GridHandler::GetEnemyEntry()
 {
   return enemy_entry;
 }

vector<Vec2>& GetEnemyMovePath(const Vec2& enemy_src)
{
  for (int i = 0; i < enemy_entry.size(); ++i) {
    if (enemy_src == enemy_entry[i]) return enemy_move_path[i];
  }
}

void GridHandler::CalEnemysMovePath()
{
  enemy_move_path.clear();
  for (int i = 0; i < enemy_entry.size(); ++i) {
    enemy_move_path.push_back(vector<Vec2>());
    CalEnemyMovePath(enemy_entry[i], enemy_move_path[i]);
  }
}


void GridHandler::CalEnemyMovePath(const Vec2& enemy_src, vector<Vec2>& path);
{
  static int dir[8][2] = { {0,1}, {-1,1}, {-1,0}, {-1,-1},
			   {-1,0}, {1,-1}, {-1,0}, {1,1} };
  static int dist[MAXN][MAXN];
  static int vis[MAXN][MAXN];
  static Vec2 pre[MAXN][MAXN];
  static Vec2 que[10*MAXN*MAXN];
  memset(vis, 0, sizeof(vis));
  memset(dist, 0x3f, sizeof(dist));

  dist[enemy_src.x][enemy_src.y] = 0;
  vis[enemy_src.x][enemy_src.y] = 1;
  int st = 0, ed = 1;
  que[st] = enemy_src;

  Vec2 p;
  int x, y, x1, y1, x2, y2;
  for (; st < ed; ++st) {
    src = que[st];
    for (int l = 0; l < 4; ++l) {
      x = x1 = x2 = p.x;
      y = y1 = y2 = p.y;
      x = x1 = p.x + dir[l][0];
      y = y2 = p.y + dir[l][1];

      if (CheckValid(x, y) && CheckValid(x1, y1) && CheckValid(x2, y2)) {
	if (dist[x][y] > dist[p.x][p.y] + 10) {
	  dist[x][y] = dist[p.x][p.y] + 10;
	  pre[x][y] = p;
	  if (!vis[x][y]) {
	    vis[x][y] = 1;
	    que[ed++] = Vec2(x,y);
	  }
	}
      }
    }
    vis[p.x][p.y] = 0;
  }

  memset(vis, 0, sizeof(vis));
  int flag = 0;
  int min_cost = INF;
  for (int i = 0; i < defend_entry.size(); ++i) {
    p = defend_entry[i];
    flag |= (dist[p.x][p.y] < INF);
    min_cost = min(min_cost, dist[p.x][p.y]);
  }

  path.clear();
  if (!flag) return enemy_move_path;

  for (int i = 0; i < defend_entry.size(); ++i) {
    p = defend_entry[i];
    if (dist[p.x][p.y] > min_cost) continue;
    vis[p.x][p.y] |= 2;
    for (; p != enemy_src; p = pre[p.x][p.y]) {
      vis[p.x][p.y] |= 1;
    }
  }

  enemy_move_path.push_back(enemy_src);
  x = enemy_src.x;
  y = enemy_src.y;
  while (vis[x][y] ^ 2) {
    path.push_back(Vec2(x,y));
    for (int l = 0; l < 8; ++l) {
      x1 = x + dir[l][0];
      y1 = y + dir[l][1];
      if (CheckValid(x1, y1) && vis[x1][y1]) {
	x = x1;
	y = y1;
	break;
      }
    }
  }
  path.push_back(Vec2(x,y));
  return path;
}

void GridHandler::SetTowerBlock(const vector<Tower>& towers);
{
  int x, y;
  for (int i = 0; i < towers.size(); ++i) {
    x = towers[i].position.x;
    y = towers[i].position.y;
    grid_info[x][y] = 't';
  }
}

void GridHandler::UnSetTowerBlock(const vector<Tower>& towers)
{
  int x, y;
  for (int i = 0; i < towers.size(); ++i) {
    x = towers[i].position.x;
    y = towers[i].position.y;
    grid_info[x][y] = '0';
  }
}

int main()
{
  int S, L;
  scanf("%d", &S);
  for (int l = 0; l < S; ++l) {
    GridHandler::Instance().Init();
    scanf("%d", &L);
    scanf("END");
    for (int k = 0; k < L; ++k) {
      FitnessHandler::Instance.Init();
      GeneProcess::Instance().Process();
      GeneProcess::Instance().OutputResult();
    }
  }
  return 0;
}
