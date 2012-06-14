#include "GA_BuildTower.h"
#include <assert.h>

GA_BuildTower& GA_BuildTower::Instance()
{
  static GA_BuildTower ins;
  return ins;
}

void GA_BuildTower::Init(const vector<Vec2>& cand_grid, 
			 const vector<int>& level,
			 const vector<int>& type)

{
  populations.clear();
  GenoType species;
  for (int i = 0; i < pop_size; ++i) {
    species.fitness = 0;
    species.rfitness = 0;
    species.cfitness = 0;
    species.gene.clear();
    for (int j = 0; j < gene_cnt; ++j) species.gene.push_back(Tower(level[j], type[j], 
								    cand_grid[j].x, cand_grid[j].y));
    populations.push_back(species);
  }

  CalFitness();

  double best_fitness = 0;
  for (int i = 0; i < pop_size; ++i) 
    if (populations[i].fitness > best_fitness) {
      best_fitness = populations[i].fitness;
      best = populations[i];
    }
}

void GA_BuildTower::CalFitness()
{
  for (int i = 0; i < pop_size; ++i) {
    MatchChecker::Instance().Init(enemy, populations[i].gene, 1);
    MatchChecker::Instance().Run();
    const vector<MatchChecker::EnemyInfo>& enemy_info = MatchChecker::Instance().GetEnemyInfo();
    
    double orig_sum = 0, positive_sum = 0, negative_sum = 0;

    int e_size = enemy_info.size();
    for (int j = 0; j < e_size; ++j) {
      orig_sum += enemy_info[j].info.life;

      if (enemy_info[j].remain_life > 0)
	positive_sum += enemy_info[j].remain_life;
      else
	negative_sum += enemy_info[i].remain_life;

      populations[i].fitness = orig_sum - positive_sum - fabs(negative_sum)/e_size;
      if (populations[i].fitness < 1) populations[i].fitness = 1;
    }
  }
}

void GA_BuildTower::Elitist()
{
  double max_fitness, min_fitness;
  int w_idx, b_idx;

  max_fitness = min_fitness = populations[0].fitness;
  w_idx = b_idx = 0;
  for (int i = 1; i < pop_size; ++i) {
    if (max_fitness < populations[i].fitness) {
      max_fitness = populations[i].fitness;
      b_idx = i;
    }
    if (min_fitness > populations[i].fitness) {
      min_fitness = populations[i].fitness;
      w_idx = i;
    }
  }

  if (max_fitness >= best.fitness) {
    best = populations[b_idx];
  } else {
    populations[w_idx] = best;
  }
}

void GA_BuildTower::Select()
{
  vector<GenoType> new_pop;
  double sum = 0;

  for (int i = 0; i < pop_size; ++i) 
    sum += populations[i].fitness;
  for (int i = 0; i < pop_size; ++i) 
    populations[i].rfitness = populations[i].fitness / sum;
  
  populations[0].cfitness = populations[0].rfitness;
  for (int i = 1; i < pop_size; ++i)
    populations[i].cfitness = populations[i-1].cfitness + populations[i].rfitness;

  new_pop.clear();
  for (int i = 0; i < pop_size; ++i) {
    double p = rand()%1000/1000.0;
    if (p < populations[0].cfitness) {
      new_pop.push_back(populations[0]);
    } else if (populations[pop_size-1].cfitness <= p) {
      new_pop.push_back(best);
    } else {
      for (int j = 1; j < pop_size; ++j)
	if (populations[j-1].cfitness <= p && p < populations[j].cfitness) {
	  new_pop.push_back(populations[j]);
	  break;
	}
    }
  }
  
  assert((int)new_pop.size() == pop_size);
  populations = new_pop;
}

void GA_BuildTower::Mutate()
{
  for (int i = 0; i < pop_size; ++i) {
    for (int j = 0; j < gene_cnt; ++j) {
      double p = rand()%1000/1000.0;
      if (p < PMUTATION) {
	if (populations[i].gene[j].level < 4) populations[i].gene[j].level += 1;
      }
    }
  }
}

void GA_BuildTower::Xover(int idx0, int idx1)
{
  int p0 = rand()%gene_cnt;
  int p1 = rand()%gene_cnt;
  
  swap(populations[idx0].gene[p0].position, populations[idx1].gene[p1].position);
}

void GA_BuildTower::Crossover()
{
  int pre = -1;
  for (int i = 0; i < pop_size; ++i) {
    double p = rand()%1000/1000.0;
    if (p < PXOVER) {
      if (pre == -1) 
	pre = i;
      else {
	Xover(pre, i);
	pre = -1;
      }
    }
  }
}

void GA_BuildTower::Run()
{
  for (int i = 0; i < generation_cnt; ++i) {
    Select();
    Crossover();
    Mutate();
    CalFitness();
    Elitist();
  }
}

void GA_BuildTower::SetArgs(int pop_size, int gene_cnt, int generation_cnt, const vector<Enemy>& enemy)
{
  this->enemy = enemy;
  this->pop_size = pop_size;
  this->generation_cnt = generation_cnt;
  this->gene_cnt = gene_cnt;
}

const vector<Tower>& GA_BuildTower::GetResult()
{
  return best.gene;
}
 
