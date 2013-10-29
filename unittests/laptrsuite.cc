#include <gtest/gtest.h>

#include <ironclad/ironclad.hpp>

using namespace safe;

TEST(LAPtrSuite, InitNull){
  laptr<int> p;

  EXPECT_EQ(p,nullptr);
}

TEST(LAPtrSuite, CopyNull){
  laptr<int> p;
  laptr<int> q(p);

  EXPECT_EQ(p,nullptr);
  EXPECT_EQ(q,nullptr);
}

TEST(LAPtrSuite, AssignNull){
  laptr<int> p;
  laptr<int> q;

  q = p;

  EXPECT_EQ(p,nullptr);
  EXPECT_EQ(q,nullptr);
}

TEST(LAPtrSuite, AssignArray){
  safe::array<int,5> a;
  laptr<int> l = a;
  EXPECT_TRUE(l != nullptr);
}
