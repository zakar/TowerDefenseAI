#include "Comm.h"

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

int Tower::RebuildCost(int lev, int ty)
{
  int old_cost = (ty == 2) ? 20*(1+lev) : (10+5*ty)*POW(3+ty, lev);
  int new_cost = (type == 2) ? 20*(1+level) : (10+5*type)*POW(3+type, level);

  if (ty != type) return new_cost;
  return new_cost - old_cost;
}

int Tower::BuildCost()
{
  int cost = 0;
  for (int lev = 0; lev <= level; ++lev) {
    cost += (type == 2) ? 20*(1+lev) : (10+5*type)*POW(3+type, lev);
  }
  return cost;
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
  return 2+level;
}

int Tower::ReCharge()
{
  if (type == 0) return 10-2*level;
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
  return range * range >= position.LengthSq(p);
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
  for (int i = 0; i < enemy_entry.size(); ++i) {
    enemy_move_path.push_back(vector<Vec2>());
    enemy_instruction.push_back(string());
  }
}

int GridHandler::CheckPassable(int x, int y) {
  return 0 <= x && x < H && 0 <= y && y < W && grid_info[x][y] == '0';
}

int GridHandler::CheckBuildable(int x, int y) {

  Vec2 p = Vec2(x, y);
  for (int i = 0; i < defend_entry.size(); ++i)
    if (p == defend_entry[i]) return 0;

  for (int i = 0; i < enemy_entry.size(); ++i)
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
  int x, y;
  for (int i = 0; i < near.size(); ++i) {
    for (int l = 0; l < 8; l += 2) {
      x = near[i].x + dir[l][0];
      y = near[i].y + dir[l][1];
      if (CheckBuildable(x, y)) {
	path.push_back(Vec2(x, y));
      }
    }
  }

  sort(path.begin(), path.end());
  int tmp = unique(path.begin(), path.end()) - path.begin();
  path.erase(path.begin()+tmp, path.end());
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
  for (int i = 0; i < dst.size(); ++i) {
    p = dst[i];
    min_cost = min(min_cost, dist[p.x][p.y]);
  }

  path.clear();
  instruction.clear();
  if (min_cost == INF) return -1;

  vector<Vec2> cur_path;
  string cur_path_str, best_path_str = "9";
  int cur_path_len;
  for (int i = 0; i < dst.size(); ++i) {
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
  for (int i = 0; i < enemy_entry.size(); ++i) {
    enemy_move_path[i].clear();
    enemy_instruction[i].clear();
    if (CalMovePath(enemy_entry[i], defend_entry, enemy_move_path[i], enemy_instruction[i], 1)) return -1;
  }
  return 0;
}

vector<Vec2>& GridHandler::GetEnemyMovePath(const Vec2& enemy_src)
{
  for (int i = 0; i < enemy_entry.size(); ++i)
    if (enemy_entry[i] == enemy_src) return enemy_move_path[i];
}

string GridHandler::GetEnemyMoveInstruction(const Vec2& enemy_src)
{
  for (int i = 0; i < enemy_entry.size(); ++i)
    if (enemy_entry[i] == enemy_src) return enemy_instruction[i];
}

void GridHandler::SetBlock(const vector<Vec2>& grid, char c)
{
  Vec2 p;
  for (int i = 0; i < grid.size(); ++i) {
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
  for (int i = 0; i < grid.size(); ++i) {
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
}

int MatchChecker::TowerInfo::FindTarget(vector<EnemyInfo> &cur_enemy) {
  target = NULL;
  int in_time = INF, oc_time = INF, ip_time = INF;
  for (int j = 0; j < cur_enemy.size(); ++j) {
    if (cur_enemy[j].remain_life <= 0 || cur_enemy[j].ins_idx == -1 || cur_enemy[j].wait_time == INF) continue;
    if (enemy_enter_time[j] == INF) continue;
    if (enemy_enter_time[j] < in_time || 
	(enemy_enter_time[j] == in_time  && cur_enemy[j].info.occur_time < oc_time) ||
	(enemy_enter_time[j] == in_time  && cur_enemy[j].info.occur_time == oc_time && cur_enemy[j].info.input_rank < ip_time)) {
      in_time = enemy_enter_time[j];
      oc_time = cur_enemy[j].info.occur_time;
      ip_time = cur_enemy[j].info.input_rank;
      target = &cur_enemy[j];
    }
  }

  return target != NULL;
}


int MatchChecker::EnemyInfo::CalWaitTime()
{
  int fa = instruction[ins_idx] - '0';
  if (fa&1) 
    return ((int)(info.move_time * 14 / 10)); //!!!!!
  else
    return info.move_time;
}

int MatchChecker::EnemyInfo::Action()
{
  if (-1 < ins_idx && ins_idx < instruction.size()) {
    int fa = instruction[ins_idx] - '0';
    cur_position.x += dir[fa][0];
    cur_position.y += dir[fa][1];
  } 
  ins_idx += 1;
  return 0;
}

int MatchChecker::EnemyInfo::InGoal()
{
  return ins_idx == instruction.size();
}

MatchChecker &MatchChecker::Instance()
{
  static MatchChecker ins;
  return ins;
}

void MatchChecker::Init(vector<Enemy> &enemy, vector<Tower> &tower, int player_life)
{
  cur_enemy.clear();
  cur_tower.clear();
  cur_time = 0;
  this->player_life = player_life;
  remain_enemy = enemy.size();
  
  for (int i = 0; i < enemy.size(); ++i) 
    cur_enemy.push_back(EnemyInfo(enemy[i], 
				  GridHandler::Instance().GetEnemyMovePath(enemy[i].position), 
				  GridHandler::Instance().GetEnemyMoveInstruction(enemy[i].position)));
  for (int i = 0; i < tower.size(); ++i)
    cur_tower.push_back(TowerInfo(tower[i], remain_enemy));
}

void MatchChecker::Run()
{
  int interval;
  while (player_life > 0 && remain_enemy > 0) {
    interval = INF;
    for (int i = 0; i < cur_enemy.size(); ++i) {
      if (cur_enemy[i].remain_life <= 0 || cur_enemy[i].wait_time == INF) continue;
      interval = min(interval, cur_enemy[i].wait_time);
    }
    for (int i = 0; i < cur_tower.size(); ++i) {
      if (cur_tower[i].wait_time != -1)
	interval = min(interval, cur_tower[i].wait_time);
    }

    cur_time += interval;

    for (int i = 0; i < cur_enemy.size(); ++i) {
      if (cur_enemy[i].remain_life <= 0 || cur_enemy[i].wait_time == INF) continue;
      cur_enemy[i].wait_time -= interval;
      if (cur_enemy[i].wait_time) continue;

      cur_enemy[i].Action();
      cur_enemy[i].wait_time = cur_enemy[i].CalWaitTime();

      for (int j = 0; j < cur_tower.size(); ++j) {
	if (cur_tower[j].info.CheckInRange(cur_enemy[i].cur_position)) {
	  if (cur_tower[j].enemy_enter_time[i] == INF)
	    cur_tower[j].enemy_enter_time[i] = cur_time;
	} else {
	  cur_tower[j].enemy_enter_time[i] = INF;
	}
      }
    }

    remain_enemy = 0;
    for (int i = 0; i < cur_enemy.size(); ++i) {
      if (cur_enemy[i].remain_life > 0 && 
	  cur_enemy[i].wait_time != INF && 
	  cur_enemy[i].InGoal()) {
	cur_enemy[i].wait_time = INF;
	--player_life;
      }
      if (cur_enemy[i].remain_life > 0 && cur_enemy[i].wait_time != INF) ++remain_enemy;
    }

    if (player_life <= 0) break;

    for (int i = 0; i < cur_tower.size(); ++i) cur_tower[i].FindTarget(cur_enemy);
    for (int i = 0; i < cur_tower.size(); ++i) {
      if (cur_tower[i].wait_time > 0) cur_tower[i].wait_time -= interval;
      if (cur_tower[i].wait_time > 0) continue;
      cur_tower[i].wait_time = -1;

      EnemyInfo *target = cur_tower[i].target;
      if (target) {
	cur_tower[i].wait_time = cur_tower[i].info.ReCharge()+1;
	target->remain_life -= cur_tower[i].info.Attack();
	target->wait_time += cur_tower[i].info.StopTime();

	// printf("attack-- cur_time:%d  enemy:%d-x:%d-y:%d-life:%d-wait:%d  tower:%d\n", 
	//        cur_time, target - &cur_enemy[0], target->cur_position.x, target->cur_position.y, target->remain_life, target->wait_time, i);
      }
    }

  }

  // printf("tower size: %d\n", cur_tower.size());
  // for (int i = 0; i < cur_enemy.size(); ++i) {
  //   printf("size %d %d position %d %d life %d\n", cur_enemy[i].ins_idx, cur_enemy[i].instruction.size(),
  // 	   cur_enemy[i].cur_position.x, cur_enemy[i].cur_position.y, cur_enemy[i].remain_life);
  // }
}

int MatchChecker::IsWin()
{
  return player_life > 0;
}
