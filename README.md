# symlink-track
**symlink-track** is a tool (but may be considered a toy), which tracks and
display a symbolic link.

## Example

First, create our very difficult symlink structure:

```shell
ln -s b a
ln -s c b
# ...
ln -s y x
ln -s z y

# And finally, a file :-)
touch z
```

In such case, calling

```shell
symlink-track a
```

will display a very long line, so I include only the beginning:

```
a -> b -> c ->
```

and ending:

```
-> x -> y -> z (normal file)
```

## Features

 * Maximum hop count - a hop is resolving a symbolic link and jumping to its
   target. This has two advantages - prevents infinite recursion and provides
   some more control over everything.
 * More incoming

## TODO list

 * Use Unicode arrow (yes, it exists)
 * Detect symlink loops
