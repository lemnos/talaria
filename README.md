# DESCRIPTION

talaria is a program which produces a simple dynammically filterable menu from
its input and prints the chosen option to STDOUT. It is designed to be flexible
and provides options for controlling menu placement, color themes, and
delimeters.

# Examples

```

# Produces a searchable list of items in the current directory and opens the selected ones.

> xdg-open $(ls|talaria) 

# A (very) simple launcher which remembers the last item can be written like so:

eval $(echo $PATH|xargs -I{} -d: find {}|xargs basename -a|talaria -l /tmp/launcher.hist) 

# In reality you will probably want to write a proper script with error checking, but this 
# gives you an idea of what can be done.

```

The man page contains a more thorough description and a list of options.
