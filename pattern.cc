/*****
      pattern.cc
      Defines patterns and occurrences and operations on the two
      To Do:
      -move p[], dt[] into a class called pattern
      -create model_node and mn_link and move the associated functions into model.cc
******/

#include "pattern.hh"

//pattern related functions
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

//functions related to patterns and occurrences

//Note: t_abs here refers to the absolute time to assign to the earliest event in the created occurrence.
occurrence get_occurrence(const pattern& pat, int t_abs) {
  occurrence occ;
	
  int t = t_abs;
	
  for(unsigned i = 0;i < pat.p.size();i++) {
    event e;
    e.t = t;
    e.p = pat.p[i];
    occ.push_back(e);
		
    if(i < pat.dt.size())
      t += pat.dt[i];
  }
	
  return occ;
}

pattern get_pattern(const occurrence& occ) {
  pattern patt;
  patt.count = 0;
  for(unsigned i = 0;i < occ.size();i++) {
    patt.p.push_back(occ[i].p);
    if(i > 0)
      patt.dt.push_back(occ[i].t - occ[i - 1].t);
  }
  return patt;
}


//Functions related to matches


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


