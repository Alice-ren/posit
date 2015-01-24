/*****
      model.cc
      Defines probability spaces over events and sets of events
******/

#include "model.h"

/*
  A word on the storage strategy of patterns.
  Whenever we have some pattern ABC, if we have one count of ABC then we correspondingly have one count of AB, BC and A_C.
  We may still store a separate pattern AB, however, because we have found counts of AB which are not part of the larger pattern ABC.
  In that case, the raw count for AB will represent the count of occurrences of AB&(~C).  Therefore when we want the total count of AB*, we must take the sum of the count for AB, and add in the counts for ABC, ABQ and any other superpattern.  So the raw count for AB will represent the counts of AB which were not part of *any* superpattern.
  Now, we may also have an instance in the data where we have QABC and we have existing patterns QAB and ABC.  In that case we have one count of QAB, one count of ABC, and *negative* one counts of AB.  That seems confusing, but it is actually consistent- if we ask for the count of ABC, we get one, QAB, we get one, and if we ask for the count of *AB* we get one.  The only thing we cannot ask for the count of is the count of occurences of AB which were not part of either QAB or ABC.  We could add the ability to ask that question by storing negative and positive counts for AB separately.
  Therefore, with the current storage scheme, the stored raw count of *any* pattern is essentially meaningless.  We can only ever ask the database for the count of ...**AB**..., that is, a certain pattern, with anything for the other values.  And the way we ask the database this question is by looking at the stored raw value and the stored raw values of any superpatterns.
*/

/*
  A word on the general processing strategy for finding pattern matches.
  Because of the curse of dimensionality, there in general will not exist any efficient mechanism for finding matches in the database to only some target pattern.  Therefore, no matter what our target pattern is, we might as well find the distance between that and every other pattern.
  Because of this fact, all the processing algorithms below are designed if possible to answer a whole set of questions at once - for example, list the most likely character at every point- so that the loops can be written to scan through the database exactly once.  If it was written in a more conceptually simple way, with functions to find the counts for one target occurrence for instance- then we would wind up scanning the whole database many times in the process of answering one question.  Scanning the database one time for any question we ask of it is a basic design goal, not an optimisation, so writing the simpler functions has been deliberately avoided.
*/

//model::model(const unsigned max_pattern_dt, const unsigned event_lb, const unsigned event_ub, const unsigned event_blocksize, const bool single_valued) {
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
 
void model::add_pattern(const pattern &p, double count) {
  p.count = count;
  patterns.push_back(p);
  update_base_case(p, count);
}

void model::update_base_case(const pattern &p, double count) {
  for(auto p_iter = p.p.begin();p_iter != p.p.end;p_iter++) {
    base_case[*p_iter] += count;
  }
}

double model::prior_count(unsigned pattern_length, const event_bounds &p_bounds) const {
  const double PRIOR_EVENT_DENSITY = 1.0, PRIOR_INTERVAL = 1.0;
  double prior_position_density = PRIOR_EVENT_DENSITY/(double(p_bounds.ub - p_bounds.lb));
  return pow(prior_position_density, pattern_length)*PRIOR_INTERVAL;
}

void model::delete_pattern(const pattern& p) {
  /*	auto p_patt = patterns.begin();
	while(p_patt != patterns.end()) {
	if(*p_patt == p) {
	p_patt = patterns.erase(p_patt);
	}
	else {
	p_patt->count += count_suboccurrences_matching_pattern(get_occurrence(p, 0), const pattern(*p_patt)); 
	//FIXME: wrong, this can double count.  I feel like I solved this once upon a time.
	p_patt++;
	}
	}*/
}
//ABC BCD ABCD
//remove ABCD
//want: 2xABC 2xBCD -1xBC
//Should be able to do this through proper use of set_local_counts
//Or, it might turn out to be a really nasty problem.  As you can see above maintaining the right counts could mean adding an extra pattern to the table.
//Maybe, instead I should write a function subdivide_pattern.  Then the two subs are clear, and either they exist, or they don't.  We just set the local counts to be equal to the parent.

void model::subdivide_pattern(list<pattern>::iterator p_patt) {
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
  occurrence dup_givens = get_intersection(prev_givens, givens);
  add_pattern(get_pattern(givens), 1.0);
  add_pattern(get_pattern(dup_givens), -1.0); //So that we don't double count
  prev_givens = givens;
}

/*
  FIXME: there still might be a case I'm not getting here- if we return two chains ABC_Q and XBC_Q with Q being the new event, the get_matches function never returns BC_Q and therefore we don't get the chance to consider BC_Q as a separate match (and with 2 counts it might be a more relevant match)
*/

void model::get_first_order_completions(const context& cxt, list<completion> &completions) const {
  if(cxt.type == NORMAL) {
    //Perform a match against every pattern in the database.  Stupid, but easy to write.
    list<match> raw_matches;
    for(list<pattern>::const_iterator p_patt = patterns.begin();p_patt != patterns.end();p_patt++)
      get_occurrence_matches(cxt.occ, *p_patt, raw_matches);

    //Find the global counts for each match
    //FIXME: double check this function.
    set_global_counts(raw_matches);

    list<match> new_matches;

    //Go through the raw matches and for each, generate a set of single-event matches
    for(auto p_raw_match = raw_matches.begin();p_raw_match != raw_matches.end();p_raw_match++) {
      match m = *p_raw_match;
      occurrence new_events = subtract(p_raw_match->patt_occ, p_raw_match->match_occ);
      for(auto p_new_event = new_events.begin();p_new_event != new_events.end();p_new_event++) {
	if(ctx.is_included(get_union(ctx.occ, get_occurrence(*p_new_event)))) {
	  m.e = *p_new_event;
	  new_matches.push_back(m);
	}
      }
    }

    //Sort so that matches for the same event are next to each other
    new_matches.sort(match_new_event_descending);
    consolidate(new_matches);

    //FIXME: the following is hokey and was put in place because I don't have any free time.
    //For each set of matches generating the same new event, output a single match with a calculated probability based on a simple arbitrary formula
    //For each match we have a weight which depends on:
    //c, global count of patterns: if c=1 then multiply by .05, if 2 then .5, else 1
    //n, number of events in match occurrence.
    //v, the volume of the space for a single event (depends on the bounds).  We multiply by n*log(v).
    //So then we have w = scale(c)*n*log(v).
    //We then take the sum of w for each match, and take the sum of the weighted probabilities, and normalize to w.
    completions.clear();
    event current_event = {UINT_MAX, INT_MAX};
    double norm = 0.0; //normalizing factor
    double p = 0.0; //total probability
    for(auto p_new_match = new_matches.begin();p_new_match != new_matches.end();p_new_match++) {
      if(p_new_match->e != current_event) {
	//If we have a match to output, output it
	if(p > 0.0 && norm > 0.0) {
	  completion cmp;
	  cmp.e = current_event;
	  cmp.prob = p/norm;
	  cmp.type = NORMAL;
	  completions.push_back(cmp);
	}

	//Reinitialize state variables
	norm = 0.0;
	p = 0.0;
	current_event = p_new_match->e;
      }

      //Find the match weight
      double v = event_ub - event_lb;
      double n = p_new_match->match_occ.size();
      double c;
      if(p_new_match->patt_global_count == 1)
	c = .05;
      else if(p_new_match->patt_global_count == 2)
	c = .5;
      else
	c = 1.0;

      double w = c*n*log(v);
      norm += w;
      //p += (w*p_new_match->prob);
      //Why? Because:
      //P(A|Gn) = P(A & Gn)/P(Gn) = (count(A & Gn)/T) / (count(Gn)/T) = count(A & Gn) / count(Gn)
      //In this case, Gn represents the matched portion of the givens.  Hence Gn maps to "match_global_count" below.  So for some set of givens AB
      //we might find patterns 10xABC, 10xABD, and 10xABE and then match_global_count for AB would be 30, and patt_global_count would be 10 for each pattern,
      //giving a probability of 33% for completions C, D, and E.
      //And we are assuming that the probabilities of patterns are conditionally independent with respect to other patterns which are not supersets of this one.
      double match_occ_count = prior_count(p_new_match->match_occ.size(), ctx.p_bounds) + p_new_match->match_global_count;
      double patt_occ_count = prior_count(p_new_match->patt_occ.size(), ctx.p_bounds) + p_new_match->patt_global_count;
      p += w*(patt_occ_count/match_occ_count);
      //p += w*(p_new_match->patt_global_count/p_new_match->match_global_count);
    }
  } else if(type == COMPLEMENT) {
    const unsigned MAX_COMPLEMENT_COMPLETIONS 20; //FIXME: expose this trait to the outside somehow, maybe make it part of the constructor
    //Generate completions based on context free event counts, within the known bounds
    //The issue here is that if the event space is unbounded in time, we could generate an infinite number of next events, and we don't want to do that.  So we have to have a maximum number of new completions generated.
    //So, keep generating completions until we reach the end of the bounds or we reach our maximum generated events.
    //FIXME: better to do this in order from most common to least common, rather than in a predetermined order like this.
    completions.clear();
    unsigned int n = 0;
    event e;
    e.p = ctx.p_bounds.lb;
    e.t = ctx.t_bounds.lb;
    while(n < MAX_COMPLEMENT_COMPLETIONS) {
      if(ctx.is_included(get_union(ctx.occ, get_occurrence(e)))) {
	completion cmp;
	cmp.e = e;
	double base_case_pos_count = base_case[e.p] + prior_count(1, ctx.p_bounds);
	double base_case_total_count = total_num_events + (prior_count(1, ctx.p_bounds)*(double(ctx.p_bounds.ub - ctx.p_bounds.lb)));
	cmp.prob = base_case_pos_count/base_case_total_count; //prior counts are important so we do not get probs of zero or 100%
	cmp.type = NORMAL;
	completions.push_back(cmp);
	n++;
      }

      if(e.p > ctx.p_bounds.ub) {
	if(e.t > ctx.t_bounds.ub) {
	  return; //No more completions can be generated; therefore no complement completion to add to the end
	}
	e.p = ctx.p_bounds.lb;
	e.t++;
      }
      e.p++;
    }
  }

  completion complement_cmp;
  complement_cmp.type = COMPLEMENT;
  complement_cmp.prob = 1.0; //probability of something other than one of these
  for(auto p_comp = completions.begin();p_comp != completions.end();p_comp++) {
    complement_cmp.prob *= (1.0 - p_comp->prob);
  }
  completions.push_back(complement_cmp);
}




