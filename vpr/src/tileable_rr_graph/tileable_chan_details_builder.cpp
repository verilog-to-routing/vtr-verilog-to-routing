/************************************************************************
 *  This file contains a builder for the ChanNodeDetails data structure 
 *  Different from VPR rr_graph builders, this builder aims to create a 
 *  highly regular routing channel. Thus, it is called tileable, 
 *  which brings significant advantage in producing large FPGA fabrics.
 ***********************************************************************/
#include <vector>
#include <algorithm>

/* Headers from vtrutil library */
#include "vtr_assert.h"
#include "vtr_log.h"

#include "rr_graph_builder_utils.h"
#include "tileable_chan_details_builder.h"

/* begin namespace openfpga */
namespace openfpga {

/************************************************************************
 * Generate the number of tracks for each types of routing segments
 * w.r.t. the frequency of each of segments and channel width
 * Note that if we dertermine the number of tracks per type using
 *     chan_width * segment_frequency / total_freq may cause 
 * The total track num may not match the chan_width, 
 * therefore, we assign tracks one by one until we meet the frequency requirement
 * In this way, we can assign the number of tracks with repect to frequency 
 ***********************************************************************/
std::vector<size_t> get_num_tracks_per_seg_type(const size_t& chan_width, 
                                                const std::vector<t_segment_inf>& segment_inf, 
                                                const bool& use_full_seg_groups) {
  std::vector<size_t> result;
  std::vector<double> demand;
  /* Make sure a clean start */
  result.resize(segment_inf.size());
  demand.resize(segment_inf.size());

  /* Scale factor so we can divide by any length
   * and still use integers */
  /* Get the sum of frequency */
  size_t scale = 1;
  size_t freq_sum = 0;
  for (size_t iseg = 0; iseg < segment_inf.size(); ++iseg) {
    scale *= segment_inf[iseg].length;
    freq_sum += segment_inf[iseg].frequency;
  }
  size_t reduce = scale * freq_sum;

  /* Init assignments to 0 and set the demand values */
  /* Get the fraction of each segment type considering the frequency:
   * num_track_per_seg = chan_width * (freq_of_seg / sum_freq)
   */
  for (size_t iseg = 0; iseg < segment_inf.size(); ++iseg) {
    result[iseg] = 0;
    demand[iseg] = scale * chan_width * segment_inf[iseg].frequency;
    if (true == use_full_seg_groups) {
      demand[iseg] /= segment_inf[iseg].length;
    }
  }

  /* check if the sum of num_tracks, matches the chan_width */
  /* Keep assigning tracks until we use them up */
  size_t assigned = 0;
  size_t size = 0;
  size_t imax = 0;
  while (assigned < chan_width) {
    /* Find current maximum demand */
    double max = 0;
    for (size_t iseg = 0; iseg < segment_inf.size(); ++iseg) {
      if (demand[iseg] > max) {
        imax = iseg;
      }
      max = std::max(demand[iseg], max); 
    }

    /* Assign tracks to the type and reduce the types demand */
    size = (use_full_seg_groups ? segment_inf[imax].length : 1);
    demand[imax] -= reduce;
    result[imax] += size;
    assigned += size;
  }

  /* Undo last assignment if we were closer to goal without it */
  if ((assigned - chan_width) > (size / 2)) {
    result[imax] -= size;
  }

  return result;
} 

/************************************************************************
 * Adapt the number of channel width to a tileable routing architecture
 ***********************************************************************/
int adapt_to_tileable_route_chan_width(const int& chan_width, 
                                       const std::vector<t_segment_inf>& segment_infs) {
  int tileable_chan_width = 0;
  
  /* Estimate the number of segments per type by the given ChanW*/ 
  std::vector<size_t> num_tracks_per_seg_type = get_num_tracks_per_seg_type(chan_width, 
                                                                            segment_infs, 
                                                                            true); /* Force to use the full segment group */
  /* Sum-up the number of tracks */
  for (size_t iseg = 0; iseg < num_tracks_per_seg_type.size(); ++iseg) {
    tileable_chan_width += num_tracks_per_seg_type[iseg];
  }
  
  return tileable_chan_width;
}

/************************************************************************
 * Build details of routing tracks in a channel 
 * The function will 
 * 1. Assign the segments for each routing channel,
 *    To be specific, for each routing track, we assign a routing segment.
 *    The assignment is subject to users' specifications, such as 
 *    a. length of each type of segment
 *    b. frequency of each type of segment.
 *    c. routing channel width
 *
 * 2. The starting point of each segment in the channel will be assigned
 *    For each segment group with same directionality (tracks have the same length),
 *    every L track will be a starting point (where L denotes the length of segments)
 *    In this case, if the number of tracks is not a multiple of L,
 *    indeed we may have some <L segments. This can be considered as a side effect.
 *    But still the rr_graph is tileable, which is the first concern!
 *
 *    Here is a quick example of Length-4 wires in a W=12 routing channel
 *    +---------------------------------------+--------------+
 *    | Index |   Direction  | Starting Point | Ending Point |
 *    +---------------------------------------+--------------+
 *    |   0   | MUX--------> |   Yes          |  No          |
 *    +---------------------------------------+--------------+
 *    |   1   | <--------MUX |   Yes          |  No          |
 *    +---------------------------------------+--------------+
 *    |   2   |   -------->  |   No           |  No          |
 *    +---------------------------------------+--------------+
 *    |   3   | <--------    |   No           |  No          |
 *    +---------------------------------------+--------------+
 *    |   4   |    --------> |   No           |  No          |
 *    +---------------------------------------+--------------+
 *    |   5   | <--------    |   No           |  No          |
 *    +---------------------------------------+--------------+
 *    |   7   | -------->MUX |   No           |  Yes         |
 *    +---------------------------------------+--------------+
 *    |   8   | MUX<-------- |   No           |  Yes         |
 *    +---------------------------------------+--------------+
 *    |   9   | MUX--------> |   Yes          |  No          |
 *    +---------------------------------------+--------------+
 *    |   10  | <--------MUX |   Yes          |  No          |
 *    +---------------------------------------+--------------+
 *    |   11  | -------->MUX |   No           |  Yes         |
 *    +------------------------------------------------------+
 *    |   12  | <--------    |   No           |  No          |
 *    +------------------------------------------------------+
 *
 * 3. SPECIAL for fringes: TOP|RIGHT|BOTTOM|RIGHT
 *    if device_side is NUM_SIDES, we assume this channel does not locate on borders
 *    All segments will start and ends with no exception
 *
 * 4. IMPORTANT: we should be aware that channel width maybe different 
 *    in X-direction and Y-direction channels!!!
 *    So we will load segment details for different channels 
 ***********************************************************************/
ChanNodeDetails build_unidir_chan_node_details(const size_t& chan_width,
                                               const size_t& max_seg_length,
                                               const bool& force_start, 
                                               const bool& force_end, 
                                               const std::vector<t_segment_inf>& segment_inf) {
  ChanNodeDetails chan_node_details;
  size_t actual_chan_width = find_unidir_routing_channel_width(chan_width);
  VTR_ASSERT(0 == actual_chan_width % 2);
  
  /* Reserve channel width */
  chan_node_details.reserve(chan_width);
  /* Return if zero width is forced */
  if (0 == actual_chan_width) {
    return chan_node_details; 
  }

  /* Find the number of segments required by each group */
  std::vector<size_t> num_tracks = get_num_tracks_per_seg_type(actual_chan_width / 2, segment_inf, false);  

  /* Add node to ChanNodeDetails */
  size_t cur_track = 0;
  for (size_t iseg = 0; iseg < segment_inf.size(); ++iseg) {
    /* segment length will be set to maxium segment length if this is a longwire */
    size_t seg_len = segment_inf[iseg].length;
    if (true == segment_inf[iseg].longline) {
      seg_len = max_seg_length;
    } 
    for (size_t itrack = 0; itrack < num_tracks[iseg]; ++itrack) {
      bool seg_start = false;
      bool seg_end = false;
      /* Every first track of a group of Length-N wires, we set a starting point */
      if (0 == itrack % seg_len) {
        seg_start = true;
      }
      /* Every last track of a group of Length-N wires or this is the last track in this group, we set an ending point */
      if ( (seg_len - 1 == itrack % seg_len) 
        || (itrack == num_tracks[iseg] - 1) ) {
        seg_end = true;
      }
      /* Since this is a unidirectional routing architecture,
       * Add a pair of tracks, 1 INC track and 1 DEC track 
       */
      chan_node_details.add_track(cur_track, Direction::INC, iseg, seg_len, seg_start, seg_end);
      cur_track++;
      chan_node_details.add_track(cur_track, Direction::DEC, iseg, seg_len, seg_start, seg_end);
      cur_track++;
    }    
  }
  /* Check if all the tracks have been satisified */ 
  VTR_ASSERT(cur_track == actual_chan_width);
  
  /* If this is on the border of a device/heterogeneous blocks, segments should start/end */
  if (true == force_start) {
    /* INC should all start */
    chan_node_details.set_tracks_start(Direction::INC);
    /* DEC should all end */
    chan_node_details.set_tracks_end(Direction::DEC);
  }

  /* If this is on the border of a device/heterogeneous blocks, segments should start/end */
  if (true == force_end) {
    /* INC should all end */
    chan_node_details.set_tracks_end(Direction::INC);
    /* DEC should all start */
    chan_node_details.set_tracks_start(Direction::DEC);
  }

  return chan_node_details; 
}

} /* end namespace openfpga */
