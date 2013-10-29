#include <gtest/gtest.h>

#include <iostream>
#include <ironclad/ironclad.hpp>

using namespace safe;

TEST(CStringSuite, Strlen){
  aptr<char> s = WRAP_LITERAL("Test");

  EXPECT_EQ(safe_strlen(s),4);
}

TEST(CStringSuite, StrcmpSelf){
  aptr<char> s = WRAP_LITERAL("Test");

  EXPECT_EQ(safe_strcmp(s,s),0);
}

TEST(CStringSuite, StrcmpLiteral){
  aptr<char> s = WRAP_LITERAL("Test");

  EXPECT_EQ(safe_strcmp(s,WRAP_LITERAL("Test")),0);
}

TEST(CStringSuite, StrcmpOther){
  aptr<char> s = WRAP_LITERAL("Test");
  aptr<char> t = WRAP_LITERAL("Test");

  EXPECT_EQ(safe_strcmp(s,t),0);
}

TEST(CStringSuite, StrncmpSelf){
  aptr<char> s = WRAP_LITERAL("Test");

  EXPECT_EQ(safe_strncmp(s,s,3),0);
}

TEST(CStringSuite, Strncmp){
  aptr<char> s = WRAP_LITERAL("Tested");
  aptr<char> t = WRAP_LITERAL("Testing");

  EXPECT_EQ(safe_strncmp(s,s,4),0);
}

TEST(CStringSuite, Strcat){
  aptr<char> s = WRAP_LITERAL("Test");
  aptr<char> t = new_array<char>(20);

  zero<char>(t,20);

  aptr<char> result = safe_strcat(t,s);
  result = safe_strcat(t,s);
  
  EXPECT_EQ(safe_strlen(result),8);
  EXPECT_EQ(safe_strcmp(result,WRAP_LITERAL("TestTest")),0);
}

TEST(CStringSuite, Strncat){
  aptr<char> s = WRAP_LITERAL("Test");
  aptr<char> t = new_array<char>(20);

  zero<char>(t,20);

  aptr<char> result = safe_strncat(t,s,4);
  result = safe_strncat(t,s,3);
  
  EXPECT_EQ(safe_strlen(result),7);
  EXPECT_EQ(safe_strcmp(result,WRAP_LITERAL("TestTes")),0);
}

TEST(CStringSuite, Strtok1){
  aptr<char> s = new_array<char>(17);
  aptr<char> l = WRAP_LITERAL("Test,Test2,Test3");
  copy<char>(s,l,17);
  aptr<char> del = WRAP_LITERAL(",");
  aptr<char> t = WRAP_LITERAL("Test");
  aptr<char> result = safe_strtok(s,del);

  EXPECT_EQ(safe_strcmp(t,result),0);
}

TEST(CStringSuite, Strtok2){
  aptr<char> s = new_array<char>(17);
  aptr<char> l = WRAP_LITERAL("Test,Test2,Test3");
  copy<char>(s,l,17);
  aptr<char> del = WRAP_LITERAL(",");
  aptr<char> t = WRAP_LITERAL("Test2");
  aptr<char> result = safe_strtok(s,del);
  result = safe_strtok(NULL,del);

  EXPECT_EQ(safe_strcmp(t,result),0);
}

TEST(CStringSuite, Strcpy){
  aptr<char> s = new_array<char>(5);
  safe_strcpy(s,WRAP_LITERAL("Test"));

  EXPECT_EQ(safe_strcmp(s,WRAP_LITERAL("Test")),0);
}

TEST(CStringSuite, Strncpy){
  aptr<char> s = new_array<char>(5);
  safe_strncpy(s,WRAP_LITERAL("Test"),5);

  EXPECT_EQ(safe_strcmp(s,WRAP_LITERAL("Test")),0);
}

TEST(CStringSuite, Strchr){
  aptr<char> s = WRAP_LITERAL("Test");
  aptr<char> result = safe_strchr(s,'e');

  EXPECT_EQ(2,result-s);
}

TEST(CStringSuite, Memset){
  aptr<char> s = new_array<char>(5);

  safe_memset<char>(s,0,5);

  for(int c = 0; c < 5; c++){
    EXPECT_EQ(s[c],0);
  }
}

TEST(CStringSuite, Memcpy){
  aptr<char> s = new_array<char>(5);

  safe_memcpy<char>(s,WRAP_LITERAL("Test"),5);

  EXPECT_EQ(safe_strcmp(s,WRAP_LITERAL("Test")),0);
}

TEST(CStringSuite, Memmove){
  aptr<char> s = new_array<char>(5);

  safe_memmove<char>(s,WRAP_LITERAL("Test"),5);

  EXPECT_EQ(safe_strcmp(s,WRAP_LITERAL("Test")),0);
}

TEST(CStringSuite, Memcmp){
  aptr<char> s = WRAP_LITERAL("Test");

  EXPECT_EQ(safe_memcmp<char>(s,s,5),0);
}
