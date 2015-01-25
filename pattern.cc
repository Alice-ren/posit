/*****
      pattern.cc
      Defines patterns and occurrences and operations on the two
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

//Returns a occurrence for every way that the given pattern can match the given occurrence, but we have to match the whole pattern, and
//therefore the size of the pattern has to be <= the size of the occurrence.
bool get_suboccurrences_matching_pattern(const occurrence &occ, const pattern &patt, list<occurrence> &matches) {
  bool found_matches = false;
  for(int i = 0;i <= occ.size() - patt.p.size();i++) {
    occurrence sub = get_occurrence(patt, occ[i].t);
    if(is_sub_occurrence(occ, sub)) {
      matches.push_back(sub);
      found_matches = true;
    }
  }
  return found_matches;
}

//Same but just returns a count
double count_suboccurrences_matching_pattern(const occurrence &occ, const pattern &patt) {
  double count = 0.0;
  for(int i = 0;i <= occ.size() - patt.p.size();i++) {
    occurrence sub = get_occurrence(patt, occ[i].t);
    if(is_sub_occurrence(occ, sub)) {
      count++;
    }
  }
  return count;
}

//Helper function for get_matches
//Return the minimum dt we can add to an the elements of occ1 such that at least one event matches exactly.
//Does not return dt=0
int min_dt(const occurrence& occ1, const occurrence& occ2) {
  //Assume: occ1 and occ2 are sorted by time
  int min_dt = INT_MAX;
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
  return min_dt;
}

//This returns the matching part of the occurrence and the pattern for every way the occurrence can match the pattern
//This function does not set *all* the elements of match; for instance:
//We do not assume that patt is a pattern from the database, it could be arbitrary, therefore we don't set the match pattern pointer.
//We do not know the global count at this point.
//We do not know the probability at this point.
bool get_occurrence_matches(
			    const occurrence &occ, //the occurrence we are searching over
			    const pattern &patt, //the pattern to search for.  Remember that patterns implicitly include all their subpatterns.
			    list<match> &matches, //the output - a list of matches
			    bool get_submatches) //for any matching occurrence, include all suboccurrences (which must also match)
{
  //Assume: the elements of occ are sorted by time.
  if(occ.empty() || patt.p.empty()) {std::cout << "occurrence or pattern is empty in get_matching_occurrences...\n"; return false; }
	
  bool found_matches = false;
  int t = occ[occ.size() - 1].t;
  int dt = 1;
  occurrence patt_occ = get_occurrence(patt, t);
	
  while(patt_occ[patt_occ.size() - 1].t > occ[0].t && dt != INT_MAX) {
    patt_occ = get_occurrence(patt, t);
    occurrence common_occ = get_intersection(occ, patt_occ);
    if(!common_occ.empty()) {
      match m;
      m.p_patt = NULL; //This must be supplied externally; the given pattern may be a reference to a temporary
      m.patt_occ = patt_occ;
      m.match_occ = common_occ;
      m.local_count = patt.count;
      m.match_global_count = 0;
      m.patt_global_count = 0;
      matches.push_back(m);

      //Get the submatches too.
      if(get_submatches && !(common_occ == occ))
	get_occurrence_matches(common_occ, get_pattern(common_occ), matches, true); //FIXME: huh?  why would this work.

      found_matches = true;
    }
		
    //get the next dt to subtract from the pattern t_abs
    dt = min_dt(occ, patt_occ);
    t -= dt;
  }
	
  return found_matches;
}

//For each match, get the global counts, taking into account occurrences which are supersets of this one.
//FIXME: It should be possible to do this in O(Nlog(N)) time since we have an acyclic directed graph here.
//FIXME: corner case here for two identical matches?
void set_global_counts(list<match> &matches) {
  //Sort the matches by occurrence size, with shortest first.
  matches.sort(match_occurrence_size_ascending);
	
  for(list<match>::iterator p_match_a = matches.end();p_match_a != matches.begin();) {
    list<match>::iterator p_match_b = p_match_a;
    p_match_a--;
		
    //Start with the local count of this occurrence	
    p_match_a->match_global_count = p_match_a->local_count;
    p_match_a->patt_global_count = p_match_a->local_count;
		
    //Now add the local counts of the super-occurrences to get the true global count.
    while(p_match_b != matches.end()) {
      p_match_a->match_global_count += (is_sub_occurrence(p_match_b->match_occ, p_match_a->match_occ)*(p_match_b->local_count));
      p_match_a->patt_global_count += (is_sub_occurrence(p_match_b->patt_occ, p_match_a->patt_occ)*(p_match_b->local_count));
      p_match_b++;
    }
  }
}

//For each match, get the true counts, taking into account occurrences which are subsets of this one.
//FIXME: It should be possible to do this in O(Nlog(N)) time since we have an acyclic directed graph here.
void set_local_counts(list<match> &matches, occurrence oracle_occ) {
  //Sort the matches by occurrence size, with shortest first.
  matches.sort(match_occurrence_size_ascending);
	
  for(list<match>::iterator p_match_a = matches.end();p_match_a != matches.begin();) {
    list<match>::iterator p_match_b = p_match_a;
    p_match_a--;
		
    //Find the true global count of this occurrence.
    pattern patt = get_pattern(p_match_a->match_occ);
    double count = count_suboccurrences_matching_pattern(oracle_occ, patt);
    //Now subtract the global counts of the superpatterns to get the true local count.
    while(p_match_b != matches.end()) {
      count -= (count_suboccurrences_matching_pattern(p_match_b->match_occ, patt)*(p_match_b->local_count));
      p_match_b++;
    }
    p_match_a->local_count = count;
  }
}

//Eliminate duplicate pattern occurrences from this list of matches
void make_unique(list<match> &matches) {
  for(list<match>::iterator p_match_a = matches.begin();p_match_a != matches.end();p_match_a++) {
    list<match>::iterator p_match_b = p_match_a;
    p_match_b++;
    while(p_match_b != matches.end()) {
      if(p_match_a->patt_occ == p_match_b->patt_occ) {
	p_match_b = matches.erase(p_match_b);
      } else
	p_match_b++;
    }
  }
}

//Eliminate duplicate pattern occurrences from this list of matches, and sum counts
void consolidate(list<match> &matches) {
  for(list<match>::iterator p_match_a = matches.begin();p_match_a != matches.end();p_match_a++) {
    list<match>::iterator p_match_b = p_match_a;
    p_match_b++;
    while(p_match_b != matches.end()) {
      if(p_match_a->match_occ == p_match_b->match_occ && p_match_a->e == p_match_b->e) {
	p_match_a->match_global_count += p_match_b->match_global_count;
	p_match_a->patt_global_count += p_match_b->match_global_count;
	p_match_b = matches.erase(p_match_b);
      } else
	p_match_b++;
    }
  }
}

bool match_occurrence_size_descending(const match& m1, const match& m2) {
  return (m1.match_occ.size() > m2.match_occ.size());
}

bool match_occurrence_size_ascending(const match& m1, const match& m2) {
  return (m1.match_occ.size() < m2.match_occ.size());
}

//bool match_global_count_descending(const match& m1, const match& m2) {
//	return (m1.global_count < m2.global_count);
//}

bool match_new_event_descending(const match& m1, const match& m2) {
  return m1.e < m2.e;
}

void print_match(const match& match) {
  std::cout << "match: " << endl;
  if(match.p_patt != NULL)
    print_pattern(*(match.p_patt));
  print_occurrence(match.patt_occ);
  print_occurrence(match.match_occ);
  std::cout << "local count: " << match.local_count;
  std::cout << " match global count: " << match.match_global_count << endl;
  std::cout << " pattern global count: " << match.patt_global_count << endl;
}

void print_matches(const list<match>& matches) {
  std::cout << "printing matches: " << endl;
  for(list<match>::const_iterator p_match=matches.begin();p_match != matches.end();p_match++) {
    print_match(*p_match);
  }
}

