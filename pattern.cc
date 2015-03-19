/*****
      pattern.cc
      Defines patterns and operations on patterns
******/

#include "pattern.hh"

//event related functions
bool operator<(const event& t1, const event& t2) {
  if(t1.t == t2.t)
    return t1.p < t2.p;
  else
    return t1.t < t2.t;
}

bool operator==(const event& t1, const event& t2) {
  return (t1.t == t2.t && t1.p == t2.p);
}

bool operator!=(const event& t1, const event& t2) {
  return (t1.t != t2.t || t1.p != t2.p);
}

void print_event(const event& e) {
  cout << "(" << e.t << ":" << e.p << ") ";
}

//event_ptr related functions
event_ptr::event_ptr(const pattern* p, int t_offset) {
  p_patt = p;
  i = 0;
  t_abs = t_offset;
}

event_ptr::event_ptr(const pattern* p, bool end) {
  if(end) {
    p_patt = p;
    i = p_patt->p.size();
  } else {
    p_patt = p;
    i = 0;
    t_abs = 0;
  }
}

bool event_ptr::operator++() {
  if(i < p_patt->p.size()) {
    if(i < p_patt->dt.size())
      t_abs += p_patt->dt[i];
    
    i++;
    return true;
  }
  return false;
}

event event_ptr::operator*() const {
  event e;
  if(i < p_patt->p.size()) {  //bounds checking: not for losers
    e.p = p_patt->p[i];
    e.t = t_abs;
  } else
    cout << "ERROR: tried to access past end of pattern.\n";
  
  return e;
}

bool operator==(const event_ptr& p1, const event_ptr& p2) {
  if(p1.p_patt != p2.p_patt)
    cout << "ERROR: tried to compare two event_ptr objects from different patterns.\n";

  return p1.i == p2.i;
}

bool operator!=(const event_ptr& p1, const event_ptr& p2) {
  return !(p1 == p2);
}

//pattern class member functions
pattern::pattern() {
  //nothing to do
}

pattern::pattern(bool p) {
  this->p.push_back(p);
}

event_ptr pattern::begin(int t_offset) const {
  return event_ptr(this, t_offset);
}

event_ptr pattern::end() const {
  return event_ptr(this, true);
}

void pattern::append(bool pos, unsigned delta_t = 0) {
  if(p.size() > 0)
    dt.push_back(delta_t);
  p.push_back(pos);
}

void pattern::insert(bool p, int t_offset) {
  (*this) = get_union(*this, t_offset, pattern(p));
}

int pattern::size() const {
  return p.size();
}

bool pattern::empty() const {
  return p.empty();
}

int pattern::width() const {
  int w = 0;
  for(unsigned i : dt)
    w += i;

  return w;
}

//functions operating on patterns


void print_pattern(const pattern& p) {
  for(event e : p)
    print_event(e);
}

void print_patterns(const list<pattern>& patterns) {
  std::cout << "patterns: " << endl;
  for(const pattern &p : patterns)
    print_pattern(p);
}

bool operator==(const pattern& p1, const pattern& p2) {
  if(p1.size() != p2.size())
    return false;

  event_ptr p_e1 = p1.begin();
  event_ptr p_e2 = p2.begin();
  while(p_e1 != p1.end() && p_e2 != p2.end()) {
    if(*p_e1 != *p_e2)
      return false;
    ++p_e1;
    ++p_e2;
  }
  return true;
}

bool operator!=(const pattern& p1, const pattern& p2) {
  return !(p1 == p2);
}

//FIXME: There are such striking similarities between the implementations of get_union, get_intersection and subtract that it would be
//easy enough to write a single function to do them all with a lambda defining the behavior or a constant.
pattern subtract(const pattern &p, int t_offset, const pattern &sub_p, int& result_offset) { //Returns the events for which event is in p but not in sub_p
  pattern result;
  result_offset = INT_MAX;
  
  int last_t = 0;
  event_ptr p_e1=p.begin(), p_e2=sub_p.begin(t_offset);
  while(p_e1 != p.end()) {
    if((p_e2 == sub_p.end()) || (*p_e1 < *p_e2)) {
      result.append((*p_e1).p, (*p_e1).t - last_t);
      last_t = (*p_e1).t;
      ++p_e1;
      if(result_offset == INT_MAX)
	result_offset = last_t;
    } else if(*p_e2 < *p_e1) {
      ++p_e2;
    } else {
      ++p_e1;
      ++p_e2;
    }
  }

  if(result_offset == INT_MAX)
    result_offset = 0; //This means the result is empty anyway but why return a wierd number, it looks like a bug.

  return result;
}
pattern subtract(const pattern &p, int t_offset, const pattern &sub_p) { int dummy; return subtract(p, t_offset, sub_p, dummy); }

bool is_sub(const pattern &p, int t_offset, const pattern &sub_p, int& result_offset) {
  return get_intersection(p, t_offset, sub_p, result_offset) == sub_p;
}
bool is_sub(const pattern &p, int t_offset, const pattern &sub_p) {
  return get_intersection(p, t_offset, sub_p) == sub_p;
}

bool is_compatible(const pattern& p1, int t_offset, const pattern& p2) {
  return is_single_valued(get_union(p1, t_offset, p2));
}

//FIXME not right.. t_offset of the union is not right
pattern get_xor(const pattern& p1, int t_offset, const pattern& p2, int& result_offset) {
  int sub1_offset, sub2_offset;
  pattern sub1 = subtract(p1, t_offset, p2, sub1_offset);
  pattern sub2 = subtract(p2, -t_offset, p1, sub2_offset);
  return get_union(sub1, sub2_offset - sub1_offset, sub2);
}
pattern get_xor(const pattern& p1, int t_offset, const pattern& p2) { int dummy; return get_xor(p1, t_offset, p2, dummy); }

pattern get_intersection(const pattern& p1, int t_offset, const pattern& p2, int& result_offset) {
  pattern result;
  result_offset = INT_MAX;
  
  int last_t = 0;
  event_ptr p_e1=p1.begin(), p_e2=p2.begin(t_offset);
  while(p_e1 != p1.end() && p_e2 != p2.end()) {
    if(*p_e1 < *p_e2) {
      ++p_e1;
    } else if(*p_e2 < *p_e1) {
      ++p_e2;
    } else {
      result.append((*p_e1).p, (*p_e1).t - last_t);
      last_t = (*p_e1).t;
      if(result_offset == INT_MAX)
	result_offset = last_t;
      ++p_e1;
      ++p_e2;
    }
  }

  if(result_offset == INT_MAX)
    result_offset = 0; //This means the result is empty anyway but why return a wierd number, it looks like a bug.
  
  return result;
}
pattern get_intersection(const pattern& p1, int t_offset, const pattern& p2) { int dummy; return get_intersection(p1, t_offset, p2, dummy); }

pattern get_union(const pattern& p1, int t_offset, const pattern& p2) {
  pattern result;
	
  int last_t = 0;
  event_ptr p_e1=p1.begin(), p_e2=p2.begin(t_offset);
  while(p_e1 != p1.end() || p_e2 != p2.end()) {
    if((p_e2 == p2.end()) || (p_e1 != p1.end() && *p_e1 < *p_e2)) {
      result.append((*p_e1).p, (*p_e1).t - last_t);
      last_t = (*p_e1).t;
      ++p_e1;
    } else if((p_e1 == p1.end()) || (*p_e2 < *p_e1)) {
      result.append((*p_e2).p, (*p_e2).t - last_t);
      last_t = (*p_e2).t;
      ++p_e2;
    } else { //they are equal - add one but not the other and get the next of both
      result.append((*p_e2).p, (*p_e2).t - last_t);
      last_t = (*p_e2).t;
      ++p_e1;
      ++p_e2;
    }
  }

  return result;
}

bool is_single_valued(const pattern& p) {
  return (find(p.dt.begin(), p.dt.end(), unsigned(0)) == p.dt.end());
}


//Helper function for convolute()
//Return the minimum dt we can add to an the elements of occ1 such that at least one event matches exactly.
//Does not return dt=0
int min_dt(const pattern& p1, int t_offset, const pattern& p2) {
  int min_dt = INT_MAX;
  event_ptr p_e1=p1.begin(), p_e2=p2.begin(t_offset);  
  while(p_e1 != p1.end() && p_e2 != p2.end()) {
    if((*p_e1).t < (*p_e2).t) {
      for(event_ptr p_e2_match = p_e2;(p_e2_match != p2.end()) && ((*p_e2_match).t - (*p_e1).t < min_dt);++p_e2_match) {
	if((*p_e1).p == (*p_e2_match).p) {
	  min_dt = (*p_e2_match).t - (*p_e1).t;
	  break;
	}
      }
      ++p_e1;
    } else
      ++p_e2;
  }
  return min_dt;
}

void convolute(const pattern& p1, const pattern& p2, list<pattern>& result, list<int>& result_offset, int min_size) {
  if(p1.empty() || p2.empty())
    return;
  
  int t_offset = p1.width() + 1;
  while(true) {
    int dt = min_dt(p1, t_offset, p2);
    if(dt != INT_MAX)
      t_offset -= dt;
    else
      return;

    int intersect_offset;
    pattern intersect = get_intersection(p1, t_offset, p2, intersect_offset);
    if(intersect.size() >= min_size) {
      result.push_back(intersect);
      result_offset.push_back(intersect_offset);
    }
  }
}
void convolute(const pattern& p1, const pattern& p2, list<pattern>& result, int min_size) { list<int> dummy; convolute(p1, p2, result, dummy, min_size); }

