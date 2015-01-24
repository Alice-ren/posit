/*****
      pattern.hh
      Defines probability spaces over events and sets of events
******/

#include <vector>
#include <list>
#include <iostream>
#include <climits>
#include "occurrence.h"
using namespace std;

#ifndef PATTERN
#define PATTERN

/*
  A pattern is an abstraction generated from one or more occurrences with matching *absolute* positions, wrt the *relative* time between events in those occurrences.
  In this system our organizing principle is that sequences of events repeat with respect to time and that the probability of an event depends on
  how many similar sequences we have seen before per unit time.
  The probability of seeing a particular pattern at some point in time depends on whether there are events in the space which match elements of the pattern.
  But the probability is assumed to be *conditionally independent* of any events that do not match the pattern.  This assumption allows us to chain together 
  overlapping patterns to make extended inferences.
*/

typedef struct {
  vector<unsigned> p;
  vector<unsigned> dt;
  double count;
} pattern;

void print_pattern(const pattern& p);
void print_patterns(const list<pattern>& patterns);
bool operator==(const pattern& p1, const pattern& p2);

//functions related to patterns and occurrences
occurrence get_occurrence(const pattern& pat, int t_abs);
pattern get_pattern(const occurrence& occ);

//functions related to matches
typedef struct {
  pattern* p_patt;
  occurrence patt_occ;  //The whole matched pattern, as an occurrence
  occurrence match_occ; //Matched portion of patt_occ
  event e;
  double local_count; //Count for the matched pattern as stored, not including superpatterns
  double match_global_count; //Count for the matched occurrence, including counts of supersets of the matched occurrence
  double patt_global_count; //Count for the matched pattern, including counts of supersets of the matched pattern.
  //Note: global_count >= local_count always.
  double prob;
} match;

void print_match(const match& match);
void print_matches(const list<match>& matches);

bool get_suboccurrences_matching_pattern(const occurrence &occ, const pattern &patt, list<occurrence> &matches);
double count_suboccurrences_matching_pattern(const occurrence &occ, const pattern &patt);
bool get_occurrence_matches(const occurrence &occ, const pattern &patt, list<match> &matches, bool get_submatches = false); //Get all the matches
void set_global_counts(list<match> &matches); //Sets the global count of each match given that we know the local counts
void set_local_counts(list<match> &matches, occurrence oracle_occ); //sets the local count of each match given some occurrence to use to measure the global count
void make_unique(list<match> &matches); //Eliminate duplicate pattern occurrences from this list of matches
void consolidate(list<match> &matches); //Eliminate duplicate pattern occurrences from this list of matches, and sum counts
bool match_occurrence_size_descending(const match& m1, const match& m2);
bool match_occurrence_size_ascending(const match& m1, const match& m2);
//bool match_global_count_descending(const match& m1, const match& m2);
bool match_new_event_descending(const match& m1, const match& m2);

#endif
