#pragma once

// Ensures an even distribution for all values.
inline long mapEven(
    long x,
    long in_min, long in_max,
    long out_min, long out_max)
{
  if (x < in_min) x = in_min;
  if (x > in_max) x = in_max;
  long in_range = in_max - in_min + 1;
  long out_range = out_max - out_min + 1;
  long x0 = x - in_min;
  long bucket = (x0 * out_range) / in_range;
  if (bucket >= out_range) bucket = out_range - 1;
  return out_min + bucket;
}
