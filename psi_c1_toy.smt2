(set-logic QF_LRA)
(declare-fun x1 () Real)
(declare-fun x2 () Real)
(declare-fun x3 () Real)

(assert (and (<= 0 x1) (<= (- 4) (* (- 1) x1)) (<= 0 x2) (<= (- 4) (* (- 1) x2)) (<= 0 x3) (<= (- 4) (* (- 1) x3))))
(assert (<= (/ 1 64) (+ (* (- 1) (ite (<= 0 (+ x1 (* x3 (/ 1 2)))) (+ (* x1 2) x3) 0)) (* (ite (<= 0 (+ (* (- 1) x1) x2 (* (- 1) x3))) (+ (* (- 1) x1) x2 (* (- 1) x3)) 0) 4))))
