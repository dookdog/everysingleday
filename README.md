# everysingleday
For the Future

## Lionel: Jungle Fury

A first-person jungle shooter. Lionel holds the Temple of the Golden Jaguar with seven weapons.

- `lionel-jungle-fury/`: the browser version. A single HTML file that runs on iPhone with nothing to install.
- `LionelJungleFuryUE/`: the Unreal Engine 5 C++ version. Open it in Unreal on a PC or Mac; see its README for building and for running it on an iPhone.

### Play in Safari with motion look

`docs/index.html` is a standalone copy of the browser game for GitHub Pages. Opened straight in Safari, it can use the phone's motion sensors: turn **MOTION LOOK** on, then physically turn around and tilt the phone to look around, 1:1.

To publish it: on GitHub, open **Settings → Pages**, set **Source** to *Deploy from a branch*, choose this branch and the **/docs** folder, then **Save**. The game appears at `https://dookdog.github.io/everysingleday/`.

After changing `lionel-jungle-fury/index.html`, rebuild the copy with `python3 tools/build_pages.py`.
