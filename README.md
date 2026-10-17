# Hearts

Hearts against three bots for Sailfish OS.

- Full rules: passing left, right, across and holding, the two of clubs leads,
  no points on the first trick, hearts broken, and shooting the moon
- A game to 100 points, or a quick one to 50; the lowest score wins
- Bots in three levels: Easy plays at random, Medium passes the dangerous cards and ducks
  under the trick, Hard keeps track of who is out of which suit and plays every card
  through to the end of the hand over many guessed deals
- Tricks and the points taken shown at a speed you choose, and won and lost games per level

Needs Sailfish OS 5.1 or newer, on aarch64 or armv7hl.

## Building

The card code shared with the other card games is the
[sailfish-cards](https://github.com/klahr/sailfish-cards) submodule, so clone with
`git clone --recursive`, or run `git submodule update --init` in an existing clone.

With the [Sailfish SDK](https://docs.sailfishos.org/Tools/Sailfish_SDK/), from the source directory:

```
sfdk -c target=SailfishOS-5.1.0.11-aarch64 build
sfdk -c target=SailfishOS-5.1.0.11-armv7hl build
```

The RPM ends up in `RPMS/`. Clean the build files between targets.

The rules and the bots are plain C++ without Qt, with tests that run on any desktop:

```
make -C tests
```

They also run when the RPM is built.

## License

GPLv3, see [LICENSE](LICENSE).
