**Detection.**

| On the 20 photos | Result |
| --- | ---: |
| Balls found with the right colour, centre inside the labelled ball | 28 / 28 |
| Balls missed | 0 |
| False detections | 0 |
| Empty scenes with no detection | 3 / 3 |
| Images with every ball found and nothing else | 20 / 20 |

**Position and size.** Reported circles against the labelled circles:

| Colour | Balls | Centre error, median (px) | Centre error, max (px) | Radius error, median (px) | Radius error, max (px) | Circle IoU, median | Circle IoU, min |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| orange | 11 | 6.1 | 14.2 | 2.0 | 10 | 0.83 | 0.59 |
| blue | 10 | 1.8 | 6.1 | 2.0 | 4 | 0.87 | 0.74 |
| yellow | 7 | 4.1 | 18.0 | 1.0 | 7 | 0.85 | 0.69 |
| all | 28 | 4.1 | 18.0 | 2.0 | 10 | 0.85 | 0.59 |

**What the masks catch.** Share of each labelled disc covered by the component that is kept, median over the balls of a colour:

| Colour | Balls | Whole disc | Upper half | Lower half |
| --- | ---: | ---: | ---: | ---: |
| orange | 11 | 38 % | 5 % | 64 % |
| blue | 10 | 52 % | 65 % | 45 % |
| yellow | 7 | 51 % | 85 % | 14 % |

**Effect of the empirical corrections.** Orange and yellow balls, before and after (a positive vertical offset means too low in the image):

| Colour | Estimate | Centre error, median (px) | Vertical offset, mean (px) | Radius error, mean (px) | Circle IoU, median |
| --- | --- | ---: | ---: | ---: | ---: |
| orange | bounding box of the mask | 11.0 | +11.5 | -4.5 | 0.66 |
| orange | after the corrections (output) | 6.1 | +3.7 | -0.4 | 0.83 |
| yellow | bounding box of the mask | 10.0 | -13.1 | -2.0 | 0.67 |
| yellow | after the corrections (output) | 4.1 | -6.0 | +1.9 | 0.85 |

**Sensitivity to brightness.** Same photos with every RGB sample multiplied by a gain and clipped to 255 (a simulated exposure change, not new photos):

| Gain | Balls found | Orange | Blue | Yellow | False detections | Images fully correct |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 50 % | 18 / 28 | 10 / 11 | 8 / 10 | 0 / 7 | 4 | 13 / 20 |
| 60 % | 20 / 28 | 10 / 11 | 10 / 10 | 0 / 7 | 4 | 13 / 20 |
| 70 % | 20 / 28 | 10 / 11 | 10 / 10 | 0 / 7 | 3 | 13 / 20 |
| 80 % | 25 / 28 | 10 / 11 | 10 / 10 | 5 / 7 | 3 | 15 / 20 |
| 90 % | 27 / 28 | 11 / 11 | 10 / 10 | 6 / 7 | 1 | 18 / 20 |
| 100 % | 28 / 28 | 11 / 11 | 10 / 10 | 7 / 7 | 0 | 20 / 20 |
| 110 % | 27 / 28 | 11 / 11 | 9 / 10 | 7 / 7 | 0 | 19 / 20 |
| 120 % | 27 / 28 | 11 / 11 | 10 / 10 | 6 / 7 | 3 | 16 / 20 |
| 130 % | 26 / 28 | 10 / 11 | 10 / 10 | 6 / 7 | 4 | 14 / 20 |
| 140 % | 26 / 28 | 10 / 11 | 10 / 10 | 6 / 7 | 5 | 13 / 20 |
| 150 % | 26 / 28 | 10 / 11 | 10 / 10 | 6 / 7 | 7 | 12 / 20 |
