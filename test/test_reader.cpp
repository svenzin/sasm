#include <gtest/gtest.h>

#include <sasm/reader.h>

class TestReader : public ::testing::Test {
};

TEST_F(TestReader, Empty) {
    sasm::reader reader("");

    EXPECT_TRUE(reader.get().eof());
    
    EXPECT_TRUE(reader.get().eof());
}

TEST_F(TestReader, SingleContent) {
    sasm::reader reader("test");

    auto c = reader.get();
    EXPECT_FALSE(c.eof());
    EXPECT_EQ(c.offset, 0);
    EXPECT_EQ(c.width, 1);
    EXPECT_EQ(c.value, 't');

    c = reader.get();
    EXPECT_FALSE(c.eof());
    EXPECT_EQ(c.offset, 1);
    EXPECT_EQ(c.width, 1);
    EXPECT_EQ(c.value, 'e');

    c = reader.get();
    EXPECT_FALSE(c.eof());
    EXPECT_EQ(c.offset, 2);
    EXPECT_EQ(c.width, 1);
    EXPECT_EQ(c.value, 's');

    c = reader.get();
    EXPECT_FALSE(c.eof());
    EXPECT_EQ(c.offset, 3);
    EXPECT_EQ(c.width, 1);
    EXPECT_EQ(c.value, 't');

    EXPECT_TRUE(reader.get().eof());
}
