# posit
Posit is a machine learning algorithm.  It chooses the most advantageous output at each point in time based on observed data.  Use at your own risk!<br>
Note that currently this library is immature/nonfunctional.  Check back later.

TO DO LIST:
-Revise model.cc to work with the move to pattern instead of occurrence<br>
-Add a pointer back to the model in model_node and revise the get_supers() and get_subs() functions to have the same return format<br>
-Smooth out the members of model_node so that we can make assurances about reference counting, iterator validity, and leave no dangling pointers or leak any memory.<br>
-configuration space code does not have a way to truncate the current set of givens before the fulcrum node; we might want to have that.<br>
-write a partial delink function so that we can delink parts of the training set without delinking the whole training set.<br>
-write the link_node() function<br>
-write a distribute_counts() function<br>
-write the optimize() function<br>
