//
// Created by Dustin Cobas Batista <dustin.cobas@gmail.com> on 7/15/18.
//


#include <gtest/gtest.h>
#include <memory>

#include "grammar/slp.h"
#include "grammar/slp_metadata.h"
#include "grammar/slp_interface.h"
#include "grammar/slp_helper.h"
#include "grammar/re_pair.h"


TEST(SLP, AddRule_Failed) {

  auto sigma = 4ul;

  grammar::SLP slp(sigma);

  EXPECT_DEATH(slp.AddRule(1, sigma + 1), "");
  EXPECT_EQ(slp.AddRule(1, 2), sigma + 1);
  EXPECT_EQ(slp.AddRule(1, sigma + 1), sigma + 2);
}


using RightHand = std::pair<std::size_t, std::size_t>;
using Rules = std::vector<RightHand>;


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
      slp_->AddRule(rule.first, rule.second);
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
  for (std::size_t i = 0; i < rules.size(); ++i) {
    EXPECT_EQ(slp_->AddRule(rules[i].first, rules[i].second), sigma + i + 1);
  }

  EXPECT_EQ(slp_->Variables(), sigma + rules.size());
}


TEST_P(SLP_TF, Access) {
  auto &sigma = GetSigma();
  auto &rules = GetRules();

  // Check start rule
  EXPECT_EQ(slp_->Start(), sigma + rules.size());

  // Check rule access
  for (std::size_t i = 0; i < rules.size(); ++i) {
    EXPECT_EQ((*slp_)[sigma + i + 1], rules[i]);
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
                    spans[rule.first].begin(),
                    spans[rule.first].end());
    spans[i].insert(spans[i].end(),
                    spans[rule.second].begin(),
                    spans[rule.second].end());
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
    lengths[i] = lengths[rule.first] + lengths[rule.second];
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
            std::make_tuple(4ul, Rules{{1, 2}, {3, 4}, {5, 6}, {7, 7}}),
            std::make_tuple(3ul, Rules{{1, 2}, {4, 3}, {5, 4}})
        )
    )
);



class SLPWithMetadata_TF : public ::testing::TestWithParam<std::tuple<std::size_t, Rules>> {
 protected:
  grammar::SLPWithMetadata<grammar::PTS> slp_{0};

// Sets up the test fixture.
  void SetUp() override {
    auto &sigma = std::get<0>(GetParam());
    auto &rules = std::get<1>(GetParam());

    slp_.Reset(sigma);

    for (auto &&rule : rules) {
      slp_.AddRule(rule.first, rule.second), rule.first;
    }
  }
};


TEST_P(SLPWithMetadata_TF, PTS) {
  slp_.ComputeMetadata();

  for (auto i = 1ul; i <= slp_.Variables(); ++i) {
    auto span = slp_.Span(i);
//    std::cout << "+" << testing::PrintToString(span) << std::endl;
    sort(span.begin(), span.end());
    span.erase(unique(span.begin(), span.end()), span.end());
//    std::cout << "-" << testing::PrintToString(span) << std::endl;
//    std::cout << "*" << testing::PrintToString(slp_.GetData(i)) << std::endl;

    EXPECT_EQ(slp_.GetData(i), span);
  }
}


INSTANTIATE_TEST_CASE_P(
    SLP,
    SLPWithMetadata_TF,
    ::testing::Values(
        std::make_tuple(4ul, Rules{{1, 2}, {3, 4}, {5, 6}, {7, 7}}),
        std::make_tuple(3ul, Rules{{1, 2}, {4, 3}, {5, 4}}),
        std::make_tuple(3ul, Rules{{1, 1}, {1, 2}, {5, 3}, {5, 2}, {4, 4}, {6, 7}, {9, 8}})
    )
);


template <typename T>
class SLPGeneric_TF : public ::testing::Test {
};

using MyTypes = ::testing::Types<grammar::SLP, grammar::SLPWithMetadata<grammar::PTS>>;
TYPED_TEST_CASE(SLPGeneric_TF, MyTypes);

TYPED_TEST(SLPGeneric_TF, construct) {
//  auto sigma = 3ul;
  std::vector<int> data = {1, 2, 3, 1, 2, 2, 1, 1, 1, 1, 3, 1, 2, 3};
  grammar::RePairEncoder<true> encoder;

  TypeParam slp(0);
  grammar::ConstructSLP(data.begin(), data.end(), encoder, slp);

  EXPECT_EQ(slp.Sigma(), *std::max_element(data.begin(), data.end()));

  Rules rules = {{1, 1}, {1, 2}, {3, 5}, {2, 4}, {6, 3}, {5, 6}, {7, 4}, {9, 10}, {11, 8}};
  for (int i = 0; i < rules.size(); ++i) {
    EXPECT_EQ(slp[slp.Sigma() + i + 1].first, rules[i].first);
    EXPECT_EQ(slp[slp.Sigma() + i + 1].second, rules[i].second);
//    EXPECT_EQ(slp.SpanLength(slp.Sigma() + i + 1), rules[i].length);
  }
}


TEST(SLPGeneric_TF, compute) {
//  auto sigma = 3ul;
  std::vector<int> data = {1, 2, 3, 1, 2, 2, 1, 1, 1, 1, 3, 1, 2, 3};
  grammar::RePairEncoder<true> encoder;

  grammar::SLPWithMetadata<grammar::PTS> slp(0);
  grammar::ComputeSLP(data.begin(), data.end(), encoder, slp);

  EXPECT_EQ(slp.Sigma(), *std::max_element(data.begin(), data.end()));

  Rules rules = {{1, 1}, {1, 2}, {3, 5}, {2, 4}, {6, 3}, {5, 6}, {7, 4}, {9, 10}, {11, 8}};
  for (int i = 0; i < rules.size(); ++i) {
    EXPECT_EQ(slp[slp.Sigma() + i + 1].first, rules[i].first);
    EXPECT_EQ(slp[slp.Sigma() + i + 1].second, rules[i].second);
//    EXPECT_EQ(slp.SpanLength(slp.Sigma() + i + 1), rules[i].length);
  }

  for (auto i = 1ul; i <= slp.Variables(); ++i) {
    auto span = slp.Span(i);
//    std::cout << "+" << testing::PrintToString(span) << std::endl;
    sort(span.begin(), span.end());
    span.erase(unique(span.begin(), span.end()), span.end());
//    std::cout << "-" << testing::PrintToString(span) << std::endl;
//    std::cout << "*" << testing::PrintToString(slp_.GetData(i)) << std::endl;

    EXPECT_EQ(slp.GetData(i), span);
  }
}