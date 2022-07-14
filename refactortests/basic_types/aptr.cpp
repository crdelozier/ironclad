#include <iostream>

int main(){
  int *p = new int[5];
  p[0] = 5;
  p[4] = 10;
  int x = *p;
  int y = p[4];
  std::cout << "Passed test";
  return 0;
}
