//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/15/18.
//


#include <gtest/gtest.h>

#include "grammar/slp.h"


TEST(SLP, SigmaSize) {
  grammar::SLP slp(4);

  EXPECT_EQ(slp.SigmaSize(), 4);
}


TEST(SLP, Rules) {
  auto sigma = 4ul;
  grammar::SLP slp(sigma);

  std::vector<std::pair<std::size_t, std::pair<std::size_t, std::size_t>>>
      rules = {{5, {1, 2}}, {6, {3, 4}}, {7, {5, 6}}, {8, {7, 7}}};

  // Check rule addition
  for (auto &&rule : rules) {
    EXPECT_EQ(slp.AddRule(rule.second.first, rule.second.second), rule.first);
  }


  // Check start rule
  EXPECT_EQ(slp.Start(), rules.back().first);


  // Check rule access
  for (auto &&rule : rules) {
    EXPECT_EQ(slp[rule.first], rule.second);
  }

  for (std::size_t i = 1; i <= sigma; ++i) {
    EXPECT_EQ(slp[i], std::make_pair(i, std::numeric_limits<std::size_t>::max()));
  }

  EXPECT_ANY_THROW(slp[sigma + rules.size() + 1]);
}


TEST(SLP, AddRule_Failed) {
  grammar::SLP slp(4);

  EXPECT_DEATH(slp.AddRule(1, 5), "");
  EXPECT_EQ(slp.AddRule(1, 2), 5);
  EXPECT_EQ(slp.AddRule(1, 5), 6);
}
