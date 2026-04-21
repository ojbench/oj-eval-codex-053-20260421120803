#include <bits/stdc++.h>
#include "src.hpp"

int main() {
  using namespace std;
  using namespace Grammar;
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  string regex;
  string s;
  if (!(cin >> regex)) return 0;
  if (!(cin >> s)) return 0;
  RegexChecker checker(regex);
  cout << (checker.Check(s) ? "Accepted." : "Not Accepted.") << "\n";
  return 0;
}

