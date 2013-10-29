#include <gtest/gtest.h>

#include <ironclad/ironclad.hpp>

using namespace safe;

TEST(APtrSuite, InitNull){
  aptr<int> p;

  EXPECT_EQ(p,nullptr);
}

TEST(APtrSuite, CopyNull){
  aptr<int> p;
  aptr<int> q(p);

  EXPECT_EQ(p,nullptr);
  EXPECT_EQ(q,nullptr);
}

TEST(APtrSuite, AssignNull){
  aptr<int> p;
  aptr<int> q;

  q = p;

  EXPECT_EQ(p,nullptr);
  EXPECT_EQ(q,nullptr);
}

TEST(APtrSuite, InitSize){
  aptr<int> a = new_array<int>(5);
  EXPECT_EQ(5,a.getSize());
  EXPECT_EQ(0,a.getIndex());
}

TEST(APtrSuite, PtrDiff){
  aptr<int> a = new_array<int>(5);
  EXPECT_EQ(2,(a+2) - a);
}

TEST(APtrSuite, ConvertToPtr){
  aptr<int> a = new_array<int>(5);
  a += 4;
  ptr<int> p = a;
  EXPECT_TRUE(p != nullptr);
}

TEST(APtrSuite, ConvertToLAPtr){
  aptr<int> a = new_array<int>(5);
  laptr<int> l = a;
  EXPECT_TRUE(l.notOnStack());
}
