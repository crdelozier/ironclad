#include <gtest/gtest.h>

#include <ironclad/ironclad.hpp>

using namespace safe;

ptr<FILE> file;

TEST(CStdioSuite, fopen){
  file = safe_fopen(WRAP_LITERAL("test.txt"), WRAP_LITERAL("w+"));
  EXPECT_TRUE(file != nullptr);
}

TEST(CStdioSuite, fwrite){
  EXPECT_EQ(17,safe_fwrite(WRAP_LITERAL("abcdefghijklmnop\n"),1,17,file));
  EXPECT_EQ(4,safe_fwrite(WRAP_LITERAL("str\n"),1,4,file));
}

TEST(CStdioSuite, fseek){
  EXPECT_EQ(0,safe_fseek(file,0,SEEK_SET));
}

TEST(CStdioSuite, fgetc){
  EXPECT_EQ('a',safe_fgetc(file));
}

TEST(CStdioSuite, fread){
  safe::array<char,20> buffer;
  EXPECT_EQ(16,safe_fread<char>(buffer,1,16, file));
}

TEST(CStdioSuite, ftell){
  EXPECT_EQ(17,safe_ftell(file));
}

TEST(CStdioSuite, fgets){
  aptr<char> buffer = new_array<char>(20);
  safe_fgets(buffer, 20, file);
  EXPECT_EQ(0,safe_ferror(file));
}

TEST(CStdioSuite, feof){
  EXPECT_EQ(0,safe_feof(file));
}

TEST(CStdioSuite, fclose){
  safe_fclose(file);
  EXPECT_TRUE(file == nullptr);
}

TEST(CStdioSuite, printfMessage){
  EXPECT_EQ(5,safe_printf(WRAP_LITERAL("Test\n")));
}

TEST(CStdioSuite, printfInt){
  EXPECT_EQ(10,safe_printf(WRAP_LITERAL("Number: %d\n"),5));
}

TEST(CStdioSuite, printfString){
  // TODO: Fix printf to allow aptrs as strings
  EXPECT_EQ(13,safe_printf(WRAP_LITERAL("String: %s\n"),WRAP_LITERAL("TEST").convert_to_raw()));
}
