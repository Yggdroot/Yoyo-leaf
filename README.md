# Yoyo-leaf
Yoyo-leaf is an awesome command-line fuzzy finder.

## Usage

```
usage: yy [options]

  Layout
    --border [BORDER[:STYLE]]   Display the T(top), R(right), B(bottom), L(left) border. BORDER
                                is a string combined by 'T', 'R', 'B' or 'L'. If BORDER is not
                                specified, display borders all around. STYLE is 1, 2, 3 and 4
                                which denotes "[─,│,─,│,╭,╮,╯,╰]",
                                "[─,│,─,│,┌,┐,┘,└]",
                                "[━,┃,━,┃,┏,┓,┛,┗]",
                                "[═,║,═,║,╔,╗,╝,╚]" respectively.
    --border-chars=<CHARS>      Specify the character to use for the top/right/bottom/left
                                border, followed by the character to use for the
                                topleft/topright/botright/botleft corner. Default value is
                                "[─,│,─,│,╭,╮,╯,╰]"
    --height=<N[%]>             Display window with the given height N[%] instead of using
                                fullscreen.
    --margin=<MARGIN>           Specify the width of the top/right/bottom/left margin. MARGIN is
                                a list of integers or percentages. The list can have 1, 2 and 4
                                elements. For example, [10]: margins all around are 10.
                                [10,20%]: top and bottom margin are 10, left and right margin
                                are 20% of the terminal width.
    -r, --reverse               Display from the bottom of the screen to top.

  Search
    --sort-preference=<PREFERENCE>
                                Specify the sort preference to apply, value can be [begin|end].
                                (default: end)

alias: leaf
```

## License
This software is released under the Apache License, Version 2.0 (the "License").

