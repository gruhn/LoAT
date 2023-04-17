; ./hopv/./lia/termination/binomial03_000.smt2
(set-logic HORN)

(declare-fun |main_1033$unknown:28| ( Int Int Int Int Int Int ) Bool)
(declare-fun |fail$unknown:21| ( Int ) Bool)
(declare-fun |bin_1030$unknown:8| ( Int Int Int Int Int Int Int Int ) Bool)

(assert
  (forall ( (A Int) (B Int) (C Int) (D Int) (E Int) (F Int) (G Int) (H Int) (I Int) (J Int) (K Int) (L Int) (M Int) (N Int) (O Int) (P Int) (Q Int) (R Int) (S Int) (T Int) (U Int) (V Int) (W Int) (X Int) (Y Int) (Z Int) (A1 Int) (B1 Int) (C1 Int) (D1 Int) (E1 Int) (F1 Int) (G1 Int) (H1 Int) (I1 Int) (J1 Int) (K1 Int) (L1 Int) (M1 Int) (N1 Int) (O1 Int) (P1 Int) (Q1 Int) (R1 Int) (S1 Int) (T1 Int) (U1 Int) (V1 Int) (W1 Int) (X1 Int) (Y1 Int) (Z1 Int) (A2 Int) (B2 Int) (C2 Int) (D2 Int) (E2 Int) (F2 Int) ) 
    (=>
      (and
        (|bin_1030$unknown:8| H G F E D C B A)
        (let ((a!1 (= (= 0 G1) (and (not (= 0 Z)) (not (= 0 F1)))))
      (a!2 (= (= 0 N) (or (not (= 0 G1)) (not (= 0 M)))))
      (a!3 (= (= 0 M) (and (not (= 0 L)) (not (= 0 R1)))))
      (a!4 (= (= 0 L) (and (not (= 0 K)) (not (= 0 C2))))))
  (and (not (= (= 0 R1) (>= L1 Q1)))
       (not a!1)
       (not (= (= 0 F1) (>= E1 0)))
       (= (= 0 Z) (<= T Y))
       (not a!2)
       (not a!3)
       (not a!4)
       (not (= (= 0 K) (>= J 0)))
       (= 0 N)
       (not (= 0 E))
       (= F2 (+ D2 E2))
       (= E2 D)
       (= D2 0)
       (= B2 (+ Z1 A2))
       (= A2 (* (- 1) H))
       (= Z1 (+ X1 Y1))
       (= Y1 D)
       (= X1 0)
       (= W1 (+ U1 V1))
       (= V1 (* (- 1) G))
       (= U1 (+ S1 T1))
       (= T1 F)
       (= S1 0)
       (= Q1 (+ O1 P1))
       (= P1 H)
       (= O1 (+ M1 N1))
       (= N1 0)
       (= M1 0)
       (= L1 (+ J1 K1))
       (= K1 G)
       (= J1 (+ H1 I1))
       (= I1 0)
       (= H1 0)
       (= E1 (+ C1 D1))
       (= D1 H)
       (= C1 (+ A1 B1))
       (= B1 0)
       (= A1 0)
       (= Y (+ W X))
       (= X H)
       (= W (+ U V))
       (= V 0)
       (= U 0)
       (= T (+ R S))
       (= S G)
       (= R (+ P Q))
       (= Q 0)
       (= P 0)
       (= O 1)
       (= J (+ F2 I))
       (= I (* (- 1) H))
       (= (= 0 C2) (<= W1 B2))))
      )
      (|fail$unknown:21| O)
    )
  )
)
(assert
  (forall ( (A Int) (B Int) (C Int) (D Int) (E Int) (F Int) (G Int) (H Int) (I Int) (v_9 Int) (v_10 Int) (v_11 Int) ) 
    (=>
      (and
        (|main_1033$unknown:28| F E D C B A)
        (let ((a!1 (= (= 0 I) (and (not (= 0 G)) (not (= 0 H))))))
  (and (not (= (= 0 H) (>= F 0)))
       (not (= (= 0 G) (>= E 0)))
       (not (= 0 I))
       (not a!1)
       (= v_9 C)
       (= v_10 B)
       (= v_11 A)))
      )
      (|bin_1030$unknown:8| F C B A E v_9 v_10 v_11)
    )
  )
)
(assert
  (forall ( (A Int) (B Int) (C Int) (D Int) (E Int) (F Int) ) 
    (=>
      (and
        (and (= E 0) (= D 0) (= C 0) (= F 1))
      )
      (|main_1033$unknown:28| A B F E D C)
    )
  )
)
(assert
  (forall ( (A Int) ) 
    (=>
      (and
        (|fail$unknown:21| A)
        true
      )
      false
    )
  )
)

(check-sat)
(exit)
