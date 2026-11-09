# everysingleday
For the Future

## Lily Leap

`lily-leap/index.html` is a frog platformer that runs in a browser, built for iPhone. Hop across flowers and lily pads and eat every fly in the pond. Open the file in Safari (or host it anywhere) to play. It has touch controls on the screen, and keyboard controls on a computer.

### Sending it to friends

`lily-leap/LilyLeap.html` is one self-contained file (about 134 KB) with the fonts built in. It works offline with nothing else to download. To rebuild it after changing `index.html`, run `python3 lily-leap/build_standalone.py`.

## Lily Leap for Unreal Engine 5

`LilyLeapUnreal/` is the same game rebuilt in Unreal Engine 5 in C++, with a 3D frog, flowers and flies built from code. It has not been compiled yet. See `LilyLeapUnreal/README.md` for how to build it and put it on an iPhone.

### Put Lily Leap on your iPhone home screen (no Mac needed)

The `docs/` folder is an installable version of the game: full screen, its own icon, and it works offline once loaded. It's built by `python3 lily-leap/build_standalone.py`.

1. Turn on GitHub Pages once: on github.com, open this repo's **Settings → Pages**. Under **Build and deployment**, set **Source** to **Deploy from a branch**, choose the branch that holds `docs/` and the **/docs** folder, then **Save**.
2. After about a minute, open `https://dookdog.github.io/everysingleday/` in **Safari** on the iPhone.
3. Tap **Share**, then **Add to Home Screen**.
