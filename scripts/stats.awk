#!/bin/awk -f

/^size:/ {
   split($2, nums, "/")
   sum_size += nums[1]
   div_sum_size += nums[2]
}

/^fixed features:/ {
   split($3, nums, "/")
   sum_fixed += nums[1]
   div_sum_fixed += nums[2]
}

/^relVolume\*:/ {
   split($2, nums, "%")
   sum_volume += nums[1]
   cnt_volume++
}

END {
   printf("avg #any features: %.1f%%\n", (sum_size/div_sum_size)*100)
   if (div_sum_fixed > 0) printf("avg #fixed features: %.1f%%\n", (sum_fixed/div_sum_fixed)*100)
   if (cnt_volume > 0) printf("avg volume: %.2f%%\n", sum_volume/cnt_volume)
}
