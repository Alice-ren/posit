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
double model::sample_size(const pattern& p) {
  return total_num_events - p.total_events_at_creation + 1.0;  //might be wrong - time based not event based?
}

double model::local_prob(const context& cxt, const pattern& p, unsigned t_abs) const {
  occurrence patt_occ - get_occurrence(p, t_abs);
  if(get_intersection(cxt.exclude, patt_occ).empty() && (!cxt.single_valued || is_single_valued(get_union(cxt.given, patt_occ)))) {
    return p.count/sample_size(p);
  }
  else
    return 0.0;
}

double model::global_prob(const context& cxt, const pattern& p, unsigned t_abs) const {
  double lp = local_prob(cxt, p, t_abs);
  double gp = 0.0;
  if(lp != 0.0) {
    for(vector<patt_link>::const_iterator p_super = p.super_patterns.begin();p_super != p.super_patterns.end();p_super++) {
      gp += global_prob(cxt, *(p_super->p_patt), t_abs - p_super->sub_pattern_t_offset);
    }
  }

  return gp;
}

double model::prob(const context& cxt, const occurrence& occ) const {
  //Assume: P(X|G1 & G2 &...) = P(X|G1)P(X|G2)P(X|Gn)/P(X|G1^G2)..P(X)^m
  //Constraint is that each event occurs net one time
  //To Do:  Prove the above

  list<patt_link> terms;
  //Start with the base level
  for(vector<pattern>::const_iterator p_patt = base_level_patterns.begin();p_patt != base_level_patterns.end();p_patt++) {
    get_pattern_matches(occ, *p_patt, terms);
  }

  list<
  make_unique(terms);
  
}

/* TO DO: this no longer compiles
   Also, we now want to revert to computing via the method of multiplied conditionally independent probabilities.

   TO DO: Create a new completions object which has an iterator of some kind so that we can run a simple loop
   through all the possible next events - including the ones which are just heat bath events.
*/
void model::get_first_order_completions(const context& cxt, list<completion> &completions) const {
  //Find all matches to the context which contain at least one event not in ctx.occ
  //Call this set of matches strict_matches.

  //Find all matches to the context.
  list<match> all_matches;
  for(list<pattern>::const_iterator p_patt = patterns.begin();p_patt != patterns.end();p_patt++)
    get_occurrence_matches(cxt.occ, *p_patt, all_matches);

  //Create a second set with only matches which contain at least one event not in ctx.occ
  //Call this strict_matches.
  list<match> strict_matches;
  for(auto p_match = all_matches.begin();p_match != all_matches.end();p_match++) {
    if(!is_sub_occurrence(ctx.occ, p_match->match_occ))
      strict_matches.push_back(*p_match);
  }
    
  //Take union(strict_matches.match_occ) = strict_matches_occ.
  occ strict_matches_occ;
  for(auto p_match = strict_matches.begin();p_match != strict_matches.end();p_match++)
    strict_matches_occ = get_union(strict_matches_occ, p_match->match_occ);
  
  //Now strict_matches_occ - context.occ = new_events_occ, which is the union of the set of possible predicted events
  //Every match to an event in new_events_occ is a subset of strict_matches_occ
  occ new_events_occ = subtract(strict_matches_occ, ctx.occ);
  
  //However, strict_matches is not complete in the sense that to find the global count of every element
  //of strict_matches, we need every match that overlaps with strict_matches_occ.  Call it complete_matches.
  list<match> complete_matches;
  for(auto p_match = all_matches.begin();p_match != all_matches.end();p_match++) {
    if(get_intersection(p_match->match_occ, strict_matches_occ).size() != 0)
      complete_matches.push_back(*p_match);
  }

  //Now for each event e in new_events_occ, find all the matches to it from the set of strict_matches.
  //Call it event_strict_matches[e].

  //Now use the recursive algorithm to cross every element of event_strict_matches[e] with each other recursively to
  //obtain the set of match_terms for e.  Generate a list sub_matches by taking the union of every
  //element of match terms for every e.  Generate a list of match_terms[e] for every e which points to
  //an element of sub_matches[i]

  //For each element of sub_matches, find the global count by referencing to complete_matches.

  //For each match in sub_matches, find the size of the sample space.  Divide the global count
  //by the size of the sample space to obtain sub_matches[i].prob

  //Multiply the terms in match_terms[e] together to obtain prob[e].  Use prob[e] to populate completions[e].

  /*
    Question we are asking: what is the probability of getting pattern p at this location in time?
    We assume that that probability is the same as the frequency of p in historical data, which is count(p)/(size(H) - size(p))
    Now let's say that we split H into H1 and H2.  Now, really, the true probability of p is the same as it was before.
    But when we divided H into H1 and H2, there is a chance that we destroyed a sample of p, which would reduce the count.
    Therefore our formula for P(p) is now slightly wrong.
    But let's say we detected the loss of p, and we introduced a new pattern p to make up for the loss.  Then of course, to
    make the counts add up correctly, we introduced negative counts for the overlap of (p,H1) and (p,H2).  Then our calculated
    P(p) would still be correct.  Doing this for *all* such patterns might be tricky.  Maybe, I can always divide a pattern
    into 2 patterns which each have one event the other doesn't, then a third with the intersection.  Of course there are n(n-1) ways
    to do that, where n is the number of events in the pattern.

    The other question is, what happens when we blow away a pattern but then we find another example such that if we had
    kept the first one, we would have realized the pattern was important?  In other words the above handles *accidental* deletion
    of a pattern, but what about *intentional* deletion.  Well, in that case, we will never ask for the count, we will multiply
    two subpatterns to get it.

    Another case is, we create a new pattern at some time step after the beginning, but the pattern has never occurred before.
    In that situation the sample space would be unaffected.
    
    Another situation is, we subdivide a pattern at one point having found it to be a combination of two subs, but then we
    recreate it later.  In that situation the count would be wrong.  We can make a guess though as to the true prior
    count by multiplication.
    
    The idea occurs that perhaps this is the wrong way to store patterns- because what I am really doing is a sort of awkward
    form of lossy data compression.  But, arithmetic coding allows you to encode the pattern given the probability; I want 
    to encode the probability given the pattern, for patterns with the highest probability.
    
    But of course we could generate a tree, from the bottom up, starting with A,B,C,D then recording tuples of AB, AC, BD
    wherever they are not c.i., and so on.  So maybe I could have a list of bottom-level and top-level patterns, or maybe
    a pattern tree, although of course it would only be for matching faster since counts would not add up.  Also it
    would make finding global counts much faster.

    But with or without a tree, we can still construct the following functions: 
  */

  
#if 0
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
#endif
}

