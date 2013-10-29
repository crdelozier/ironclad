#include <gtest/gtest.h>

#include <ironclad/lptr.hpp>

using namespace safe;

TEST(PtrSuite, InitNull){
  lptr<int> p;

  EXPECT_EQ(p,nullptr);
}

TEST(LPtrSuite, CopyNull){
  lptr<int> p;
  lptr<int> q(p);

  EXPECT_EQ(p,nullptr);
  EXPECT_EQ(q,nullptr);
}

TEST(LPtrSuite, AssignNull){
  lptr<int> p;
  lptr<int> q;

  q = p;

  EXPECT_EQ(p,nullptr);
  EXPECT_EQ(q,nullptr);
}

void f(lptr<int> & l){
  int f;
  EXPECT_FALSE(l.can_accept(&f));
}

void g(int x, lptr<int> & l){
  EXPECT_FALSE(l.can_accept(&x));
}

TEST(LPtrSuite, TestAssignGood1){
  int x = 0;
  lptr<int> l = &x;
}

TEST(LPtrSuite, TestAssignGood2){
  lptr<int> l;
  int y = 0;
  l = &y;
}

TEST(LPtrSuite, TestAssignBad1){
  lptr<int> l;
  f(l);
}

TEST(LPtrSuite, TestAssignBad2){
  int x = 0;
  lptr<int> l;
  g(x,l);
}
