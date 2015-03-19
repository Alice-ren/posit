#include <iostream>
#include "configuration.hh"

using namespace std;

completion_set dummy_func1(const pattern& p) {
  completion_set cmp;
  cmp.t_offset = 1;
  cmp.event_prob = .4;
  cmp.complement_prob = .5;
  return cmp;
}

completion_set dummy_func2(const pattern& p) {
  completion_set cmp;
  cmp.t_offset = 2;
  cmp.event_prob = .15;
  cmp.complement_prob = .2;
  return cmp;
}

completion_set dummy_func3(const pattern& p) {
  completion_set cmp;
  cmp.t_offset = 2;
  cmp.event_prob = .3;
  cmp.complement_prob = .15;
  return cmp;
}

completion_set dummy_func4(const pattern& p) {
  completion_set cmp;
  cmp.t_offset = 3;
  cmp.event_prob = .1;
  cmp.complement_prob = .15;
  return cmp;
}

int main() {
  configuration_space s;
  s.predict(1, 9999, dummy_func1);
  s.predict(1, 9999, dummy_func3);
  s.predict(1, 9999, dummy_func2);
  s.predict(1, 9999, dummy_func4);
  s.display_contents();

  cout << "next event: ";
  print_event(s.next_event());
  cout << "next event prob: ";
  cout << s.next_event_cond_prob() << endl;
  s.choose_event();
  s.display_contents();
  cout << "next event: ";
  print_event(s.next_event());
  cout << "next event prob: ";
  cout << s.next_event_cond_prob() << endl;
  s.choose_event();
  s.display_contents();
  cout << "next event ready?: " << s.next_event_ready() << endl;
  
  return 0;
}
