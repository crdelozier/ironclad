#include "aptr.hpp"
#include <cstdlib>
#include <iostream>

using namespace safe;

int main(int argc, char **argv){
  int N = atoi(argv[1]);
  /*
  aptr<int,1> A = new_array_1<int>(N);
  aptr<int,2> B = new_array_2<int>(N);
  aptr<int,3> C = new_array_3<int>(N);
  */
  aptr<int> A = new_array<int>(N);
  aptr<int> B = new_array<int>(N);
  aptr<int> C = new_array<int>(N);

  int sum = 0;
  
  for(int c = 0; c < N; c++){
    A[c] = rand() % 100;
    B[c] = rand() % 100;
  }

  for(int c = 0; c < N; c++){
    C[c] = A[c] + B[c];
  }

  for(int c = 0; c < N; c++){
    sum += C[c];
  }

  std::cout << sum << "\n";
  
  return 0;
}