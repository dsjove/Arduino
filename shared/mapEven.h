//
//  mapEven.h
//  Infrastructure
//
//  Created by David Giovannini on 1/20/26.
//

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

template <
  typename In,
  typename Out,
  typename Acc = int64_t
>
inline Out mapEven2(
  In x,
  In in_min, In in_max,
  Out out_min, Out out_max)
{
  // Clamp input
  if (x < in_min) x = in_min;
  if (x > in_max) x = in_max;

  // Compute ranges in a wider accumulator type
  const Acc in_range  = static_cast<Acc>(in_max) - static_cast<Acc>(in_min) + 1;
  const Acc out_range = static_cast<Acc>(out_max) - static_cast<Acc>(out_min) + 1;

  const Acc x0 = static_cast<Acc>(x) - static_cast<Acc>(in_min);

  Acc bucket = (x0 * out_range) / in_range;
  if (bucket >= out_range)
  {
    bucket = out_range - 1;
  }

  return static_cast<Out>(static_cast<Acc>(out_min) + bucket);
}
