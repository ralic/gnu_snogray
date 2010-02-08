// sample-set.h -- Set of samples
//
//  Copyright (C) 2010  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef __SAMPLE_SET_H__
#define __SAMPLE_SET_H__

#include <vector>

#include "sample-gen.h"


namespace snogray {


// A set of samples.  There are zero or more channels, each holding the
// same number of samples.  Each channel has samples generated by the same
// generator, but the channels are explicitly de-correlated from each other
// by randomly shuffling the samples in each channel after generation.
//
class SampleSet
{
public:

  // A single sample channel.  Sample channels are typed, so they can only
  // contain a single type of sample (the available types of samples are
  // restricted to whatever the sample generator can generate).
  //
  template<typename T>
  class Channel
  {
  public:

    // Default constructor zero-initializes, hopefully resulting in a
    // segfault if an otherwise uninitialized channel is used by mistake.
    //
    Channel () {}

    // Copy constructor
    //
    Channel (const Channel &from)
      : size (from.size), base_offset (from.base_offset),
	num_total_samples (from.num_total_samples)
    {}

    // Number of sub-samples this channel contains.  There are this many
    // sub-samples per top-level sample.
    //
    unsigned size;

  private:

    friend class SampleSet;

    // Normal constructor.  This is private, as BASE_OFFSET is an
    // implementation detail.
    //
    Channel (unsigned _base_offset, unsigned _size, unsigned _num_total_samples)
      : size (_size), base_offset (_base_offset),
	num_total_samples (_num_total_samples)
    {}

    // Offset of our first sample in the appropriate sample vector of
    // our SampleSet.
    //
    unsigned base_offset;

    // Number of total samples generated for this channel.  This should be
    // at least SIZE * NUM_TOP_LEVEL_SAMPLES.  In the case that it's
    // greater, then NUM_TOTAL_SAMPLES - SIZE * NUM_TOP_LEVEL_SAMPLES extra
    // samples will end up being unused; this should be OK as the samples
    // are in random order.
    //
    unsigned num_total_samples;
  };

  // A vector of channels, for cases where we need more than one.
  //
  template<typename T>
  struct ChannelVec : public std::vector<Channel<T> >
  {
  };


  // A reference to a single top-level sample in a sample-set.
  //
  // This is just a convenient package to hold the set and a
  // sample-number.
  //
  class Sample
  {
  public:

    Sample (const SampleSet &_set, unsigned _sample_num)
      : set (_set), sample_num (_sample_num)
    { }

    // Return sub-sample SUB_SAMPLE_NUM, from the sample-channel
    // CHANNEL.  SUB_SAMPLE_NUM may be omitted if there's only one
    // sample per top-level sample.
    //
    template<typename T>
    T get (const Channel<T> &channel, unsigned sub_sample_num = 0) const
    {
      return set.get<T> (channel, sample_num, sub_sample_num);
    }

    // Return an iterator pointing to the first sub-sample from
    // sample-channel CHANNEL.
    //
    template<typename T>
    typename std::vector<T>::const_iterator
    begin (const Channel<T> &channel) const
    {
      return set.begin<T> (channel, sample_num);
    }

    // Return an iterator pointing just past the end of the last
    // sub-sample for top-level sample SAMPLE_NUM from the sample channel
    // CHANNEL.
    //
    template<typename T>
    typename std::vector<T>::const_iterator
    end (const Channel<T> &channel) const
    {
      return set.end<T> (channel, sample_num);
    }

    // The SampleSet this sample is from.
    //
    const SampleSet &set;

    // The top-level sample-number of this sample in SET.
    //
    unsigned sample_num;
  };


  // Construct a new sample set, using the sample generator GEN and
  // RANDOM as a source of randomness.
  //
  SampleSet (unsigned _num_samples, const SampleGen &_gen, Random &_random)
    : num_samples (_num_samples), gen (_gen), random (_random)
  {}


  // Return sample for top-level sample SAMPLE_NUM, and sub-sample
  // SUB_SAMPLE_NUM, from the sample channel CHANNEL.  SUB_SAMPLE_NUM
  // may be omitted if there's only one sample per top-level sample.
  //
  template<typename T>
  T get (const Channel<T> &channel,
	 unsigned sample_num, unsigned sub_sample_num = 0)
    const
  {
    return sample<T> (channel.base_offset)
      [sample_num * channel.size + sub_sample_num];
  }

  // Return an iterator pointing to the first sub-sample for top-level
  // sample SAMPLE_NUM from the sample channel CHANNEL.
  //
  template<typename T>
  typename std::vector<T>::const_iterator
  begin (const Channel<T> &channel, unsigned sample_num) const
  {
    return sample<T> (channel.base_offset) + (sample_num * channel.size);
  }

  // Return an iterator pointing just past the end of the last
  // sub-sample for top-level sample SAMPLE_NUM from the sample channel
  // CHANNEL.
  //
  template<typename T>
  typename std::vector<T>::const_iterator
  end (const Channel<T> &channel, unsigned sample_num) const
  {
    return begin (channel, sample_num) + channel.size;
  }

  // Allocate a new sample-channel in this set, containing
  // NUM_SUB_SAMPLES samples per top-level sample (which defaults to 1).
  // The type of sample must be specified as the first template
  // parameter.
  //
  template<typename T>
  Channel<T> add_channel (unsigned num_sub_samples = 1)
  {
    // There's NUM_SUB_SAMPLES per top-level sample, so calculate the
    // total number of samples for this channel.
    //
    unsigned num_total_samples = num_samples * num_sub_samples;

    // Some sample generators may want a slightly different number of
    // samples.
    //
    num_total_samples = gen.adjust_sample_count<T> (num_total_samples);

    // Adjust NUM_SUB_SAMPLES so that NUM_SAMPLES * NUM_SUB_SAMPLES is as
    // close to NUM_TOTAL_SAMPLES as possible (it's not possible to change
    // NUM_SAMPLES).  Any difference will end up being unused.
    //
    num_sub_samples = num_total_samples / num_samples;

    // Add enough room to our sample array for all the samples.
    //
    unsigned base_sample_offset = add_sample_space<T> (num_total_samples);

    return _add_channel<T> (Channel<T> (base_sample_offset, num_sub_samples,
					num_total_samples));
  }

  // Allocate and return a vector of channels in this set, each
  // containing NUM_SUB_SAMPLES samples per top-level sample.  The type
  // of sample must be specified as the first template parameter.
  //
  template<typename T>
  ChannelVec<T> add_channel_vec (unsigned size, unsigned num_sub_samples)
  {
    ChannelVec<T> vec (size);
    for (unsigned i = 0; i < size; i++)
      vec[i] = add_channel<T> (num_sub_samples);
    return vec;
  }

  // Removes all samples from this sample-set, invalidating any previously
  // created channels.  To subsequently generate more samples, new channels
  // must be added.
  //
  void clear ();

  // Compute a completely new set of sample values in all channels.
  //
  void generate ();

  // Number of top-level samples.
  //
  unsigned num_samples;

private:

  // Returns an iterator pointing into sample space for samples of type
  // T at offset OFFSET.
  //
  template<typename T>
  const typename std::vector<T>::iterator sample (unsigned offset);
  template<typename T>
  const typename std::vector<T>::const_iterator sample (unsigned offset) const;
  
  // Add enough entries to the end of our sample table for samples of
  // type T to hold NUM samples, and return the offset of the
  // first entry so allocated.
  //
  template<typename T>
  unsigned add_sample_space (unsigned num);
  
  // Add CHANNEL to the appropriate vector of channels, and return it.
  //
  template<typename T>
  Channel<T> _add_channel (const Channel<T> &chan);

  std::vector<float> float_samples;
  std::vector<UV> uv_samples;

  std::vector<Channel<float> > float_channels;
  std::vector<Channel<UV> > uv_channels;

public:

  // Sample generator used to generate the actual sample values.
  //
  const SampleGen &gen;

  // Source of randomness to use when generating samples.
  //
  Random &random;
};


//
// Declarations for specialized SampleSet::add_sample_space methods.
//

template<>
unsigned
SampleSet::add_sample_space<float> (unsigned num_samples);

template<>
unsigned
SampleSet::add_sample_space<UV> (unsigned num_samples);

//
// Specializations of SampleGen::sample for supported sample types.
//

template<>
inline const std::vector<float>::iterator
SampleSet::sample<float> (unsigned offset)
{
  return float_samples.begin() + offset;
}

template<>
inline const std::vector<UV>::iterator
SampleSet::sample<UV> (unsigned offset)
{
  return uv_samples.begin() + offset;
}

template<>
inline const std::vector<float>::const_iterator
SampleSet::sample<float> (unsigned offset) const
{
  return float_samples.begin() + offset;
}

template<>
inline const std::vector<UV>::const_iterator
SampleSet::sample<UV> (unsigned offset) const
{
  return uv_samples.begin() + offset;
}

//
// Specializations of SampleGen::_add_channel for supported sample types.
//
  
template<>
inline SampleSet::Channel<float>
SampleSet::_add_channel (const Channel<float> &chan)
{
  float_channels.push_back (chan);
  return chan;
}
  
template<>
inline SampleSet::Channel<UV>
SampleSet::_add_channel (const Channel<UV> &chan)
{
  uv_channels.push_back (chan);
  return chan;
}


}

#endif // __SAMPLE_SET_H__
