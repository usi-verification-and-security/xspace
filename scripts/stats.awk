#!/bin/awk -f

function assert(condition, string) {
    if (!condition) {
        printf("%s:%d: assertion failed: %s\n", FILENAME, FNR, string) > "/dev/stderr"
        _assert_exit = 1
        exit 1
    }
}

/^-> expected output:/ {
   expected = $4
}

/^computed output:/ {
   computed = $3
   assert(expected != "" && computed != "")
   total_cnt++
   if (computed == expected) correct_cnt++
}

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

/^#checks:/ {
   sum_checks += $2
   cnt_checks++
}

/^relVolume\*:/ {
   split($2, nums, "%")
   sum_volume += nums[1]
   cnt_volume++
}

END {
   if (_assert_exit)
        exit 1

   printf("avg #correct classifications: %.1f%%\n", (correct_cnt/total_cnt)*100)
   printf("avg #any features: %.1f%%\n", (sum_size/div_sum_size)*100)
   if (div_sum_fixed > 0) printf("avg #fixed features: %.1f%%\n", (sum_fixed/div_sum_fixed)*100)
   else printf("avg #fixed features: %.1f%%\n", (sum_size/div_sum_size)*100)
   if (cnt_volume > 0) printf("avg volume: %.2f%%\n", sum_volume/cnt_volume)
   printf("avg #checks: %.1f\n", (sum_checks/cnt_checks))
}
