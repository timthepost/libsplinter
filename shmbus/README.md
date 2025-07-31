# shmbus - Original Ancestor Of Libsplinter

_Forked off of initial implementation 7/15/2025 to serve as a reference copy.
Included in hopes it's useful._

Shmbus is very similar to splinter in its functionality with the current main
difference of it only supporting memory-backed mapping (it can not use
persistent storage). In fact, splinter started out as shmbus, forked off, and
grew a bit more toward an LLM runtime.

Shmbus also has the most basic version of the envelope protocol that was
developed, which allows one to transplant the core functionality into something
else, without having to separate it from all of the later implementation
details.

It's here as a building block for any future uses it might have, without a lot
of evolution in other directions to strip away before it can be included.

Use it any way it is useful, hopefully in good health; if not, hopefully at
least in good spirits!
