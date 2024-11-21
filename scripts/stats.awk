#!/bin/awk -f

function assert(condition, string) {
    if (!condition) {
        printf("%s:%d: assertion failed: %s\n", FILENAME, FNR, string) > "/dev/stderr"
        _assert_exit = 1
        exit 1
    }
}

/expected output:/ {
   expected = $NF
}

/computed output:/ {
   computed = $NF
   assert(expected != "" && computed != "")
   total_cnt++
   if (computed == expected) correct_cnt++
}

/size:/ && $1 != "Dataset" {
   split($NF, nums, "/")
   sum_size += nums[1]
   div_sum_size += nums[2]
}

/fixed features:/ {
   split($NF, nums, "/")
   sum_fixed += nums[1]
   div_sum_fixed += nums[2]
}

/#checks:/ {
   sum_checks += $NF
   cnt_checks++
}

/relVolume\*:/ {
   split($NF, nums, "%")
   sum_volume += nums[1]
   cnt_volume++
}

END {
   if (_assert_exit)
      exit 1

   print("Total: " total_cnt)

   printf("avg #correct classifications: %.1f%%\n", (correct_cnt/total_cnt)*100)
   printf("avg #any features: %.1f%%\n", (sum_size/div_sum_size)*100)
   if (div_sum_fixed > 0) printf("avg #fixed features: %.1f%%\n", (sum_fixed/div_sum_fixed)*100)
   else printf("avg #fixed features: %.1f%%\n", (sum_size/div_sum_size)*100)
   if (cnt_volume > 0) {
      assert(total_cnt == cnt_volume, "total_cnt == cnt_volume: " total_cnt " == " cnt_volume)
      printf("avg volume: %.2f%%\n", sum_volume/cnt_volume)
   }
   if (cnt_checks > 0) {
      assert(total_cnt == cnt_checks, "total_cnt == cnt_checks: " total_cnt " == " cnt_checks)
      printf("avg #checks: %.1f\n", (sum_checks/cnt_checks))
   }
}
