#include "pattern.hh"

using namespace std;

int main() {

  pattern a;
  a.append(1, 1);
  a.append(0, 1);
  a.append(1, 1);
  //a.append(0, 1);
  pattern b = a;
  cout << "a: "; print_pattern(a);
  cout << "b: "; print_pattern(b);
  cout << "a == b:" << (a == b) << endl;
  cout << "a != b:" << (a != b) << endl;
  a.append(0, 1);
  a.insert(1, -1);
  cout << "a: "; print_pattern(a);
  cout << "b: "; print_pattern(b);
  cout << "a == b:" << (a == b) << endl;
  cout << "a != b:" << (a != b) << endl;
  cout << "a == a:" << (a == a) << endl;
  cout << "a != a:" << (a != a) << endl;
  cout << "union a, 0, b: "; print_pattern(get_union(a, 0, b));
  cout << "union a, 100, b: "; print_pattern(get_union(a, 100, b));
  cout << "union a, -100, b: "; print_pattern(get_union(a, -100, b));
  int offset;
  cout << "intsc a, 0, b: "; print_pattern(get_intersection(a, 0, b, offset));
  cout << "offset: " << offset << endl;
  cout << "intsc a, 1, b: "; print_pattern(get_intersection(a, 1, b, offset));
  cout << "offset: " << offset << endl;
  cout << "intsc a, -1, b: "; print_pattern(get_intersection(a, -1, b, offset));
  cout << "offset: " << offset << endl;
  cout << "xor a, 0, b: "; print_pattern(get_xor(a, 0, b));
  cout << "xor a, 1, b: "; print_pattern(get_xor(a, 1, b));
  cout << "is_single_valued(xor a 0 b): " << is_single_valued(get_xor(a, 0, b)) << endl;
  cout << "is_single_valued(xor a 1 b): " << is_single_valued(get_xor(a, 1, b)) << endl;
  cout << "conv a, b: ";
  list<pattern> conv_patts;
  list<int> conv_offsets;
  convolute(a, b, conv_patts, conv_offsets);
  auto p_patt = conv_patts.begin();
  auto p_offset = conv_offsets.begin();
  while(p_patt != conv_patts.end() && p_offset != conv_offsets.end()) {
    cout << "offset " << (*p_offset) << ": "; print_pattern(*p_patt);
    p_patt++;
    p_offset++;
  } 
  cout << endl;
  
  return 0;
}
