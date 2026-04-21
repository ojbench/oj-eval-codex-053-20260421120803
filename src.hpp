#pragma once
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

// You are NOT allowed to add any headers
// without the permission of TAs.
namespace Grammar {
class NFA;
NFA MakeStar(const char &character);
NFA MakePlus(const char &character);
NFA MakeQuestion(const char &character);
NFA Concatenate(const NFA &nfa1, const NFA &nfa2);
NFA Union(const NFA &nfa1, const NFA &nfa2);
NFA MakeSimple(const char &character);
NFA MakeEpsilon();

enum class TransitionType { Epsilon, a, b };

struct Transition {
  TransitionType type;
  int to;
  Transition(TransitionType type, int to) : type(type), to(to) {}
};

class NFA {
private:
  int start;
  std::unordered_set<int> ends;
  std::vector<std::vector<Transition>> transitions;

public:
  NFA() = default;
  ~NFA() = default;

  std::unordered_set<int>
  GetEpsilonClosure(std::unordered_set<int> states) const {
    std::unordered_set<int> closure;
    std::queue<int> queue;
    for (const auto &state : states) {
      if (closure.find(state) != closure.end())
        continue;
      queue.push(state);
      closure.insert(state);
    }
    while (!queue.empty()) {
      int current = queue.front();
      queue.pop();
      for (const auto &transition : transitions[current]) {
        if (transition.type == TransitionType::Epsilon) {
          if (closure.find(transition.to) == closure.end()) {
            queue.push(transition.to);
            closure.insert(transition.to);
          }
        }
      }
    }
    return closure;
  }

  std::unordered_set<int> Advance(std::unordered_set<int> current_states,
                                  char character) const {
    // 1) epsilon-closure of current states
    auto closure = GetEpsilonClosure(current_states);
    // 2) follow matching transitions
    std::unordered_set<int> direct_next;
    TransitionType want = (character == 'a') ? TransitionType::a
                                              : TransitionType::b;
    for (int s : closure) {
      for (const auto &tr : transitions[s]) {
        if (tr.type == want) {
          direct_next.insert(tr.to);
        }
      }
    }
    // 3) epsilon-closure of reached states
    return GetEpsilonClosure(direct_next);
  }

  bool IsAccepted(int state) const { return ends.find(state) != ends.end(); }

  int GetStart() const { return start; }

  friend NFA MakeStar(const char &character);
  friend NFA MakePlus(const char &character);
  friend NFA MakeQuestion(const char &character);
  friend NFA MakeSimple(const char &character);
  friend NFA Concatenate(const NFA &nfa1, const NFA &nfa2);
  friend NFA Union(const NFA &nfa1, const NFA &nfa2);
  friend NFA MakeEpsilon();
};

class RegexChecker {
private:
  NFA nfa;

  static NFA BuildConcat(const std::string &term) {
    bool has = false;
    NFA result;
    for (size_t i = 0; i < term.size(); ++i) {
      char c = term[i];
      if (c != 'a' && c != 'b') {
        continue;
      }
      NFA base = MakeSimple(c);
      if (i + 1 < term.size()) {
        char op = term[i + 1];
        if (op == '+') {
          base = MakePlus(c);
          ++i;
        } else if (op == '*') {
          base = MakeStar(c);
          ++i;
        } else if (op == '?') {
          base = MakeQuestion(c);
          ++i;
        }
      }
      if (!has) {
        result = base;
        has = true;
      } else {
        result = Concatenate(result, base);
      }
    }
    if (!has) return MakeEpsilon();
    return result;
  }

public:
  bool Check(const std::string &str) const {
    std::unordered_set<int> current;
    current.insert(nfa.GetStart());
    for (char ch : str) {
      current = nfa.Advance(current, ch);
    }
    for (int s : current) {
      if (nfa.IsAccepted(s)) return true;
    }
    return false;
  }

  RegexChecker(const std::string &regex) {
    // Split by '|'
    std::vector<std::string> parts;
    std::string cur;
    for (char ch : regex) {
      if (ch == '|') {
        parts.push_back(cur);
        cur.clear();
      } else {
        cur.push_back(ch);
      }
    }
    parts.push_back(cur);

    NFA built;
    bool first = true;
    for (const auto &p : parts) {
      NFA term = BuildConcat(p);
      if (first) {
        built = term;
        first = false;
      } else {
        built = Union(built, term);
      }
    }
    nfa = built;
  }
};

NFA MakeStar(const char &character) {
  NFA nfa;
  nfa.start = 0;
  nfa.ends.insert(0);
  nfa.transitions.push_back(std::vector<Transition>());
  if (character == 'a') {
    nfa.transitions[0].push_back({TransitionType::a, 0});
  } else {
    nfa.transitions[0].push_back({TransitionType::b, 0});
  }
  return nfa;
}

NFA MakePlus(const char &character) {
  NFA nfa;
  // two nodes: 0 (start) ->(char)-> 1 (accept), and 1 loops on char
  nfa.start = 0;
  nfa.ends.insert(1);
  nfa.transitions.assign(2, {});
  TransitionType t = (character == 'a') ? TransitionType::a : TransitionType::b;
  nfa.transitions[0].push_back({t, 1});
  nfa.transitions[1].push_back({t, 1});
  return nfa;
}

NFA MakeQuestion(const char &character) {
  NFA nfa;
  // two nodes with epsilon 0->1, and char 0->1
  nfa.start = 0;
  nfa.ends.insert(1);
  nfa.transitions.assign(2, {});
  TransitionType t = (character == 'a') ? TransitionType::a : TransitionType::b;
  nfa.transitions[0].push_back({TransitionType::Epsilon, 1});
  nfa.transitions[0].push_back({t, 1});
  return nfa;
}

NFA Concatenate(const NFA &nfa1, const NFA &nfa2) {
  NFA res;
  int n1 = static_cast<int>(nfa1.transitions.size());
  int n2 = static_cast<int>(nfa2.transitions.size());
  res.start = nfa1.start;
  res.transitions.assign(n1 + n2, {});

  // copy transitions of nfa1
  for (int i = 0; i < n1; ++i) {
    for (const auto &tr : nfa1.transitions[i]) {
      res.transitions[i].push_back(tr);
    }
  }
  // copy transitions of nfa2 with offset
  for (int i = 0; i < n2; ++i) {
    for (const auto &tr : nfa2.transitions[i]) {
      res.transitions[i + n1].push_back({tr.type, tr.to + n1});
    }
  }
  // connect each end of nfa1 to start of nfa2 via epsilon
  for (int e : nfa1.ends) {
    res.transitions[e].push_back({TransitionType::Epsilon, nfa2.start + n1});
  }
  // ends are ends of nfa2 (offset)
  for (int e : nfa2.ends) {
    res.ends.insert(e + n1);
  }
  return res;
}

NFA Union(const NFA &nfa1, const NFA &nfa2) {
  NFA res;
  int n1 = static_cast<int>(nfa1.transitions.size());
  int n2 = static_cast<int>(nfa2.transitions.size());
  // new start at 0, then nfa1 shifted by 1, then nfa2 shifted by 1 + n1
  res.start = 0;
  res.transitions.assign(1 + n1 + n2, {});

  // epsilon edges from new start to both starts
  res.transitions[0].push_back({TransitionType::Epsilon, 1 + nfa1.start});
  res.transitions[0].push_back({TransitionType::Epsilon, 1 + n1 + nfa2.start});

  // copy nfa1
  for (int i = 0; i < n1; ++i) {
    for (const auto &tr : nfa1.transitions[i]) {
      res.transitions[1 + i].push_back({tr.type, 1 + tr.to});
    }
  }
  // copy nfa2
  for (int i = 0; i < n2; ++i) {
    for (const auto &tr : nfa2.transitions[i]) {
      res.transitions[1 + n1 + i].push_back({tr.type, 1 + n1 + tr.to});
    }
  }
  // ends are union of both ends with offsets
  for (int e : nfa1.ends) res.ends.insert(1 + e);
  for (int e : nfa2.ends) res.ends.insert(1 + n1 + e);
  return res;
}

NFA MakeSimple(const char &character) {
  NFA nfa;
  nfa.start = 0;
  nfa.ends.insert(1);
  nfa.transitions.assign(2, {});
  if (character == 'a') {
    nfa.transitions[0].push_back({TransitionType::a, 1});
  } else {
    nfa.transitions[0].push_back({TransitionType::b, 1});
  }
  return nfa;
}

NFA MakeEpsilon() {
  NFA nfa;
  nfa.start = 0;
  nfa.ends.insert(0);
  nfa.transitions.assign(1, {});
  return nfa;
}

} // namespace Grammar
