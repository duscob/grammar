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

template<typename TNode, typename TState, typename TIsTerminal, typename TGetNextLeaf, typename TIsTreeComplete, typename TReport>
void ExpandInternalNodeCompactSLPFromFront(const TNode &t_root,
                                           std::size_t &t_length,
                                           TState t_state,
                                           TIsTerminal &t_is_terminal,
                                           TGetNextLeaf &&t_get_next_leaf,
                                           TIsTreeComplete &t_is_tree_complete,
                                           TReport &t_report) {
  auto node = t_root;
  auto state = t_state;
  std::remove_reference_t<decltype(std::get<1>(t_get_next_leaf(node, state)))> var;

  do {
    std::tie(node, var, state) = t_get_next_leaf(node, state);

    if (t_is_terminal(var)) {
      t_report(var);
      --t_length;
    } else {
      ExpandInternalNodeCompactSLPFromFront(
          var, t_length, TState(), t_is_terminal, t_get_next_leaf, t_is_tree_complete, t_report);
    }
  } while (!t_is_tree_complete(state) && t_length > 0);
}

template<typename TNode, typename TTree, typename TLeafRank, typename TLeaves, typename TReport>
void ExpandCompactSLPFromFront(const TNode &t_root,
                               std::size_t t_length,
                               std::size_t t_sigma,
                               const TTree &t_tree,
                               const TLeafRank &t_leaf_rank,
                               const TLeaves &t_leaves,
                               TReport &t_report) {
  if (t_length == 0) return;

  auto is_terminal = [&t_sigma](auto tt_var) {
    return tt_var <= t_sigma;
  };

  if (is_terminal(t_root)) {
    t_report(t_root);
    return;
  } else if (auto root = t_root - t_sigma - 1; t_tree[root] == 0) {
    t_report(t_leaves[t_leaf_rank(root)]);
    return;
  }

  struct State {
    int excess = 2;
    bool first_leaf_found = false;
    int leaf_pos = 0;
  } state;

  auto get_next_leaf = [&t_sigma, &t_tree, &t_leaf_rank, &t_leaves](auto tt_node, auto tt_state) {
    tt_node -= (t_sigma + 1);

    while (t_tree[++tt_node] == 1) {
      ++tt_state.excess;
    }

    --tt_state.excess;
    if (!tt_state.first_leaf_found) {
      tt_state.first_leaf_found = true;
      tt_state.leaf_pos = t_leaf_rank(tt_node);
    } else {
      ++tt_state.leaf_pos;
    }

    return std::make_tuple(tt_node + t_sigma + 1, t_leaves[tt_state.leaf_pos], tt_state);
  };

  auto is_tree_complete = [](const auto &tt_state) {
    return tt_state.excess == 0;
  };

  ExpandInternalNodeCompactSLPFromFront(
      t_root, t_length, state, is_terminal, get_next_leaf, is_tree_complete, t_report);
}

template<typename TNode, typename TTree, typename TLeafRank, typename TLeaves, typename TReport>
void ExpandCompactSLP(const TNode &t_root,
                      std::size_t t_sigma,
                      const TTree &t_tree,
                      const TLeafRank &t_leaf_rank,
                      const TLeaves &t_leaves,
                      TReport &t_report) {
  ExpandCompactSLPFromFront(
      t_root, std::numeric_limits<std::size_t>::max(), t_sigma, t_tree, t_leaf_rank, t_leaves, t_report);
}
}

#endif //GRAMMAR_SLP_COMPACT_H_
