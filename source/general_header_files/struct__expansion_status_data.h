#ifndef __struct__expansion_status_data_h
#define __struct__expansion_status_data_h

struct expansion_status_data {
  bool at_least_one_is_expanded : 1;
  bool at_least_one_is_collapsed : 1;
  bool at_least_one_imd_ch_is_exp : 1;
};

#endif
