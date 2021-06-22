//
// Created by Dustin Cobas <dustin.cobas@gmail.com> on 5/11/21.
//

#ifndef GRAMMAR_SLP_COMPACT_H_
#define GRAMMAR_SLP_COMPACT_H_

#include <vector>
#include <unordered_map>
#include <optional>
#include <tuple>

namespace grammar {

template<typename TGetLeafId, typename TGetChildren, typename TVariable, typename TLeafAction, typename TPreAction, typename TPostAction>
void TraverseGrammarTree(TGetLeafId &t_get_leaf_id,
                         TGetChildren &t_get_children,
                         const TVariable &t_var,
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

  TraverseGrammarTree(t_get_leaf_id, t_get_children, left, t_leaf_action, t_pre_action, t_post_action);
  TraverseGrammarTree(t_get_leaf_id, t_get_children, right, t_leaf_action, t_pre_action, t_post_action);

  t_post_action(t_var);
}

template<typename TGetLeafId, typename TGetChildren, typename TRoots, typename TLeafAction, typename TPreAction, typename TPostAction>
void TraverseGrammarForest(TGetLeafId &t_get_leaf_id,
                           TGetChildren &t_get_children,
                           const TRoots &t_roots,
                           TLeafAction &t_leaf_action,
                           TPreAction &t_pre_action,
                           TPostAction &t_post_action) {
  for (const auto &root: t_roots) {
    TraverseGrammarTree(t_get_leaf_id, t_get_children, root, t_leaf_action, t_pre_action, t_post_action);
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

  for (const auto &root: t_roots) {
    if (nt_id.find(root) == nt_id.end())
      TraverseGrammarTree(get_leaf_id, t_get_children, root, leaf_action, pre_action, post_action);
  }

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
  for (const auto &root: t_roots) {
    if (nt_id.find(root) == nt_id.end())
      TraverseGrammarTree(get_leaf_id, t_get_children, root, leaf_action, pre_action, post_action);
  }
  tree.emplace_back(0);

  return std::make_tuple(std::move(tree), std::move(leaves), std::move(nt_id));
}

template<typename TNode, typename TState, typename TIsTerminal, typename TGetFirstNode, typename TGetNextLeaf, typename TIsTreeComplete, typename TReport>
void ExpandInternalNodeCompactSLP(const TNode &t_root,
                                  std::size_t &t_length,
                                  TState t_state,
                                  TIsTerminal &t_is_terminal,
                                  TGetFirstNode &t_get_first_node,
                                  TGetNextLeaf &t_get_next_leaf,
                                  TIsTreeComplete &t_is_tree_complete,
                                  TReport &t_report) {
  auto[node, state] = t_get_first_node(t_root, t_state);
  std::remove_reference_t<decltype(std::get<1>(t_get_next_leaf(node, state)))> var;

  do {
    std::tie(node, var, state) = t_get_next_leaf(node, state);

    if (t_is_terminal(var)) {
      t_report(var);
      --t_length;
    } else {
      ExpandInternalNodeCompactSLP(
          var, t_length, TState(), t_is_terminal, t_get_first_node, t_get_next_leaf, t_is_tree_complete, t_report);
    }
  } while (!t_is_tree_complete(state) && t_length > 0);
}

template<typename TTree, typename TRankLeaf, typename TLeaves, typename TNode, typename TReport>
void ExpandCompactSLPForward(const TTree &t_tree,
                             std::size_t t_sigma,
                             const TLeaves &t_leaves,
                             const TRankLeaf &t_rank_leaf,
                             TNode t_root,
                             std::size_t t_length,
                             TReport &t_report) {
  if (t_length == 0) return;

  auto is_terminal = [&t_sigma](auto tt_var) {
    return tt_var <= t_sigma;
  };

  if (is_terminal(t_root)) {
    t_report(t_root);
    return;
  }

  if (auto root = t_root - t_sigma - 1; t_tree[root] == 0) {
    t_report(t_leaves[t_rank_leaf(root)]);
    return;
  }

  struct State {
    std::size_t excess = 2;
    std::size_t leaf_rank = 0;
  } state;

  auto get_first_node = [&t_sigma, &t_tree, &t_rank_leaf](auto tt_root, auto tt_state) {
    tt_state.leaf_rank = t_rank_leaf(tt_root - t_sigma - 1);

    return std::make_pair(tt_root, tt_state);
  };

  auto get_next_leaf = [&t_sigma, &t_tree, &t_leaves](auto tt_node, auto tt_state) {
    tt_node -= t_sigma + 1;

    while (t_tree[++tt_node] == 1) {
      ++tt_state.excess;
    }

    --tt_state.excess;
    ++tt_state.leaf_rank;
    return std::make_tuple(tt_node + t_sigma + 1, t_leaves[tt_state.leaf_rank - 1], tt_state);
  };

  auto is_tree_complete = [](const auto &tt_state) {
    return tt_state.excess == 0;
  };

  ExpandInternalNodeCompactSLP(
      t_root, t_length, state, is_terminal, get_first_node, get_next_leaf, is_tree_complete, t_report);
}

template<typename TTree, typename TRankLeaf, typename TLeaves, typename TNode, typename TReport>
void ExpandCompactSLPBackward(const TTree &t_tree,
                              std::size_t t_sigma,
                              const TLeaves &t_leaves,
                              const TRankLeaf &t_rank_leaf,
                              TNode t_root,
                              std::size_t t_length,
                              TReport &t_report) {
  if (t_length == 0) return;

  auto is_terminal = [&t_sigma](auto tt_var) {
    return tt_var <= t_sigma;
  };

  if (is_terminal(t_root)) {
    t_report(t_root);
    return;
  }

  if (auto root = t_root - t_sigma - 1; t_tree[root] == 0) {
    t_report(t_leaves[t_rank_leaf(root)]);
    return;
  }

  struct State {
    std::size_t n_leaves = 0;
    std::size_t leaf_rank = 0;
  } state;

  auto get_first_node = [&t_sigma, &t_tree, &t_rank_leaf](auto tt_root, auto tt_state) {
    tt_root -= t_sigma + 1;

    std::size_t excess = 2;
    while (excess > 0) {
      if (t_tree[++tt_root] == 1) {
        ++excess;
      } else {
        --excess;
        ++tt_state.n_leaves;
      }
    }

    tt_state.leaf_rank = t_rank_leaf(++tt_root);

    return std::make_pair(tt_root + t_sigma + 1, tt_state);
  };

  auto get_next_leaf = [&t_sigma, &t_tree, &t_leaves](auto tt_node, auto tt_state) {
    --tt_state.n_leaves;
    --tt_state.leaf_rank;
    return std::make_tuple(tt_node, t_leaves[tt_state.leaf_rank], tt_state);
  };

  auto is_tree_complete = [](const auto &tt_state) {
    return tt_state.n_leaves == 0;
  };

  ExpandInternalNodeCompactSLP(
      t_root, t_length, state, is_terminal, get_first_node, get_next_leaf, is_tree_complete, t_report);
}

template<typename TTree, typename TLeafRank, typename TLeaves, typename TNode, typename TReport>
void ExpandCompactSLP(const TTree &t_tree,
                      std::size_t t_sigma,
                      const TLeaves &t_leaves,
                      const TLeafRank &t_leaf_rank,
                      const TNode &t_root,
                      TReport &t_report) {
  ExpandCompactSLPForward(
      t_tree, t_sigma, t_leaves, t_leaf_rank, t_root, std::numeric_limits<std::size_t>::max(), t_report);
}

}

#endif //GRAMMAR_SLP_COMPACT_H_
