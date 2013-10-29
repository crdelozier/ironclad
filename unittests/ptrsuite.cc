#include <gtest/gtest.h>

#include <ironclad/ptr.hpp>

using namespace safe;

TEST(PtrSuite, InitNull){
  ptr<int> p;

  EXPECT_EQ(p,nullptr);
}

TEST(PtrSuite, CopyNull){
  ptr<int> p;
  ptr<int> q(p);

  EXPECT_EQ(p,nullptr);
  EXPECT_EQ(q,nullptr);
}

TEST(PtrSuite, AssignNull){
  ptr<int> p;
  ptr<int> q;

  q = p;

  EXPECT_EQ(p,nullptr);
  EXPECT_EQ(q,nullptr);
}
