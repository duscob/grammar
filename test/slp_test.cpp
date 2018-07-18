//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/15/18.
//


#include <gtest/gtest.h>
#include <memory>

#include "grammar/slp.h"
#include "grammar/slp_metadata.h"
#include "grammar/slp_interface.h"


TEST(SLP, AddRule_Failed) {

  auto sigma = 4ul;

  grammar::SLP slp(sigma);

  EXPECT_DEATH(slp.AddRule(1, sigma + 1), "");
  EXPECT_EQ(slp.AddRule(1, 2), sigma + 1);
  EXPECT_EQ(slp.AddRule(1, sigma + 1), sigma + 2);
}


using RightHand = std::pair<std::size_t, std::size_t>;
using Rules = std::vector<std::pair<size_t, RightHand>>;


class SLP_TF : public ::testing::TestWithParam<std::tuple<std::shared_ptr<grammar::SLPInterface>,
                                                          std::tuple<std::size_t, Rules>>> {
 protected:
  std::shared_ptr<grammar::SLPInterface> slp_;

  // Sets up the test fixture.
  void SetUp() override {
    auto &sigma = GetSigma();
    auto &rules = GetRules();

    slp_ = GetSLP();
    slp_->Reset(sigma);

    for (auto &&rule : rules) {
      slp_->AddRule(rule.second.first, rule.second.second);
    }
  }

  const unsigned long &GetSigma() const { return std::get<0>(std::get<1>(GetParam())); }

  const Rules &GetRules() const {
    auto &rules = std::get<1>(std::get<1>(GetParam()));
    return rules;
  }

  std::shared_ptr<grammar::SLPInterface> GetSLP() {
    return std::get<0>(GetParam());
  }
};


TEST_P(SLP_TF, Sigma) {
  auto sigma = GetSigma();
  slp_->Reset(sigma);

  EXPECT_EQ(slp_->Sigma(), sigma);
}


TEST_P(SLP_TF, AddRules) {
  auto &sigma = GetSigma();
  auto &rules = GetRules();

  slp_->Reset(sigma);

  // Check rule addition
  for (auto &&rule : rules) {
    EXPECT_EQ(slp_->AddRule(rule.second.first, rule.second.second), rule.first);
  }

  EXPECT_EQ(slp_->Variables(), sigma + rules.size());
}


TEST_P(SLP_TF, Access) {
  auto &sigma = GetSigma();
  auto &rules = GetRules();

  // Check start rule
  EXPECT_EQ(slp_->Start(), rules.back().first);

  // Check rule access
  for (auto &&rule : rules) {
    EXPECT_EQ((*slp_)[rule.first], rule.second);
  }

  for (std::size_t i = 1; i <= sigma; ++i) {
    EXPECT_ANY_THROW((*slp_)[i]);
  }

  EXPECT_ANY_THROW((*slp_)[sigma + rules.size() + 1]);
}


TEST_P(SLP_TF, IsTerminal) {
  auto &sigma = GetSigma();

  for (int i = 1; i <= sigma; ++i) {
    EXPECT_TRUE(slp_->IsTerminal(i));
  }

  EXPECT_FALSE(slp_->IsTerminal(sigma + 1));
  EXPECT_FALSE(slp_->IsTerminal(sigma + 2));
}


TEST_P(SLP_TF, Span) {
  auto &sigma = GetSigma();
  auto &rules = GetRules();

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
    EXPECT_EQ(slp_->Span(j), spans[j]);
  }
}


TEST_P(SLP_TF, SpanLength) {
  auto &sigma = GetSigma();
  auto &rules = GetRules();

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
    EXPECT_EQ(slp_->SpanLength(j), lengths[j]);
  }
}


TEST_P(SLP_TF, Reset) {
  auto sigma = GetSigma();
  auto &rules = GetRules();

  EXPECT_EQ(slp_->Sigma(), sigma);
  EXPECT_EQ(slp_->Variables(), sigma + rules.size());

  slp_->Reset(sigma + 5);
  EXPECT_EQ(slp_->Sigma(), sigma + 5);
  EXPECT_EQ(slp_->Variables(), sigma + 5);
}


INSTANTIATE_TEST_CASE_P(
    SLP,
    SLP_TF,
    ::testing::Combine(
        ::testing::Values(
            std::shared_ptr<grammar::SLPInterface>(new grammar::SLPTInterface<grammar::SLP>(0)),
            std::shared_ptr<grammar::SLPInterface>(new grammar::SLPTInterface<grammar::SLPWithMetadata<grammar::PTS>>(0))
        ),
        ::testing::Values(
            std::make_tuple(4ul, Rules{{5, {1, 2}}, {6, {3, 4}}, {7, {5, 6}}, {8, {7, 7}}}),
            std::make_tuple(3ul, Rules{{4, {1, 2}}, {5, {4, 3}}, {6, {5, 4}}})
        )
    )
);
