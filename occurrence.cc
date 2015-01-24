/*****
      occurrence.cc
      event class, occurrence class (typedef'd list of events) and some helper functions.
******/

#include "occurrence.h"

//completion related functions
bool compare_quality(const completion& c1, const completion& c2) {
  return (c1.quality < c2.quality);
}

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

//occurrence related functions
void print_occurrence(const occurrence& occ) {
  for(unsigned i=0;i < occ.size();i++) {
    std::cout << occ[i].t << "->" << occ[i].p << " ";
  }
  std::cout << endl << flush;
}

void print_occurrences(const list<occurrence>& occurrences) {
  std::cout << "occurrences: " << endl;
  for(list<occurrence>::const_iterator p_occurrence=occurrences.begin();p_occurrence != occurrences.end();p_occurrence++)
    print_occurrence(*p_occurrence);
}

occurrence subtract(const occurrence &occ, const occurrence &sub) {
  occurrence result;
  for(unsigned i = 0;i < occ.size();i++) {
    bool sub_contains_event = false;
    for(unsigned j = 0;j < sub.size();j++) {
      if(occ[i] == sub[j]) {
	sub_contains_event = true; //Could also remove this element from sub at this point.  This function could be optimized.
	break;
      }
    }
    if(!sub_contains_event)
      result.push_back(occ[i]);
  }
  return result;
}

//Returns true if every element in sub is also in occ.  So, returns true if sub is a strict subset of occ.
bool is_sub_occurrence(const occurrence &occ, const occurrence &sub) {
  for(unsigned i = 0;i < sub.size();i++) {
    bool occ_contains_event = false;
    for(unsigned j = 0;j < occ.size();j++) {
      if(sub[i] == occ[j]) {
	occ_contains_event = true; //Could also remove this element from sub at this point.  This function could be optimized.
	break;
      }
    }
    if(!occ_contains_event)
      return false;
  }
  return true;
}

//Returns a occurrence containing events that are in both occ1 and occ2
occurrence get_intersection(const occurrence& occ1, const occurrence& occ2) {
  occurrence result;
	
  //Assume: occ1 and occ2 are sorted by time and position
  //Assume: we never have two events in a single occurrence with the same position and time
  //Possibly I'm getting too clever for my own good here
  unsigned i=0,j=0;
  while(i < occ1.size() && j < occ2.size()) {
    if(occ1[i] < occ2[j]) {
      i++;
    } else if(occ2[j] < occ1[i]) {
      j++;
    } else {
      result.push_back(occ1[i]);
      i++;
      j++;
    }
  }

  return result;
}

occurrence get_union(const occurrence& occ1, const occurrence& occ2) { //"union" is a reserved word.
  occurrence result;
	
  //Assume: occ1 and occ2 are sorted by time and position
  //Assume: we never have two events in a single occurrence with the same position and time
  unsigned i=0,j=0;
  while(i < occ1.size() || j < occ2.size()) {
    if((j == occ2.size()) || (i < occ1.size() && occ1[i] < occ2[j])) {
      result.push_back(occ1[i]);
      i++;
    } else if((i == occ1.size()) || (occ2[j] < occ1[i])) {
      result.push_back(occ2[j]);
      j++;
    } else { //they are equal - add one but not the other and get the next of both
      result.push_back(occ2[j]);
      i++;
      j++;
    }
  }

  return result;
}

bool is_single_valued(const occurrence& occ) { //with respect to time
  for(unsigned i = 0;i < occ.size() - 1;i++) {
    if(occ[i].t == occ[i + 1].t)
      return false;
  }
  return true;
}

bool operator==(const occurrence& t1, const occurrence& t2) {
  if(t1.size() != t2.size())
    return false;
	
  for(unsigned i=0;i<t1.size();i++) {
    if(t1[i] != t2[i])
      return false;
  }
  return true;
}

occurrence get_occurrence(const event& e) {
  occurrence occ;
  occ.push_back(e);
  return occ;
}

