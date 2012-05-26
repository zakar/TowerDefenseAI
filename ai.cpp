#include "BlockSolver.h"
#include "Comm.h"

int main()
{
  int S, L;
  fscanf(stdin, "%d", &S);
  for (int l = 0; l < S; ++l) {
    GridHandler::Instance().Init();
    fscanf(stdin, "%d", &L);
    fscanf(stdin, " END");
    for (int k = 0; k < L; ++k) {
      BlockSolver::Instance().Init();
      BlockSolver::Instance().Run();
    }
  }
  return 0;
}
