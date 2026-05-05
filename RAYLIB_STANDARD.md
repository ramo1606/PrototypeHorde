# Raylib 6.0 - conventions, standards, and style

This document summarizes and organizes the public style conventions used by raylib 6.0. It is not a literal copy of the official documentation: it is a practical reference guide, in English, for keeping code and assets consistent with the raylib style.

## 1. General philosophy

The raylib style prioritizes:

- simplicity
- readability
- consistency
- clear naming
- code that is easy to read for people new to C

The most important rule is that code should look uniform and predictable. In raylib, the name of a function, type, or parameter should already explain a good part of its intent.

## 2. Naming conventions

| Element | Convention | Example |
| --- | --- | --- |
| `#define` | `ALL_CAPS` | `#define PLATFORM_DESKTOP` |
| macros | `ALL_CAPS` | `#define MIN(a,b) (((a)<(b))?(a):(b))` |
| variables | `lowerCase` | `int screenWidth = 0;` |
| local variables | `lowerCase` | `Vector2 playerPosition = { 0 };` |
| global variables | `lowerCase` | `bool windowReady = false;` |
| constants | `lowerCase` | `const int maxValue = 8;` |
| pointers | `Type *pointer` | `Texture2D *array = NULL;` |
| `enum` | `TitleCase` | `enum TextureFormat` |
| `enum` members | `ALL_CAPS` | `PIXELFORMAT_UNCOMPRESSED_R8G8B8` |
| `struct` | `TitleCase` | `struct Texture2D` |
| `struct` members | `lowerCase` | `texture.width`, `color.r` |
| functions | `TitleCase` | `InitWindow()`, `LoadImageFromMemory()` |
| parameters | `lowerCase` | `width`, `height` |

### Practical naming rules

- Names should describe the action or data clearly.
- When proposing a new function, raylib favors explicit names over confusing abbreviations.
- A module prefix often helps group related APIs naturally.

## 3. Formatting and spacing

### Indentation

- Use **4 spaces**
- **Do not use tabs**
- Avoid trailing spaces

### Braces

Opening and closing braces stay aligned:

```c
void SomeFunction()
{
    // Code here
}
```

This also applies to longer control-flow blocks:

```c
while (!WindowShouldClose())
{

}
```

### Spaces in control flow

Always leave a space between the keyword and the opening parenthesis:

```c
if (condition) value = 0;
while (running) { }
switch (value) { }
```

### Conditions

- Conditions go inside parentheses
- Compound boolean expressions are grouped with parentheses
- Simple boolean variables do not need redundant comparisons

Correct:

```c
if ((value > 1) && (value < 50) && valueActive)
{

}
```

Avoid:

```c
if (valueActive == true)
{

}
```

### Operators

raylib uses an intentional mix:

- multiplication without spaces: `value*6`
- division without spaces: `value/4`
- addition with spaces: `value + 10`
- subtraction with spaces: `value - 5`

Examples:

```c
int product = value*6;
int division = value/4;
int sum = value + 10;
int res = value - 5;
```

### `float` values

Always use the full `x.xf` suffix:

```c
float gravity = 10.0f;
```

Avoid:

```c
float gravity = 10.f;
```

### Increments in loops

raylib prefers:

```c
for (int i = 0; i < count; i++)
{

}
```

Instead of:

```c
for (int i = 0; i < count; ++i)
{

}
```

### Ternary operator

Keep the expression clear and spaced consistently:

```c
printf("Value is 0: %s", (value == 0)? "yes" : "no");
```

## 4. Initialization and basic safety

- **Always** initialize variables when declaring them
- Avoid implicit state
- Prefer obvious and safe default values

Examples:

```c
int screenWidth = 800;
float targetFrameTime = 0.016f;
Vector2 position = { 0 };
Texture2D *texture = NULL;
```

## 5. Comments

raylib uses few comments. When they appear, they follow these rules:

- the comment goes **before** the line or block it describes
- it starts with a space and a capital letter after `//`
- it does not end with a period

Correct example:

```c
// Load player texture from disk
Texture2D texture = LoadTexture("player.png");
```

### Usage criteria

- comment on intent or a non-obvious decision
- do not comment what is already clear from the code name
- prefer clear naming over comments for obvious code

## 6. `switch` and control-flow blocks

raylib pays close attention to `switch` formatting:

```c
switch (value)
{
    case 0:
    {

    } break;
    case 2: break;
    default: break;
}
```

This keeps visual consistency and makes each case easier to scan.

## 7. Files and directories

### Directory names

Use `snake_case`:

```text
resources/models
resources/fonts
```

### File names

Use `snake_case`:

```text
main_title.png
cubicmap.png
sound.wav
```

### General rule

- do not use spaces
- do not use special characters
- use descriptive names

## 8. Resource and asset organization

raylib recommends organizing assets by context and load timing:

- group together what is loaded together
- separate assets by real in-game usage
- use names that explain what the file is without opening it

Example structure:

```text
resources/audio/fx/long_jump.wav
resources/audio/music/main_theme.ogg
resources/screens/logo/logo.png
resources/screens/title/title.png
resources/screens/gameplay/background.png
resources/characters/player.png
resources/characters/enemy_slime.png
resources/common/font_arial.ttf
resources/common/gui.png
```

### Practical criteria

- audio by type (`fx`, `music`)
- screens by context (`logo`, `title`, `gameplay`)
- common resources separated from specific ones

## 9. API and documentation style

raylib favors an API that explains itself:

- direct function names
- descriptive parameters
- easy-to-recognize types
- little ceremony

raylib documentation usually leans heavily on:

- consistent naming
- small examples
- cheatsheets
- runnable examples

In other words: clarity in code first, extra documentation only when needed.

## 10. Quick checklist

Before considering a file aligned with raylib style, check:

1. Variables are initialized
2. Indentation uses 4 spaces
3. No tabs or trailing spaces
4. Functions use `TitleCase`
5. Variables and members use `lowerCase`
6. Defines and enum values use `ALL_CAPS`
7. Files and directories use `snake_case`
8. Comments are short, before the block, and have no final period
9. Braces are aligned in blocks
10. `float` values are written as `x.xf`

## 11. Official sources

- `CONVENTIONS.md` from the official raylib repository
- the official raylib cheatsheet
- official project examples

References:

- https://github.com/raysan5/raylib/blob/master/CONVENTIONS.md
- https://www.raylib.com/cheatsheet/cheatsheet.html
- https://github.com/raysan5/raylib

