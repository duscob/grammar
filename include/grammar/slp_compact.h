//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 5/11/21.
//

#ifndef GRAMMAR_SLP_COMPACT_H_
#define GRAMMAR_SLP_COMPACT_H_

namespace grammar {

template<typename TVariable, typename TGetLeafId, typename TGetChildren, typename TLeafAction, typename TPreAction, typename TPostAction>
void TraverseGrammarTree(const TVariable &t_var,
                         TGetLeafId &t_get_leaf_id,
                         TGetChildren &t_get_children,
                         TLeafAction &t_leaf_action,
                         TPreAction &t_pre_action,
                         TPostAction &t_post_action) {
// leaf_id is "optional" compatible
  if (auto leaf_id = t_get_leaf_id(t_var)) {
    t_leaf_action(t_var, *leaf_id);
    return;
  }

  t_pre_action(t_var);

  auto[left, right] = t_get_children(t_var);

  TraverseGrammarTree(left, t_get_leaf_id, t_get_children, t_leaf_action, t_pre_action, t_post_action);
  TraverseGrammarTree(right, t_get_leaf_id, t_get_children, t_leaf_action, t_pre_action, t_post_action);

  t_post_action(t_var);
}

template<typename TRoots, typename TGetLeafId, typename TGetChildren, typename TLeafAction, typename TPreAction, typename TPostAction>
void TraverseGrammarForest(const TRoots &t_roots,
                           TGetLeafId &t_get_leaf_id,
                           TGetChildren &t_get_children,
                           TLeafAction &t_leaf_action,
                           TPreAction &t_pre_action,
                           TPostAction &t_post_action) {
  for (const auto &root: t_roots) {
    TraverseGrammarTree(root, t_get_leaf_id, t_get_children, t_leaf_action, t_pre_action, t_post_action);
  }
}

template<typename TRoots, typename TSigma, typename TGetChildren>
auto CreateCompactGrammarForest(const TRoots &t_roots, const TSigma &t_sigma, TGetChildren &t_get_children) {
  std::vector<bool> tree;
  std::vector<std::size_t> leaves;
  std::unordered_map<std::size_t, std::size_t> nt_id;

  auto get_leaf_id = [&t_sigma, &nt_id](const auto &tt_var) -> std::optional<int> {
    if (tt_var <= t_sigma) {
      return tt_var;
    }

    auto iter = nt_id.find(tt_var);
    return iter != nt_id.end() ? std::optional<int>{iter->second} : std::nullopt;
  };

  auto leaf_action = [&tree, &leaves](const auto &tt_var, const auto &tt_leaf_id) {
    tree.emplace_back(0);
    leaves.emplace_back(tt_leaf_id);
  };

  auto pre_action = [&tree, &t_sigma, &nt_id](const auto &tt_var) {
    nt_id[tt_var] = t_sigma + 1 + tree.size();
    tree.emplace_back(1);
  };

  auto post_action = [](const auto &tt_var) {};

  TraverseGrammarForest(t_roots, get_leaf_id, t_get_children, leaf_action, pre_action, post_action);

  return std::make_tuple(std::move(tree), std::move(leaves), std::move(nt_id));
}

template<typename TRoots, typename TSigma, typename TGetChildren>
auto CreateCompactGrammarTreeWithBP(const TRoots &t_roots, const TSigma &t_sigma, TGetChildren &t_get_children) {
  std::vector<bool> tree;
  std::vector<std::size_t> leaves;
  std::unordered_map<int, int> nt_id;

  auto get_leaf_id = [&t_sigma, &nt_id](const auto &tt_var) -> std::optional<int> {
    if (tt_var <= t_sigma) {
      return tt_var;
    }

    auto iter = nt_id.find(tt_var);
    return iter != nt_id.end() ? std::optional<int>{iter->second} : std::nullopt;
  };

  auto leaf_action = [&tree, &leaves](const auto &tt_var, const auto &tt_leaf_id) {
    tree.emplace_back(1);
    tree.emplace_back(0);
    leaves.emplace_back(tt_leaf_id);
  };

  auto pre_action = [&tree, &t_sigma, &nt_id](const auto &tt_var) {
    nt_id[tt_var] = t_sigma + 1 + tree.size();
    tree.emplace_back(1);
  };

  auto post_action = [&tree](const auto &tt_var) {
    tree.emplace_back(0);
  };

  tree.emplace_back(1);
  TraverseGrammarForest(t_roots, get_leaf_id, t_get_children, leaf_action, pre_action, post_action);
  tree.emplace_back(0);

  return std::make_tuple(std::move(tree), std::move(leaves), std::move(nt_id));
}

}

#endif //GRAMMAR_SLP_COMPACT_H_
