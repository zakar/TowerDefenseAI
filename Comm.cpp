#include "Comm.h"
#include <assert.h>

bool Vec2::operator==(const Vec2& p) const {
  return x == p.x && y == p.y;
}
bool Vec2::operator!=(const Vec2& p) const {
  return x != p.x || y != p.y;
}
bool Vec2::operator<(const Vec2& p) const {
  return x < p.x || (x == p.x && y < p.y);
}
Vec2 Vec2::operator+(const Vec2& p) const {
  return Vec2(x + p.x, y + p.y);
}
int Vec2::LengthSq(const Vec2 p) {
  return (p.x-x)*(p.x-x) + (p.y-y)*(p.y-y);
}
void Vec2::Debug() const {
  fprintf(stdout, "x:%d y:%d\n", x, y);
}


int Tower::POW(int x, int ti) {
  int res = 1;
  for(; ti; --ti) res *= x;
  return res;
}

int Tower::BuildCost(int level, int type) 
{
  int cost = 0;
  for (int lev = 0; lev <= level; ++lev) {
    cost += (type == 2) ? 20*(1+lev) : (10+5*type)*POW(3+type, lev);
  }
  return cost;
}

int Tower::BuildCost() const
{
  int cost = 0;
  for (int lev = 0; lev <= level; ++lev) {
    cost += (type == 2) ? 20*(1+lev) : (10+5*type)*POW(3+type, lev);
  }
  return cost;
}

int Tower::Attack() const
{
  if (type == 0) return 10;
  if (type == 1) return 20*POW(5, level);
  return 3*(level+1);
}

int Tower::Range() const
{
  if (type == 0) return 3+level;
  if (type == 1) return 2;
  return 2+level;
}

int Tower::ReCharge() const
{
  if (type == 0) return 10-2*level;
  if (type == 1) return 100;
  return 20;
}

int Tower::StopTime() const
{
  if (type == 2) return 2;
  return 0;
}

int Tower::CheckInRange(const Vec2& p)
{
  int range = Range();
  return range * range >= position.LengthSq(p);
}

int Tower::CheckDiff(const Tower& tw)
{
  if (type != tw.type) return 1;
  if (level < tw.level) return 1;
  return 0;
}

void Tower::Print()
{
  printf("%d %d %d %d\n", position.y, position.x, level, type);
}


GridHandler& GridHandler::Instance()
{
  static GridHandler ins;
  return ins;
}

void GridHandler::Init()
{
  fscanf(stdin, "%d%d", &W, &H);
  for (int i = 0; i < H; ++i) fscanf(stdin, "%s", grid_info[i]);

  empty_entry.clear();
  enemy_entry.clear();
  defend_entry.clear();
  for (int i = 0; i < H; ++i) {
    for (int j = 0; j < W; ++j) {
      if (grid_info[i][j] == 's') { enemy_entry.push_back(Vec2(i, j)); grid_info[i][j] = '0'; }
      if (grid_info[i][j] == 'g') { defend_entry.push_back(Vec2(i, j)); grid_info[i][j] = '0'; }
      if (grid_info[i][j] == '0') { empty_entry.push_back(Vec2(i, j)); grid_info[i][j] = '0'; }
    }
  }

  enemy_move_path.clear();
  enemy_instruction.clear();
  for (size_t i = 0; i < enemy_entry.size(); ++i) {
    enemy_move_path.push_back(vector<Vec2>());
    enemy_instruction.push_back(string());
  }
}

int GridHandler::CheckPassable(int x, int y) {
  return 0 <= x && x < H && 0 <= y && y < W && grid_info[x][y] == '0';
}

int GridHandler::CheckBuildable(int x, int y) {

  Vec2 p = Vec2(x, y);
  for (size_t i = 0; i < defend_entry.size(); ++i)
    if (p == defend_entry[i]) return 0;

  for (size_t i = 0; i < enemy_entry.size(); ++i)
    if (p == enemy_entry[i]) return 0;

  return 0 <= x && x < H && 0 <= y && y < W && grid_info[x][y] == '0';
}

int GridHandler::CheckPassable(const Vec2& grid)
{
  return CheckPassable(grid.x, grid.y);
}

int GridHandler::CheckBuildable(const Vec2& grid)
{
  return CheckBuildable(grid.x, grid.y);
}

int GridHandler::CheckBlockable(int x, int y)
{
  Vec2 p = Vec2(x, y);
  for (size_t i = 0; i < defend_entry.size(); ++i)
    if (p == defend_entry[i]) return 0;

  for (size_t i = 0; i < enemy_entry.size(); ++i)
    if (p == enemy_entry[i]) return 0;

  return 0 <= x && x < H && 0 <= y && y < W && grid_info[x][y] != '1';
}

int GridHandler::CheckBlockable(const Vec2& grid)
{
  Vec2 p = grid;
  for (size_t i = 0; i < defend_entry.size(); ++i)
    if (p == defend_entry[i]) return 0;

  for (size_t i = 0; i < enemy_entry.size(); ++i)
    if (p == enemy_entry[i]) return 0;

  return 0 <= p.x && p.x < H && 0 <= p.y && p.y < W && grid_info[p.x][p.y] != '1';
}

vector<Vec2>& GridHandler::GetDefendEntry()
{
  return defend_entry;
}

vector<Vec2>& GridHandler::GetEmptyGrid()
{
  return empty_entry;
}

vector<Vec2>& GridHandler::GetEnemyEntry()
{
  return enemy_entry;
}

void GridHandler::GetUnion(vector<Vec2>& lhs, const vector<Vec2>& rhs)
{
  lhs.insert(lhs.end(), rhs.begin(), rhs.end());
  sort(lhs.begin(), lhs.end());
  int tmp = unique(lhs.begin(), lhs.end()) - lhs.begin();
  lhs.erase(lhs.begin()+tmp, lhs.end());
}

int GridHandler::CalNearBlockable(const vector<Vec2>& near, vector<Vec2>& path)
{
  path.clear();
  int x, y;
  for (size_t i = 0; i < near.size(); ++i) {
    for (int l = 0; l < 8; l += 2) {
      x = near[i].x + dir[l][0];
      y = near[i].y + dir[l][1];
      if (CheckBlockable(x, y)) {
	path.push_back(Vec2(x, y));
      }
    }
  }

  sort(path.begin(), path.end());
  int tmp = unique(path.begin(), path.end()) - path.begin();
  path.erase(path.begin()+tmp, path.end());

  return 0;
}

int GridHandler::CalMovePath(const Vec2& src, const vector<Vec2>& dst, 
			     vector<Vec2>& path, string &instruction, int det)
{
  static int dist[MAXN][MAXN];
  static int vis[MAXN][MAXN];
  static Vec2 pre[MAXN][MAXN];
  static Vec2 que[100*MAXN*MAXN];
  memset(vis, 0, sizeof(vis));
  memset(dist, 0x3f, sizeof(dist));

  dist[src.x][src.y] = 0;
  vis[src.x][src.y] = 1;
  int st = 0, ed = 1;
  que[st] = src;

  Vec2 p;
  int x, y, x1, y1, x2, y2;
  for (; st < ed; ++st) {
    p = que[st];
    for (int l = 0; l < 8; l+=det) { 
      x2 = p.x;
      y1 = p.y;
      x = x1 = p.x + dir[l][0];
      y = y2 = p.y + dir[l][1];

      if (CheckPassable(x, y) && CheckPassable(x1, y1) && CheckPassable(x2, y2)) {
	if (dist[x][y] > dist[p.x][p.y] + 10 + 4*(l&1)) {
	  dist[x][y] = dist[p.x][p.y] + 10 + 4*(l&1);
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

  int min_cost = INF;
  for (size_t i = 0; i < dst.size(); ++i) {
    p = dst[i];
    min_cost = min(min_cost, dist[p.x][p.y]);
  }

  path.clear();
  instruction.clear();
  if (min_cost == INF) return -1;

  vector<Vec2> cur_path;
  string cur_path_str, best_path_str = "9";
  int cur_path_len;
  for (size_t i = 0; i < dst.size(); ++i) {
    p = dst[i];
    if (dist[p.x][p.y] > min_cost) continue;

    cur_path.clear();
    for (; p != src; p = pre[p.x][p.y]) {
      cur_path.push_back(p);
    }
    cur_path.push_back(p);
    reverse(cur_path.begin(), cur_path.end());
    cur_path_len = cur_path.size();

    cur_path_str.clear();
    for (int j = 1; j < cur_path_len; ++j) {
      for (int l = 0; l < 8; ++l) {
	if (cur_path[j-1] + Vec2(dir[l][0], dir[l][1]) == cur_path[j]) {
	  if (l&1) {
	    cur_path.push_back(cur_path[j-1] + Vec2(dir[l][0], 0));
	    cur_path.push_back(cur_path[j-1] + Vec2(0, dir[l][1]));
	  }
	  cur_path_str += '0' + l;
	  break;
	}
      }
    }
    if (cur_path_str < best_path_str) {
      best_path_str = cur_path_str;
      path = cur_path;
      instruction = best_path_str;
    }
  }

  return 0;
}

int GridHandler::CalAllEnemyMovePath()
{
  int flag = 0;
  for (size_t i = 0; i < enemy_entry.size(); ++i) {
    enemy_move_path[i].clear();
    enemy_instruction[i].clear();
    if (CalMovePath(enemy_entry[i], defend_entry, enemy_move_path[i], enemy_instruction[i], 1) == -1) flag = -1;
  }
  return flag;
}

vector<Vec2>& GridHandler::GetEnemyMovePath(const Vec2& enemy_src)
{
  for (size_t i = 0; i < enemy_entry.size(); ++i)
    if (enemy_entry[i] == enemy_src) return enemy_move_path[i];
  assert(0);
}

string GridHandler::GetEnemyMoveInstruction(const Vec2& enemy_src)
{
  for (size_t i = 0; i < enemy_entry.size(); ++i)
    if (enemy_entry[i] == enemy_src) return enemy_instruction[i];
  assert(0);
}

void GridHandler::SetBlock(const vector<Vec2>& grid, char c)
{
  Vec2 p;
  for (size_t i = 0; i < grid.size(); ++i) {
    p = grid[i];
    grid_info[p.x][p.y] = c;
  }
}

void GridHandler::SetBlock(const Vec2& grid, char c)
{
  grid_info[grid.x][grid.y] = c;
}

void GridHandler::UnSetBlock(const vector<Vec2>& grid)
{
  Vec2 p;
  for (size_t i = 0; i < grid.size(); ++i) {
    p = grid[i];
    grid_info[p.x][p.y] = '0';
  }
}

void GridHandler::UnSetBlock(const Vec2& grid)
{
  grid_info[grid.x][grid.y] = '0';
}

void GridHandler::Debug()
{
  for (int i = 0; i < H; ++i) puts(grid_info[i]);
  puts("");
}

int MatchChecker::TowerInfo::FindTarget(const vector<EnemyInfo> &cur_enemy) {
  target = -1;
  int in_time = INF, oc_time = INF, ip_time = INF;
  for (size_t j = 0; j < cur_enemy.size(); ++j) {
    if (cur_enemy[j].remain_life <= 0 || cur_enemy[j].ins_idx == -1 || cur_enemy[j].wait_time == INF) continue;
    if (enemy_enter_time[j] == INF) continue;

    assert(cur_enemy[j].remain_life > 0);

    if (enemy_enter_time[j] < in_time || 
	(enemy_enter_time[j] == in_time  && cur_enemy[j].info.occur_time < oc_time) ||
	(enemy_enter_time[j] == in_time  && cur_enemy[j].info.occur_time == oc_time && cur_enemy[j].info.input_rank < ip_time)) {
      in_time = enemy_enter_time[j];
      oc_time = cur_enemy[j].info.occur_time;
      ip_time = cur_enemy[j].info.input_rank;
      target = j;
    }
  }

  return target != -1;
}


int MatchChecker::EnemyInfo::CalWaitTime()
{
  if (ins_idx == ins_len) return 0;
  int fa = instruction[ins_idx] - '0';
  if (fa&1) 
    return ((int)(info.move_time * 14 / 10)); //!!!!!
  else
    return info.move_time;
}

int MatchChecker::EnemyInfo::Action()
{
  if (-1 < ins_idx && ins_idx < ins_len) {
    int fa = instruction[ins_idx] - '0';
    cur_position.x += dir[fa][0];
    cur_position.y += dir[fa][1];
  } 
  ins_idx += 1;
  return 0;
}

int MatchChecker::EnemyInfo::InGoal()
{
  assert(ins_idx <= ins_len);
  return ins_idx == ins_len;
}

MatchChecker &MatchChecker::Instance()
{
  static MatchChecker ins;
  return ins;
}

void MatchChecker::Init(const vector<Enemy> &enemy, const vector<Tower> &tower, int player_life)
{
  cur_enemy.clear();
  cur_tower.clear();
  cur_time = 0;
  this->player_life = player_life;
  remain_enemy = (int)enemy.size();
  
  for (size_t i = 0; i < enemy.size(); ++i) 
    cur_enemy.push_back(EnemyInfo(enemy[i], 
				  GridHandler::Instance().GetEnemyMovePath(enemy[i].position), 
				  GridHandler::Instance().GetEnemyMoveInstruction(enemy[i].position)));
  for (size_t i = 0; i < tower.size(); ++i) {
    cur_tower.push_back(TowerInfo(tower[i], remain_enemy));
  }
}

void MatchChecker::Run()
{
  while (player_life > 0 && remain_enemy > 0) {
    cur_time += 1;

    for (size_t i = 0; i < cur_enemy.size(); ++i) {
      if (cur_enemy[i].remain_life <= 0 || cur_enemy[i].wait_time == INF) continue;
      cur_enemy[i].wait_time -= 1;

      if (cur_enemy[i].wait_time) continue;
      //if (cur_enemy[i].wait_time == 0) {  //!!!!!!!!!!!!!!!!!can't use this!!!!!!
	cur_enemy[i].Action();
	cur_enemy[i].wait_time = cur_enemy[i].CalWaitTime();
	//}

      for (size_t j = 0; j < cur_tower.size(); ++j) {
	if (cur_tower[j].info.CheckInRange(cur_enemy[i].cur_position)) {
	  if (cur_tower[j].enemy_enter_time[i] == INF)
	    cur_tower[j].enemy_enter_time[i] = cur_time;
	} else {
	  cur_tower[j].enemy_enter_time[i] = INF;
	}
      }
    }

    for (size_t i = 0; i < cur_tower.size(); ++i) cur_tower[i].FindTarget(cur_enemy);

    for (size_t i = 0; i < cur_tower.size(); ++i) {
      if (cur_tower[i].wait_time > 0) cur_tower[i].wait_time -= 1;
      if (cur_tower[i].wait_time) continue;

      int target = cur_tower[i].target;
      if (target != -1) {
	cur_tower[i].attack_cnt += 1;
	cur_tower[i].wait_time = cur_tower[i].info.ReCharge()+1;
	cur_enemy[target].remain_life -= cur_tower[i].info.Attack();
	cur_enemy[target].wait_time += cur_tower[i].info.StopTime();
      }
    }

    remain_enemy = 0;
    for (size_t i = 0; i < cur_enemy.size(); ++i) {
      if (cur_enemy[i].remain_life > 0 && 
	  cur_enemy[i].wait_time != INF && 
	  cur_enemy[i].InGoal()) {
	cur_enemy[i].wait_time = INF;
	--player_life;
      }
      if (cur_enemy[i].remain_life > 0 && cur_enemy[i].wait_time != INF) ++remain_enemy;
    }
  }
}

int MatchChecker::IsWin()
{
  return player_life > 0;
}

const vector<MatchChecker::EnemyInfo>& MatchChecker::GetEnemyInfo()
{
  return cur_enemy;
}

const vector<MatchChecker::TowerInfo>& MatchChecker::GetTowerInfo()
{
  return cur_tower;
}
