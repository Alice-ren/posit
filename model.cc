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

double model::local_prob(const context& cxt, pattern& p, unsigned t_abs) const {
  occurrence patt_occ = get_occurrence(p, t_abs);
  if(p.last_visit_hash_id != current_visit_hash_id &&
     get_intersection(cxt.exclude, patt_occ).empty() &&
     (!cxt.single_valued || is_single_valued(get_union(cxt.given, patt_occ)))) {
    p.last_visit_hash_id = current_visit_hash_id;
    return (p.count + prior_count(p.p.size(), cxt.p_bounds))/(sample_size(p) + prior_count(0, cxt.p_bounds));  //this could be made faster but this is elegant
  }
  else
    return 0.0;
}

/*
Lets say you have a pattern ABCX
no other examples of ABC
no other examples of X
And you want to find P(X|ABC).
Then P(ABCX)/P(ABC) = 1.

Now my global_prob function will just return global_prob(cxt=ABC, patt=___X) = global_prob(cxt=ABC,patt=ABCX)
Which in fact is wrong!
So what did I actually find?  I think I found P(ABCX).
Maybe I can just normalize: P(ABCX) + P(ABCY) + P(ABCZ) = P(ABC)
Or: P(ABCX) + P(ABC~X) = P(ABC)
So then I can calculate P(ABC) from that and --

New observations.
1) If we visit every pattern a maximum of one time, then probability should be conserved.  Since if we have two patterns ABC and BCD,
the local counts are assuming ~ABCD in both cases.  If there were a pattern ABCD, then we would have counts for it; since we don't,
the assumption is that the count of ABCD is zero. --wait, is that a good assumption??
2) The "base case" patterns actually have a common sub: the pattern containing nothing.  The other base cases are supers of that.
3) We can find P(ABC&~X) by encoding ~X into the exclusions for the context, and calling global_prob() with that context, the
base case pattern, and the desired location in time.
4) For the case where we don't have the single_valued condition, our data will show no examples of A,B at the same time, and
therefore we will find P(A,B)=0.  The only difference would be in how the prior_counts are calculated, which is something I need
to revisit.
5) To find P(X|ABC), we can either find P(ABCX)/P(ABC) or we can find P(ABCX)/(P(ABCX) + P(ABC~X)).  The latter is a bit faster but
the former is more clean if we want to find many completions at once since we can just find P(ABC) once.

This is all still not quite right.  Doesn't smell right.  I think assumption 1) above is wrong.
Duh.
OK, more observations:
1)  First above is correct *except* that if there is no ABCD, it just means that ABCD is conditionally independent on the subs.
2)  If cxt represents some givens, then the below algorithm is broken since it will find P(___Q|ABC_) = P_local(_BCQ) + P_local(ABCQ).
But, if there exists a pattern ABCQ then P_local(_BCQ) = 0.  ABC is given, therefore we are not asking about (~A)BCQ or ABCQ, just ABCQ.
3)  Because of the conditional independence property, there always exists some (constructed) pattern ABC if ABC is a given.  We just
have to find out what it is.
4)  I have no idea how to handle exclusions wrt the multiplied probability method.
So the below stuff is a good start, but not there yet.  So much more complicated than it looks!
 */

//Calculates P(cxt & p)
double model::global_prob(const context& cxt, pattern& p, unsigned t_abs) const {
  double lp = local_prob(cxt, p, t_abs);
  double gp = 0.0;
  if(lp != 0.0) {
    for(auto p_super = p.super_patterns.begin();p_super != p.super_patterns.end();p_super++) {
      gp += global_prob(cxt, *(p_super->p_patt), t_abs - p_super->sub_pattern_t_offset);
      //Wrong: if the super overlaps this pattern in the givens, then we use the super only since the existence
      //of the super implies that the local probability is zero.
    }
  }

  return gp;
}

double model::prob(context cxt, const event& e) const {
  occurrence cxt_givens = cxt.givens;
  occurrence cxt_exclusions = cxt.exclusions;
  
  current_visit_hash_id++;
  double cxt_total_prob = global_prob(cxt, base_level_pattern, e.t);
  cxt.givens = union(cxt.given, get_occurrence(e));
  current_visit_hash_id++;
  double cxt_and_e_prob = global_prob(cxt, base_level_pattern, e.t);
  return cxt_and_e_prob/cxt_total_prob;
}

/*
   TODO: Create a new completions object which has an iterator of some kind so that we can run a simple loop
   through all the possible next events - including the ones which are just heat bath events.
*/
void model::get_first_order_completions(const context& cxt, list<completion> &completions) const {
  
}

