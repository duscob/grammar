//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/15/18.
//


#include <gtest/gtest.h>

#include "grammar/slp.h"


TEST(SLP, AddRule_Failed) {
  auto sigma = 4ul;

  grammar::SLP slp(sigma);

  EXPECT_DEATH(slp.AddRule(1, sigma + 1), "");
  EXPECT_EQ(slp.AddRule(1, 2), sigma + 1);
  EXPECT_EQ(slp.AddRule(1, sigma + 1), sigma + 2);
}


class SLP_TF : public ::testing::TestWithParam<std::tuple<std::size_t,
                                                          std::vector<std::pair<std::size_t,
                                                                                std::pair<std::size_t,
                                                                                          std::size_t>>>>> {
 protected:
  grammar::SLP slp_{0};

  // Sets up the test fixture.
  void SetUp() override {
    auto &sigma = std::get<0>(GetParam());
    auto &rules = std::get<1>(GetParam());

    slp_ = grammar::SLP(sigma);

    // Check rule addition
    for (auto &&rule : rules) {
      slp_.AddRule(rule.second.first, rule.second.second), rule.first;
    }
  }
};


TEST_P(SLP_TF, Sigma) {
  auto sigma = std::get<0>(GetParam());
  grammar::SLP slp(sigma);

  EXPECT_EQ(slp.Sigma(), sigma);
}


TEST_P(SLP_TF, AddRules) {
  auto &sigma = std::get<0>(GetParam());
  auto &rules = std::get<1>(GetParam());

  grammar::SLP slp(sigma);

  // Check rule addition
  for (auto &&rule : rules) {
    EXPECT_EQ(slp.AddRule(rule.second.first, rule.second.second), rule.first);
  }
}


TEST_P(SLP_TF, Access) {
  auto &sigma = std::get<0>(GetParam());
  auto &rules = std::get<1>(GetParam());

  // Check start rule
  EXPECT_EQ(slp_.Start(), rules.back().first);

  // Check rule access
  for (auto &&rule : rules) {
    EXPECT_EQ(slp_[rule.first], rule.second);
  }

  for (std::size_t i = 1; i <= sigma; ++i) {
    EXPECT_ANY_THROW(slp_[i]);
  }

  EXPECT_ANY_THROW(slp_[sigma + rules.size() + 1]);
}


TEST_P(SLP_TF, IsTerminal) {
  auto &sigma = std::get<0>(GetParam());

  for (int i = 1; i <= sigma; ++i) {
    EXPECT_TRUE(slp_.IsTerminal(i));
  }

  EXPECT_FALSE(slp_.IsTerminal(sigma + 1));
  EXPECT_FALSE(slp_.IsTerminal(sigma + 2));
}


TEST_P(SLP_TF, Span) {
  auto &sigma = std::get<0>(GetParam());
  auto &rules = std::get<1>(GetParam());

  std::vector<std::vector<std::size_t>> spans(sigma + rules.size() + 1);
  auto i = 0ul;
  for (; i <= sigma; ++i) {
    spans[i].push_back(i);
  }

  for (auto &&rule : rules) {
    spans[i].insert(spans[i].end(),
                         spans[rule.second.first].begin(),
                         spans[rule.second.first].end());
    spans[i].insert(spans[i].end(),
                         spans[rule.second.second].begin(),
                         spans[rule.second.second].end());
    ++i;
  }

  for (int j = 1; j < spans.size(); ++j) {
//    std::cout << testing::PrintToString(spans[j]) << std::endl;
    EXPECT_EQ(slp_.Span(j), spans[j]);
  }
}


TEST_P(SLP_TF, SpanLength) {
  auto &sigma = std::get<0>(GetParam());
  auto &rules = std::get<1>(GetParam());

  std::vector<std::size_t> lengths(sigma + rules.size() + 1);

  int i = 0;
  for (; i <= sigma; ++i) {
    lengths[i] = 1;
  }

  for (auto &&rule : rules) {
    lengths[i] = lengths[rule.second.first] + lengths[rule.second.second];
    ++i;
  }

  for (int j = 1; j < lengths.size(); ++j) {
    EXPECT_EQ(slp_.SpanLength(j), lengths[j]);
  }
}


INSTANTIATE_TEST_CASE_P(SLP,
                        SLP_TF,
                        ::testing::Values(
                            std::make_tuple(4ul,
                                            std::vector<std::pair<std::size_t, std::pair<std::size_t, std::size_t>>>{
                                                {5, {1, 2}}, {6, {3, 4}}, {7, {5, 6}}, {8, {7, 7}}}),
                            std::make_tuple(3ul,
                                            std::vector<std::pair<std::size_t, std::pair<std::size_t, std::size_t>>>{
                                                {4, {1, 2}}, {5, {4, 3}}, {6, {5, 4}}})));