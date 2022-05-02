#include <gtest/gtest.h>
#include <example.h>

TEST(ExampleTest, SquareTest){
    EXPECT_EQ(square(0), 0);
    EXPECT_EQ(square(1), 1);
    EXPECT_EQ(square(2), 4);
    EXPECT_EQ(square(3), 9);
}
