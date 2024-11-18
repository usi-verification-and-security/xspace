#!/bin/awk -f

function assert(condition, string) {
    if (! condition) {
        printf("%s:%d: assertion failed: %s\n", FILENAME, FNR, string) > "/dev/stderr"
        _assert_exit = 1
        exit 1
    }
}

function compare(f1, f2, i) {
    assert(sizes[f1, i] == sizes[f2, i], "sizes[f1, i] == sizes[f2, i]: " sizes[f1, i] " == " sizes[f2, i])

    if (fix_sizes[f1, i] > fix_sizes[f2, i]) return -2
    if (fix_sizes[f1, i] < fix_sizes[f2, i]) return 2

    if (volumes[f1, i] < volumes[f2, i]) return -1
    if (volumes[f1, i] > volumes[f2, i]) return 1

    return 0
}

FNR == 1 {
    cnt = idx
    idx = 1
    fidx++
}

/fixed features:/ {
    split($NF, nums, "/")
    fix_sizes[fidx, idx] = nums[1]

    sizes[fidx, idx] = nums[2]
    if (idx == 1) extended[fidx] = 1
}

/relVolume\*:/ {
    split($NF, nums, "%")
    volumes[fidx, idx] = nums[1]
}

/size:/ {
    split($NF, nums, "/")
    exp_sizes[fidx, idx] = nums[1]

    if (extended[fidx] == 1) {
        assert(sizes[fidx, idx] == nums[2], "sizes[fidx, idx]  ==  nums[2]: " sizes[fidx, idx] " == " nums[2])
    } else {
        fix_sizes[fidx, idx] = nums[1]
        sizes[fidx, idx] = nums[2]
        volumes[fidx, idx] = 100
    }

    idx++
}

END {
    if (_assert_exit)
        exit 1

    assert(idx == cnt, "idx == cnt: " idx " == " cnt)
    for (f = 1; f < fidx; f++) {
        for (i = 1; i <= cnt; i++) {
            # printf("exp_size: %d vs. %d\n", exp_sizes[f, i], exp_sizes[f+1, i])
            # printf("fix_size: %d vs. %d\n", fix_sizes[f, i], fix_sizes[f+1, i])
            # printf("volume: %.2f vs. %.2f\n", volumes[f, i], volumes[f+1, i])

            res = compare(f, f+1, i)

            if (res == -2) {
                lts2[f]++
                lts[f]++
            } else if (res == -1) {
                lts1[f]++
                lts[f]++
            } else if (res == 1) {
                gts1[f]++
                gts[f]++
            } else if (res == 2) {
                gts2[f]++
                gts[f]++
            } else {
                eqs[f]++
            }
        }

        printf("<: %d =: %d >: %d\n", lts[f], eqs[f], gts[f])
        printf("<<: %d <: %d =: %d >: %d >>:%d\n", lts2[f], lts1[f], eqs[f], gts1[f], gts2[f])
    }
}
