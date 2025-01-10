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
    if (incremented == 0) {
        idx++
    }

    cnt = idx-1
    idx = 1
    fidx++
    incremented = 0
}

/#fixed features:/ {
    split($NF, nums, "/")
    assert(fix_sizes[fidx, idx] == "", "fix_sizes["fidx", "idx"]  ==  '': " fix_sizes[fidx, idx])
    fix_sizes[fidx, idx] = nums[1]

    if (idx == 1) {
        if (first_is_size[fidx] == "") {
            first_is_fixed[fidx] = 1
        }
    }

    if (first_is_fixed[fidx]) {
        sizes[fidx, idx] = nums[2]
    } else {
        assert(sizes[fidx, idx] == nums[2], "sizes["fidx", "idx"]  ==  nums[2]: " sizes[fidx, idx] " == " nums[2])
    }
}

/relVolume\*:/ {
    split($NF, nums, "%")
    assert(volumes[fidx, idx] == "", "volumes["fidx", "idx"]  ==  '': " volumes[fidx, idx])
    volumes[fidx, idx] = nums[1]

    if (first_is_fixed[fidx]) {
    } else {
        idx++
        incremented = 1
    }
}

/#features:/ {
    if (first_is_fixed[fidx]) {
    ## this discards the very first encounter
    } else if (first_is_size[fidx]) {
        if (!incremented) {
            idx++
        }
        incremented = 0
    }

    split($NF, nums, "/")
    assert(exp_sizes[fidx, idx] == "", "exp_sizes["fidx", "idx"]  ==  '': " exp_sizes[fidx, idx])
    exp_sizes[fidx, idx] = nums[1]

    if (idx == 1) {
        if (first_is_fixed[fidx] == "") {
            first_is_size[fidx] = 1
        }
    }

    if (first_is_fixed[fidx]) {
        if (sizes[fidx, idx] == "") {
            sizes[fidx, idx] = nums[2]
        } else {
            assert(sizes[fidx, idx] == nums[2], "sizes["fidx", "idx"]  ==  nums[2]: " sizes[fidx, idx] " == " nums[2])
        }
        idx++
        incremented = 1
    } else {
        sizes[fidx, idx] = nums[2]
    }
}

END {
    if (_assert_exit)
        exit 1

    if (incremented == 0) {
        idx++
    }

    assert(idx-1 == cnt, "idx-1 == cnt: " idx-1 " == " cnt)
    print("Total: " cnt)

    for (f = 1; f <= fidx; f++) {
        for (i = 1; i <= cnt; i++) {
            assert(sizes[f, i] > 0, "sizes["f", "i"]  >  0: " sizes[f, i])
            assert(exp_sizes[f, i] != "", "exp_sizes[f, i]  !=  '': " exp_sizes[f, i])
            if (fix_sizes[f, i] == "") fix_sizes[f, i] = exp_sizes[f, i]
            if (volumes[f, i] == "") volumes[f, i] = 100
        }
    }

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

        assert(lts[f] + gts[f] + eqs[f] == cnt, "lts["f"] + gts["f"] + eqs["f"] == "cnt)
        assert(lts2[f] + lts1[f] + gts1[f] + gts2[f] + eqs[f] == cnt, "lts2["f"] + lts1["f"] + gts1["f"] + gts2["f"] + eqs["f"] == "cnt)
    }
}
