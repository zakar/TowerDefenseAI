#ifndef _GA_BUILDTOWER_
#define _GA_BUILDTOWER_

#include "Comm.h"

struct GA_BuildTower
{
  static GA_BuildTower& Instance();

  struct GenoType
  {
    double fitness;
    double rfitness;
    double cfitness;
    vector<Tower> gene;
  };
  
  vector<GenoType> populations;
  GenoType best;

  void Init(const vector<Vec2>& cand_grid,
	    const vector<int>& level, 
	    const vector<int>& type);
  void SetArgs(int pop_size, int gene_cnt, int generation_cnt, const vector<Enemy>& enemy);
  void CalFitness();
  void Elitist();
  void Select();
  void Crossover();
  void Xover(int idx0, int idx1);
  void Mutate();
  void Run();
  const vector<Tower>& GetResult();

  int pop_size;
  int gene_cnt;
  int generation_cnt;
  vector<Enemy> enemy;

  static const double PMUTATION = 0.1;
  static const double PXOVER = 0.1;
  
};

#endif
