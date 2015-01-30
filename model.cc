/*****
      model.cc
      Defines probability spaces over events and sets of events
******/

#include "model.hh"

/*
  A word on the storage strategy of patterns.
  Whenever we have some pattern ABC, if we have one count of ABC then we correspondingly have one count of AB, BC and A_C.
  We may still store a separate pattern AB, however, because we have found counts of AB which are not part of the larger pattern ABC.
  In that case, the raw count for AB will represent the count of occurrences of AB&(~C).  Therefore when we want the total count of AB*, we must take the sum of the count for AB, and add in the counts for ABC, ABQ and any other superpattern.  So the raw count for AB will represent the counts of AB which were not part of *any* superpattern.
  The subdivide() function is not written yet, but when it is, we may do things like subdivide 1x"QABC" into 1x"QAB" and 1x"ABC".  In that case we would also want to generate or update the pattern "AB" with a negative count.  So if there were no previously existing examples of "AB" by itself we would have -1x"AB".  That seems wierd but it is consistent-because we add in the counts for superpatterns, if we ask for the count of ABC, we get one, QAB, we get one, and if we ask for the count of AB we get one (1+1-1).
*/

model::model(unsigned memory_constraint) {
  this->memory_constraint = memory_constraint;
  total_num_events = 0.0;
	
  //Load the counts for the base case N=1 with even prior distribution
  /*const double PRIOR_COUNTS = 0.02;  //We don't ever want to assume zero probability for something, so we initialize to a small prior count.
    for(unsigned i=0;i < (bounds.event_ub - bounds.event_lb);i++) {
    pattern patt;
    patt.p.push_back(i);
    add_pattern(patt, PRIOR_COUNTS);
    }*/
}

void model::add_sample(pattern* p, double count) {
  p->count += count;
  update_base_case(*p, count);
}
 
void model::add_pattern(pattern p, double count) {
  p.count = count;
  patterns.push_back(p);
  update_base_case(p, count);
}

void model::update_base_case(const pattern &p, double count) {
  for(vector<unsigned>::const_iterator p_iter = p.p.begin();p_iter != p.p.end();p_iter++) {
    base_case[*p_iter] += count;
  }
}

//FIXME: This function does not depend on any element of model.  Move the constants into the model.
double model::prior_count(unsigned pattern_length, const event_bounds &p_bounds) const {
  const double PRIOR_EVENT_DENSITY = 1.0, PRIOR_INTERVAL = 1.0;
  double prior_position_density = PRIOR_EVENT_DENSITY/(double(p_bounds.ub - p_bounds.lb));
  return pow(prior_position_density, pattern_length)*PRIOR_INTERVAL;
}

void model::subdivide_pattern(pattern* p, unsigned split_point) {
  //later
}

//This function is a stub.  But it will work in a limited way.  Ignores mem_constraint.
//FIXME: Add the ability to subdivide existing patterns as events are added until we are within some memory constraint.
/*-Need to determine which patterns should be purged by looking at the older ones (?) and
  a) asking which ones can be written as a product of conditionally independent factors
  b) asking which ones have only occurred once ever and are large.
  Maybe there is some more elegant way of choosing thresholds for "old" and "large".
*/
void model::train(const occurrence &givens) {
  static occurrence prev_givens; //state variablee - last set of givens, so we can determine which are new
  occurrence dup_givens = get_intersection(prev_givens, givens);
  add_pattern(get_pattern(givens), 1.0);
  add_pattern(get_pattern(dup_givens), -1.0); //So that we don't double count
  prev_givens = givens;
}

//Returns the size of the sample space containing p
//TODO: This probably should be frequency wrt time not frequency wrt events
double model::sample_size(const pattern& p) {
  return total_num_events - p.total_events_at_creation + 1.0;  //might be wrong - time based not event based?
}

double model::local_prob(const context& cxt, const pattern& p, unsigned t_abs) const {
  occurrence patt_occ - get_occurrence(p, t_abs);
  if(get_intersection(cxt.exclude, patt_occ).empty() && (!cxt.single_valued || is_single_valued(get_union(cxt.given, patt_occ)))) {
    return (p.count + prior_count(p.p.size(), cxt.p_bounds))/sample_size(p);
  }
  else
    return 0.0;
}

double model::global_prob(const context& cxt, const pattern& p, unsigned t_abs) const {
  double lp = local_prob(cxt, p, t_abs);
  double gp = 0.0;
  if(lp != 0.0) {
    for(vector<patt_link>::const_iterator p_super = p.super_patterns.begin();p_super != p.super_patterns.end();p_super++) {
      gp += global_prob(cxt, *(p_super->p_patt), t_abs - p_super->sub_pattern_t_offset)
    }
  }

  return gp;
}

//TODO: This can be rewritten to be fast
pattern get_base_level_pattern(unsigned p) const {
  for(vector<pattern>::const_iterator p_patt = base_level_patterns.begin();p_patt != base_level_patterns.end();p_patt++) {
    if(!(p_patt->p.empty()) && p_patt->p[0] == p)
      return *p_patt;
  }

  //We found nothing that matches; return the prior counts alone
  pattern patt;
  patt.count = 0;
  patt.total_events_at_creation = total_num_events - PRIOR_INTERVAL;
  patt.p.push_back(p);
  return patt;
}

double model::prob(const context& cxt, const event& e) const {
  return global_prob(cxt, get_base_level_pattern(e.p), e.t);
}

/*
   TODO: Create a new completions object which has an iterator of some kind so that we can run a simple loop
   through all the possible next events - including the ones which are just heat bath events.
*/
void model::get_first_order_completions(const context& cxt, list<completion> &completions) const {
  
}

