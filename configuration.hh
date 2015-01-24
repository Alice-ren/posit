/*****
configuration.h
Configuration struct and related functions to walk a tree defining a configuration space
******/

#include "occurrence.h"
#include "model.h"
#include <vector>
#include <list>
#include <algorithm>
#include <iostream>
#include <iomanip>
using namespace std;

#ifndef CONFIGURATION
#define CONFIGURATION

/*
A word on the strategy for finding predicted event densities.
The right way to think about predicting events that have not occurred, based on the ones that have, is as a configurational space over the unknown portions of time and space.  Now, one of these configurations is guaranteed to occur (if we count empty space as another configuration), so the total volume of the space is one.  In order for this strategy to work, we have to start by assuming that event densities cannot be infinite, because otherwise there is no way to normalize the parts of the configurational space so that the total volume is one.
The prediction algorithm works by attempting to generate all the possible configurations the space can be in, starting with the most likely ones.  However, any configuration we might generate has finite length, and the unknown portions of time and space are infinite, therefore every configuration we generate has its own space of possible sub-configurations.
Now if we have two configurations, and they do not contradict one another, then there is some subspace that the two configurations share.  We are double counting that part of the total configuration space.  Therefore as we generate longer and longer configurations we will want to always check whether we have generated the same one twice.
After generating some set of configurations depending on available memory and CPU resources, we can ask questions like, 'what is the most likely future event at time t0' or 'what is the probability of X between t0 and tf.' or the important question 'what is most probable event, at any point in space and future time'.
*/

/*
The term "configuration space" is used here in the sense that it is used in thermodynamics - it is an enumeration of all the possible configurations that the unknown portions of our space might be in.  Now, I've defined here a tree structure over the configurations in the space such that all the configurations listed as "subs" are strictly inside the configurations listed as "super"s.
Note that if we have some configuration AB, and we have configuration ABC, then ABC is in a subspace of AB since AB contains AB* where * is anything.  Even though AB is a sub*string* of ABC.
The tree structure is defined such that even if there is more than one possible configuration which could be a super of a new configuration, in fact there is only one unique "true super" and the others are "false supers".  Use the ok_to_add_configuration function to make the distinction.
Also note that many of the configurations in the space overlap one another, thus we cannot simply add up the probability of each configuration and have it add up to one without taking into account false supers and overlapping subspaces.  To do that, call the calculate_eff_prob function which calculates "effective" probability accounting for overlapping configurations.
*/

typedef struct configuration configuration; //forward declaration, C++ is showing its age

class configuration_space {
public:
	configuration_space(unsigned mem_constraint);
	~configuration_space();
	completion most_probable_event() const;  //Most probable event which is not a given (it would then have probability 100%)
	void given(const event &e); //Tell the configuration space, this event is a given (probability 100%)
	void exclude(const event &e); //Tell the configuration space, this event is definitely not going to happen (probability 0%)
	void predict(const model &m, event_bounds t_bounds, event_bounds p_bounds, unsigned max_depth, double predict_time_limit); //Properly sets the state for future queries to most_probable_event().  Otherwise you just get flat even "white noise"
	//Note that the time variable here is in realtime seconds, so set for .1 to run for 100msec for example.
	//FIXME: add some logic for if we need events within some window in model time, or if we want to somehow link real time and model time
	//Note: We could potentially streamline the function calls at some cost to clarity by making given/filter an input variable and always calling predict() after being
	//given some new event.
private:
	configuration root;
};

#endif