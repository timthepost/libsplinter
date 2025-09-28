# What's This?

A simple way to test the theme build based on what's declared in `./mod.ts`,
which defines the remote theme properties. Running `deno task build` in this
directory should result in a successful build with no broken links.

## What If There Are Broken Links?

If broken links are reported it's because:

1. Remote links broke (external documentation links on
   [cushytext.deno.dev](https://cushytext.deno.dev)),
2. The theme didn't generate correctly, but somehow didn't throw an exception
   while doing it incorrectly,
3. A combination of 1 and 2.

The files aren't actually 'remotely' loaded; they're just used in-place from
`src/`, which is the base of the `files[]` array. If they're broken in this
test, that means they're probably broken in `src/` too. Double-check to make
sure the file is being properly included.

It could also mean something that the nav links to didn't generate, which shows
up on every page, and produces a lot of broken links. Check generator functions
first. If you need help figuring out a bug, head to our
[Discord Server](https://discord.gg/ETx8S8cWwH).

At some point this and other tests will be automated.
