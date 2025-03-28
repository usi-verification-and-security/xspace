#!/bin/awk -f

function assert(condition, string) {
    if (!condition) {
        printf("%s:%d: assertion failed: %s\n", FILENAME, FNR, string) > "/dev/stderr"
        _assert_exit = 1
        exit 1
    }
}

function max(a ,b) {
   return a >= b ? a : b
}

BEGIN {
   max_class = 0
}

/expected output:/ {
   expected = $NF
   max_class = max(max_class, expected)
   cnt_expected[expected]++
}

/computed output:/ {
   computed = $NF
   max_class = max(max_class, computed)
   cnt_computed[computed]++
   assert(expected != "" && computed != "")
   total_cnt++
   if (computed == expected) {
      correct_cnt++
      correct_cnt_expected[expected]++
      correct_cnt_computed[computed]++
   }
   else {
      incorrect_cnt++
      incorrect_cnt_expected[expected]++
      incorrect_cnt_computed[computed]++
   }
}

/#features:/ {
   split($NF, nums, "/")
   sum_size += nums[1]
   div_sum_size += nums[2]

   if (expected != "" && computed != "") {
      sum_size_expected[expected] += nums[1]
      div_sum_size_expected[expected] += nums[2]

      sum_size_computed[computed] += nums[1]
      div_sum_size_computed[computed] += nums[2]

      if (computed == expected) {
         sum_size_correct += nums[1]
         div_sum_size_correct += nums[2]

         sum_size_correct_expected[expected] += nums[1]
         div_sum_size_correct_expected[expected] += nums[2]
      }
      else {
         sum_size_incorrect += nums[1]
         div_sum_size_incorrect += nums[2]

         sum_size_incorrect_expected[expected] += nums[1]
         div_sum_size_incorrect_expected[expected] += nums[2]
      }
   }
}

/#fixed features:/ {
   split($NF, nums, "/")
   sum_fixed += nums[1]
   div_sum_fixed += nums[2]

   if (expected != "" && computed != "") {
      sum_fixed_expected[expected] += nums[1]
      div_sum_fixed_expected[expected] += nums[2]

      sum_fixed_computed[computed] += nums[1]
      div_sum_fixed_computed[computed] += nums[2]

      if (computed == expected) {
         sum_fixed_correct += nums[1]
         div_sum_fixed_correct += nums[2]

         sum_fixed_correct_expected[expected] += nums[1]
         div_sum_fixed_correct_expected[expected] += nums[2]
      }
      else {
         sum_fixed_incorrect += nums[1]
         div_sum_fixed_incorrect += nums[2]

         sum_fixed_incorrect_expected[expected] += nums[1]
         div_sum_fixed_incorrect_expected[expected] += nums[2]
      }
   }
}

/#terms:/ {
   sum_termsize += $NF
   cnt_termsize++
}

/#checks:/ {
   sum_checks += $NF
   cnt_checks++
}

/relVolume\*:/ {
   split($NF, nums, "%")
   sum_volume += nums[1]
   cnt_volume++

   if (expected != "" && computed != "") {
      sum_volume_expected[expected] += nums[1]
      cnt_volume_expected[expected]++

      sum_volume_computed[computed] += nums[1]
      cnt_volume_computed[computed]++

      if (computed == expected) {
         sum_volume_correct += nums[1]
         cnt_volume_correct++

         sum_volume_correct_expected[expected] += nums[1]
         cnt_volume_correct_expected[expected]++
      }
      else {
         sum_volume_incorrect += nums[1]
         cnt_volume_incorrect++

         sum_volume_incorrect_expected[expected] += nums[1]
         cnt_volume_incorrect_expected[expected]++
      }
   }
}

function print_total(total_cnt_) {
   print("Total: " total_cnt_)
}

function print_correct_ratio(total_cnt_, correct_cnt_) {
   printf("avg #correct classifications: %.1f%%\n", (correct_cnt_/total_cnt_)*100)
}

function print_size(sum_size_, div_sum_size_, sum_fixed_, div_sum_fixed_) {
   printf("avg #any features: %.1f%%\n", (sum_size_/div_sum_size_)*100)
   if (div_sum_fixed_ > 0) printf("avg #fixed features: %.1f%%\n", (sum_fixed_/div_sum_fixed_)*100)
   else printf("avg #fixed features: %.1f%%\n", (sum_size_/div_sum_size_)*100)
}

function print_volume(sum_volume_, cnt_volume_) {
   if (cnt_volume == 0) return
   printf("avg relVolume*: %.2f%%\n", sum_volume_/cnt_volume_)
}

function print_sep() {
   printf("\n--------------------------------------------------\n")
}

function print_header(name) {
   printf("\n/%s/\n", name)
}

END {
   if (_assert_exit)
      exit 1

   if (total_cnt > 0) {
      print_total(total_cnt)
      print_correct_ratio(total_cnt, correct_cnt)
   }
<<<<<<< Updated upstream
   printf("avg #any features: %.1f%%\n", (sum_size/div_sum_size)*100)
   if (div_sum_fixed > 0) printf("avg #fixed features: %.1f%%\n", (sum_fixed/div_sum_fixed)*100)
   else printf("avg #fixed features: %.1f%%\n", (sum_size/div_sum_size)*100)
   if (cnt_termsize > 0) {
      assert(total_cnt == 0 || total_cnt == cnt_termsize, "total_cnt == cnt_termsize: " total_cnt " == " cnt_termsize)
      printf("avg #terms: %.1f\n", (sum_termsize/cnt_termsize))
   }
   if (cnt_volume > 0) {
      assert(total_cnt == 0 || total_cnt == cnt_volume, "total_cnt == cnt_volume: " total_cnt " == " cnt_volume)
      printf("avg relVolume*: %.2f%%\n", sum_volume/cnt_volume)
=======

   print_size(sum_size, div_sum_size, sum_fixed, div_sum_fixed)

   if (cnt_volume > 0) {
      assert(total_cnt == 0 || total_cnt == cnt_volume, "total_cnt == cnt_volume: " total_cnt " == " cnt_volume)
      print_volume(sum_volume, cnt_volume)
>>>>>>> Stashed changes
   }

   if (cnt_checks > 0) {
      assert(total_cnt == 0 || total_cnt == cnt_checks, "total_cnt == cnt_checks: " total_cnt " == " cnt_checks)
      printf("avg #checks: %.1f\n", (sum_checks/cnt_checks))
   }

   if (total_cnt > 0) {
      if (cnt_volume > 0) {
         assert(total_cnt == cnt_volume_correct + cnt_volume_incorrect, "total_cnt == cnt_volume_correct + cnt_volume_incorrect: " total_cnt " == " cnt_volume_correct " + " cnt_volume_incorrect)
      }

      print_sep()

      print_header("correct")
      print_total(correct_cnt)
      print_size(sum_size_correct, div_sum_size_correct, sum_fixed_correct, div_sum_fixed_correct)
      print_volume(sum_volume_correct, cnt_volume_correct)

      print_header("incorrect")
      print_total(incorrect_cnt)
      print_size(sum_size_incorrect, div_sum_size_incorrect, sum_fixed_incorrect, div_sum_fixed_incorrect)
      print_volume(sum_volume_incorrect, cnt_volume_incorrect)

      print_sep()

      for (c = 0; c <= max_class; c++) {
         print_header("expected class " c)
         print_total(cnt_expected[c])
         print_correct_ratio(cnt_expected[c], correct_cnt_expected[c])
         print_size(sum_size_expected[c], div_sum_size_expected[c], sum_fixed_expected[c], div_sum_fixed_expected[c])
         print_volume(sum_volume_expected[c], cnt_volume_expected[c])
      }

      for (c = 0; c <= max_class; c++) {
         print_header("computed class " c)
         print_total(cnt_computed[c])
         print_correct_ratio(cnt_computed[c], correct_cnt_computed[c])
         print_size(sum_size_computed[c], div_sum_size_computed[c], sum_fixed_computed[c], div_sum_fixed_computed[c])
         print_volume(sum_volume_computed[c], cnt_volume_computed[c])
      }

      print_sep()

      for (c = 0; c <= max_class; c++) {
         print_header("correct expected class " c)
         print_total(correct_cnt_expected[c])
         print_size(sum_size_correct_expected[c], div_sum_size_correct_expected[c], sum_fixed_correct_expected[c], div_sum_fixed_correct_expected[c])
         print_volume(sum_volume_correct_expected[c], cnt_volume_correct_expected[c])
      }

      for (c = 0; c <= max_class; c++) {
         print_header("incorrect expected class " c)
         print_total(incorrect_cnt_expected[c])
         print_size(sum_size_incorrect_expected[c], div_sum_size_incorrect_expected[c], sum_fixed_incorrect_expected[c], div_sum_fixed_incorrect_expected[c])
         print_volume(sum_volume_incorrect_expected[c], cnt_volume_incorrect_expected[c])
      }
   }
}
