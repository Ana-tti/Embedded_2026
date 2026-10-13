#include <gtest/gtest.h>
#include "../TimeParser.h"


TEST(TimeParserTest, TestCaseTimeLen) {

    char time_test[] = "0001";
    EXPECT_EQ(time_parse(time_test), TIME_LEN_ERROR);

    char time_test2[] = "000000001";
    EXPECT_EQ(time_parse(time_test2), TIME_LEN_ERROR);
}

TEST(TimeParserTest, TestCaseCorrectTime) {

    char time_test[] = "000105";
    EXPECT_EQ(time_parse(time_test), 65);
}

TEST(TimeParserTest, TestCaseIncorrectTime) {    

    char time_test[] = "000088";
    EXPECT_EQ(time_parse(time_test), TIME_VALUE_ERROR);

    char time_test2[] = "240000";
    EXPECT_EQ(time_parse(time_test2), TIME_VALUE_ERROR);

    char time_test3[] = "009800";
    EXPECT_EQ(time_parse(time_test3), TIME_VALUE_ERROR);

}

TEST(TimeParserTest, TestCaseNullTime) {

    char time_test5[] = "";
    EXPECT_EQ(time_parse(time_test5), TIME_ARRAY_ERROR);
}

TEST(TimeParserTest, TestCaseDigitCheck) {

    char time_test6[] = "s02345";
    EXPECT_EQ(time_parse(time_test6), TIME_DIGIT_ERROR);

}

TEST(TimeParserTest, TestCaseReturnValueCheck) {

    char time_test[] = "000000";
    EXPECT_EQ(time_parse(time_test), TIMER_RESULT_ERROR);

}


//dir build -> cmake --build .
// dir test_program -> timeparsertest.exe


// https://google.github.io/googletest/reference/testing.html
// https://google.github.io/googletest/reference/assertions.html
