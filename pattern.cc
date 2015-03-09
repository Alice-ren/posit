/*****
      pattern.cc
      Defines patterns and occurrences and operations on the two
      To Do:
      -move p[], dt[] into a class called pattern
      -create model_node and mn_link and move the associated functions into model.cc
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
  cout << e.t << "->" << e.p;
}

//event_ptr related functions
event_ptr::event_ptr(const pattern* p) {
  p_patt = p;
  i = 0;
  t_abs = 0;
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

event event_ptr::event_at() {
  event e;
  if(i < p_patt->p.size()) {  //bounds checking: not for losers
    e.p = p_patt->p[i];
    e.t = t_abs;
  }
  return e;
}

bool event_ptr::at_end() {
  return (i >= p_patt->p.size());
}

//pattern related functions
pattern() {
  //nothing to do
}

pattern::pattern(bool p) {
  this->p.push_back(p);
}

void pattern::insert(bool p, int t_offset) {
  int t_abs = 0;
  for(unsigned i = 0;i < p.size();i++) {
    if(i > 0)
      t_abs += dt[i - 1];
  }

  if(i < p.size()) {
    //insert
    if(i > 0) {
      int 

    } else {

    }
  } else {
    
  }
}

event_ptr pattern::begin() {
  return event_ptr(this);
}

void print_pattern(const pattern& p) {
  std::cout << p.count << " of " << flush;
  for(unsigned i=0;i < p.p.size();i++) {
    std::cout << p.p[i];
    if(i < p.dt.size())
      std::cout << " -" << p.dt[i] << "- " << flush;
  }
  std::cout << endl << flush;
}

void print_patterns(const list<pattern>& patterns) {
  std::cout << "patterns: " << endl;
  for(list<pattern>::const_iterator p_patt=patterns.begin();p_patt != patterns.end();p_patt++)
    print_pattern(*p_patt);
}

bool operator==(const pattern& p1, const pattern& p2) {
  if(p1.p.size() != p2.p.size() || p1.dt.size() != p2.dt.size())
    return false;
	
  for(unsigned i=0;i<p1.p.size();i++) {
    if(p1.p[i] != p2.p[i])
      return false;
  }
  for(unsigned i=0;i<p1.dt.size();i++) {
    if(p1.dt[i] != p2.dt[i])
      return false;
  }
  return true;
}

pattern subtract(const pattern &p, int t_offset, const pattern &sub_p); //Returns the events for which event is in p but not in sub_p
bool is_sub(const pattern &p, int t_offset, const pattern &sub_p) {
  return get_intersection(p, t_offset, sub_p) == sub_p;
}

bool is_compatible(const pattern& p1, int t_offset, const pattern& p2) {
  return is_single_valued(get_union(p1, t_offset, p2));
}

//Assume: if we have 2 p's with an interval of zero between, they are in the order 0,1.
//This condition is enforced as long as you don't directly access dt and p.
pattern get_intersection(const pattern& p1, int t_offset, const pattern& p2) {
  pattern result;

  #define next_p1 0
  #define next_p2 1
  #define next_both 2
  
  unsigned i=0,j=0;
  int pr_last_t=INT_MAX,p1_t=0,p2_t=t_offset;
  unsigned action;
  while(true) {
    if(p1_t < p2_t)
      action = next_p1;
    else if(p2_t < p1_t)
      action = next_p2;
    else { //p1_t == p2_t
      if(p1.p[i] < p2.p[j])
	action = next_p1;
      else if(p1.p[i] > p2.p[j])
	action = next_p2;
      else {
	result.p.push_back(p1.p[i]);
	if(pr_last_t != INT_MAX)
	  result.dt.push_back(p1_t - pr_last_t);
	pr_last_t = p1_t;
	action = next_both;
      }
    }
    if(action == next_p1) {
      if(i < p1.dt.size()) {
	p1_t += p1.dt[i];
	i++;
      } else
	break;
    } else if(action == next_p2) {
      if(j < p2.dt.size()) {
	p2_t += p2.dt[j];
	j++;
      } else
	break;
    } else if(action == next_both) {
      if(i < p1.dt.size() && j < p2.dt.size()) {
	p1_t += p1.dt[i];
	p2_t += p2.dt[j];
	i++;
	j++;
      } else
	break;
    }
  }
  
  return result;
}

pattern get_union(const pattern& p1, int offset, const pattern& p2);
bool is_single_valued(const pattern& p);
void convolute(const pattern& p1, const pattern& p2, list<pattern>& patterns, list<int>& t_offset);


//Helper function for convolute()
//Return the minimum dt we can add to an the elements of occ1 such that at least one event matches exactly.
//Does not return dt=0
int min_dt(const occurrence& occ1, const occurrence& occ2) {
  //Assume: occ1 and occ2 are sorted by time
  int min_dt = INT_MAX;
  if(occ2[occ2.size() - 1].t < occ1[0].t) {
    unsigned i=0,j=0;
    while(i < occ1.size() && j < occ2.size()) {
      if(occ1[i].t < occ2[j].t) {
	for(unsigned k=j;(k < occ2.size()) && (occ2[k].t - occ1[i].t < min_dt);k++) {
	  if(occ1[i].p == occ2[k].p) {
	    min_dt = occ2[k].t - occ1[i].t;
	    break;
	  }
	}
	i++;
      } else
	j++;
    }
  }
  return min_dt;
}

//FIXME: handle corner case where time delta of occ1 and occ2 adds up to more than INT_MAX
void convolute(const occurrence& occ1, const occurrence& occ2, list<pattern>& patterns, list<int>& t_offset);
{
  if(occ1.empty() || occ2.empty())
    return;
  
  //Assume: the elements of occ1, occ2 are sorted by time.
  occ1 = get_occurrence(get_pattern(occ1), -INT_MAX); //rebase to avoid as many overflow issues as possible
  int t = occ1[occ1.size() - 1] + 1;
  pattern patt = get_pattern(occ2);
  occurrence shifted_occ = get_occurrence(patt, t); //initialize to just out of range
  
  while(true) {
    int dt = min_dt(occ, shifted_occ);
    if(dt != INT_MAX)
      t -= dt;
    else
      return;

    shifted_occ = get_occurrence(patt, t);
    occurrence intersect_occ = get_intersection(occ1, shifted_occ);
    if(!intersect_occ.empty()) {
      patterns.push_back(get_pattern(intersect_occ));
      t_offset.push_back(intersect_occ[0].t + INT_MAX); //x - occ1[0].t == x + INT_MAX
    }
    else
      cout << "ERROR: intersection should not be empty if min_dt != INT_MAX\n";
  }
}


