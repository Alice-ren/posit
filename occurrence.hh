/*****
      occurrence.hh
      event class, occurrence class (typedef'd list of events) and some helper functions.
******/

#include <list>
#include <vector>
#include <iostream>

using namespace std;

#ifndef OCCURRENCE
#define OCCURRENCE

//An event is a complete piece of data as it is received, not abstracted as a pattern, comprised as a position p
//(which can be thought of as an event type, since we do not assume position shift invariance) taking place at some time t.
typedef struct {
  unsigned p;
  int t;
} event;

//This and the event struct above will naturally change as we implement continuous coordinates at some point later.
typedef struct {
  unsigned ub; //upper bound
  unsigned lb; //lower bound
} event_bounds;

//Comparison operators over events - sort by time first then position.  Events can be placed in a unique order.
bool operator<(const event& t1, const event& t2);
bool operator==(const event& t1, const event& t2);
bool operator!=(const event& t1, const event& t2);
void print_event(const event& e);

//An occurrence is a set of events, each represented with respect to absolute time
typedef vector<event> occurrence;

void print_occurrence(const occurrence& occ);
void print_occurrences(const list<occurrence>& occurrences);
occurrence subtract(const occurrence &occ, const occurrence &sub); //Returns the events for which event is in occ but not in sub
bool is_sub_occurrence(const occurrence &occ, const occurrence &sub);
occurrence get_intersection(const occurrence& occ1, const occurrence& occ2);
occurrence get_union(const occurrence& occ1, const occurrence& occ2);
bool is_single_valued(const occurrence& occ);  //with respect to time
bool operator==(const occurrence& t1, const occurrence& t2);
occurrence get_occurrence(const event& e);

//A context is a set of assumptions we are making about surrounding events.
typedef struct {
  occurrence givens;      //We are assuming that these events have occurred
  occurrence exclusions;  //We are assuming that these events have not occurred
  event_bounds p_bounds;  //upper and lower bound on position
  event_bounds t_bounds;  //upper and lower bound on time
  bool single_valued;     //true if we are only allowed one event at each moment in time.
                          //FIXME: replace with a more generalized constraint; only two events, or one event of each of these 2 types, etc.
} context;

#endif
