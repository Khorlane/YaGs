# Experience Calculation

YaGs calculates the experience awarded for killing a mobile from the mobile's level, the player's level, and the `Exp` adjustment in the mobile record.

## Constants

```c
#define BASE_MOB_XP 50 // Base mob xp per level
#define BASE_PLAYER_XP 1000 // Base player xp per level
```

## Mobile Experience Award

First calculate the mobile's base experience:

```text
Base XP = Mobile Level * BASE_MOB_XP
```

Next, calculate how far the player is above the mobile:

```text
Level Difference = Player Level - Mobile Level
```

Use the level difference to determine how much of the base experience the player receives:

| Player Levels Above Mobile | Base XP Awarded |
| ---: | ---: |
| 2 or fewer | 100% |
| 3 | 80% |
| 4 | 60% |
| 5 | 40% |
| 6 | 20% |
| 7 or more | 0% |

The percentage can also be calculated directly:

```text
XP Percentage = clamp(100 - ((Level Difference - 2) * 20), 0, 100)
```

Finally, add the `Exp` value from the mobile record:

```text
Adjusted XP = Base XP * XP Percentage / 100
Final XP = Adjusted XP + Mobile.Exp
```

`Mobile.Exp` may be positive, zero, or negative. It is added after the level-based percentage is applied.

All current experience values are integers. Integer division discards any fractional portion of the adjusted experience.

## Mobile Experience Examples

### Level 10 player kills a level 5 mobile with `Exp: 15`

```text
Base XP = 5 * 50 = 250
Level Difference = 10 - 5 = 5
XP Percentage = 40%
Adjusted XP = 250 * 40 / 100 = 100
Final XP = 100 + 15 = 115
```

### Level 10 player kills a level 1 mobile with `Exp: 0`

```text
Base XP = 1 * 50 = 50
Level Difference = 10 - 1 = 9
XP Percentage = 0%
Adjusted XP = 0
Final XP = 0 + 0 = 0
```

### Level 10 player kills a level 10 mobile with `Exp: 25`

```text
Base XP = 10 * 50 = 500
Level Difference = 10 - 10 = 0
XP Percentage = 100%
Adjusted XP = 500
Final XP = 500 + 25 = 525
```

## Opponent Difficulty

The following descriptions and colors correspond to the difference between the mobile's level and the player's level. These descriptions are not currently part of the experience award calculation.

| Mobile Level Minus Player Level | Message | Color |
| ---: | --- | --- |
| -7 or less | Don't Bother | Green |
| -6 | Way Too Easy | Green |
| -5 | Very Easy | Green |
| -4 | Easy | Cyan |
| -3 | No Problem | Cyan |
| -2 | A Worthy Opponent | Cyan |
| -1 | You Might Win | Yellow |
| 0 | Tough Fight | Yellow |
| 1 | Lots Of Luck | Yellow |
| 2 | Bad Idea | Red |
| 3 or more | You are Mad | Red |

## Player-Level Progression Model

The player-level model calculates the total experience required for any player level. The original worksheet displayed levels 1 through 60 as examples, but level 60 is not a maximum. The model is separate from the mobile experience award.

For player level `L`, starting with level 1 at zero experience:

```text
Base Experience(1) = 0
Base Experience(L) = Base Experience(L - 1) + (L * 1000)

Log Level(L) = log10(L + 20)

Additional Experience(L) =
  Base Experience(L) ^ Log Level(L) * (L / 10000)

Total Experience(L) =
  round(Base Experience(L) + Additional Experience(L))

Experience To Gain(L) =
  Total Experience(L) - Total Experience(L - 1)
```

The total is rounded to the nearest whole experience point. Player experience is stored as a 64-bit integer because the level-60 requirement exceeds the range of a signed 32-bit integer.

The base-experience recurrence can also be written as:

```text
Base Experience(L) = 1000 * ((L * (L + 1) / 2) - 1)
```

The estimated number of equal-level mobile kills is:

```text
Estimated Kills(L) = Experience To Gain(L) / (L * BASE_MOB_XP)
```

This estimate assumes the player receives the full base award from each mobile and that each mobile has an `Exp` adjustment of zero.