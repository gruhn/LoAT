; ./hcai/./svcomp/O3/gcd02_true-unreach-call_true-no-overflow_true-termination_000.smt2
(set-logic HORN)

(declare-fun |main@.lr.ph..lr.ph.split_crit_edge| ( Int Int ) Bool)
(declare-fun |main@verifier.error.split| ( ) Bool)
(declare-fun |main@_bb1| ( Int Int ) Bool)
(declare-fun |main@_bb| ( Int Int Bool Int ) Bool)
(declare-fun |main@entry| ( ) Bool)
(declare-fun |main@.lr.ph.i4| ( Int Int ) Bool)
(declare-fun |main@.lr.ph.i| ( Int Int Bool Int ) Bool)

(assert
  (forall ( (CHC_COMP_UNUSED Bool) ) 
    (=>
      (and
        true
      )
      main@entry
    )
  )
)
(assert
  (forall ( (A Bool) (B Bool) (C Bool) (D Bool) (E Bool) (F Bool) (G Bool) (H Bool) (I Bool) (J Int) (K Int) (L Int) (M Bool) (N Bool) (O Int) (P Int) (Q Int) ) 
    (=>
      (and
        main@entry
        (and (not (= (<= 1 K) D))
     (not (= (<= J 0) B))
     (not (= (<= K 0) A))
     (= C (and B A))
     (= I (or E D))
     (or (not H) (not G) (not F))
     (or (not N) (not I) (not H))
     (or (not N) (not M) (= L J))
     (or (not N) (not M) (= O K))
     (or (not N) (not M) (= P O))
     (or (not N) (not M) (= Q L))
     (or (not H) (and H F))
     (or (not M) (and N M))
     (or (not N) (and N H))
     (= C true)
     (not D)
     (= M true)
     (not (= (<= 1 J) E)))
      )
      (main@.lr.ph..lr.ph.split_crit_edge P Q)
    )
  )
)
(assert
  (forall ( (A Bool) (B Bool) (C Bool) (D Bool) (E Bool) (F Bool) (G Bool) (H Bool) (I Bool) (J Bool) (K Int) (L Bool) (M Int) (N Bool) (O Bool) (P Int) (Q Int) (R Int) (S Bool) (T Int) ) 
    (=>
      (and
        main@entry
        (and (not (= (<= 1 Q) J))
     (not (= (<= K 0) B))
     (not (= (<= Q 0) A))
     (= C (and B A))
     (= G (or J D))
     (or (not O) (not N) (= P Q))
     (or (not O) (not N) (= M K))
     (or (not O) (not N) (= R P))
     (or (not O) (not N) (= T M))
     (or (not O) (not N) (= S L))
     (or (not O) (not N) (= L J))
     (or F (not H) (not E))
     (or (not O) (not I) (not H))
     (or (not N) (and O N))
     (or (not O) (and H O))
     (or (not H) (= I (= Q K)))
     (or (not H) (and H E))
     (or (not H) (not G))
     (= C true)
     (= N true)
     (not J)
     (not (= (<= 1 K) D)))
      )
      (main@.lr.ph.i Q R S T)
    )
  )
)
(assert
  (forall ( (A Bool) (B Bool) (C Bool) (D Bool) (E Bool) (F Bool) (G Bool) (H Bool) (I Int) (J Bool) (K Bool) (L Int) (M Bool) (N Bool) (O Int) (P Bool) (Q Bool) (R Int) (S Int) (T Int) ) 
    (=>
      (and
        main@entry
        (and (not (= (<= 1 O) D))
     (not (= (<= I 0) B))
     (not (= (<= O 0) A))
     (= C (and B A))
     (= H (or E D))
     (or (not M) (= L O) (not K))
     (or (not M) (= T L) (not K))
     (or G (not K) (not F))
     (or J (not M) (not K))
     (or (not Q) (not N) (not M))
     (or (not Q) (not P) (= R O))
     (or (not Q) (not P) (= S R))
     (or (not M) (= N (= O 0)))
     (or (not M) (and K M))
     (or (not K) (= J (= O I)))
     (or (not K) (and K F))
     (or (not K) (not H))
     (or (not P) (and Q P))
     (or (not Q) (and Q M))
     (= C true)
     (not D)
     (= P true)
     (not (= (<= 1 I) E)))
      )
      (main@.lr.ph.i4 S T)
    )
  )
)
(assert
  (forall ( (A Bool) (B Bool) (C Bool) (D Bool) (E Bool) (F Bool) (G Bool) (H Int) (I Bool) (J Bool) (K Int) (L Int) (M Int) (N Bool) (O Bool) (P Bool) (Q Bool) (R Bool) (S Bool) (T Bool) (U Bool) (V Bool) (W Bool) ) 
    (=>
      (and
        main@entry
        (and (not (= (<= 1 M) D))
     (not (= (<= H 0) B))
     (not (= (<= M 0) A))
     (= C (and B A))
     (= U (or E D))
     (or (and T S) (and V R) (not V))
     (or G (not J) (not F))
     (or (not R) (not O) (= Q P))
     (or (not R) (not O) (not P))
     (or (not O) (not J) (= K L))
     (or (not O) (not J) (= L M))
     (or (not O) (not J) I)
     (or (not R) (not O) N)
     (or (not S) (not G) (not F))
     (or (not T) (not S) U)
     (or (not W) (and V W))
     (or (not R) (and O R))
     (or (not R) Q)
     (or (not J) (= I (= M H)))
     (or (not J) (and J F))
     (or (not J) (not U))
     (or (not O) (= N (= M 0)))
     (or (not O) (and O J))
     (or (not S) (and S F))
     (or (not T) S)
     (= C true)
     (= W true)
     (not D)
     (not (= (<= 1 H) E)))
      )
      main@verifier.error.split
    )
  )
)
(assert
  (forall ( (A Int) (B Bool) (C Bool) (D Int) (E Int) (F Int) ) 
    (=>
      (and
        (main@.lr.ph..lr.ph.split_crit_edge E A)
        (and (or (not C) (not B) (= F D))
     (or (not B) (and C B))
     (= B true)
     (or (not C) (not B) (= D A)))
      )
      (main@_bb1 E F)
    )
  )
)
(assert
  (forall ( (A Bool) (B Bool) (C Int) (D Bool) (E Int) (F Int) (G Bool) (H Bool) (I Bool) (J Bool) (K Int) (L Int) (M Int) (N Bool) (O Bool) (P Int) (Q Int) (R Int) ) 
    (=>
      (and
        (main@_bb1 F C)
        (let ((a!1 (or (not O) (= L (+ F (* (- 1) K)))))
      (a!2 (or (not O) (not (= (<= 1 K) I))))
      (a!3 (or (not O) (not (= (<= 1 L) H)))))
  (and (= A (= C F))
       (or (not O) (not D) (= E C))
       (or (not O) (not D) (= K E))
       (or (not O) (not D) B)
       (or (not O) (not N) (= M K))
       (or (not O) (not N) (= P L))
       (or (not O) (not N) (= Q P))
       (or (not O) (not N) (= R M))
       (or (not O) (not N) (not J))
       (or (not N) (and O N))
       a!1
       a!2
       a!3
       (or (not O) (= J (or I H)))
       (or (not O) (and O D))
       (or (not O) (not G))
       (not A)
       (= N true)
       (not (= (<= F C) B))))
      )
      (main@.lr.ph..lr.ph.split_crit_edge Q R)
    )
  )
)
(assert
  (forall ( (A Bool) (B Bool) (C Bool) (D Int) (E Bool) (F Bool) (G Int) (H Bool) (I Bool) (J Int) (K Int) (L Int) ) 
    (=>
      (and
        (main@_bb1 K D)
        (let ((a!1 (or (not I) (= G (+ D (* (- 1) K)))))
      (a!2 (or (not I) (not (= (<= 1 G) F)))))
  (and (= A (= D K))
       (or (not I) (not C) (not B))
       (or (not I) (not H) (= J G))
       (or (not I) (not H) (= L J))
       (or (not I) (not H) (not F))
       (or (not H) (and I H))
       a!1
       a!2
       (or (not I) (and I B))
       (or (not I) (not E))
       (not A)
       (= H true)
       (not (= (<= K D) C))))
      )
      (main@_bb1 K L)
    )
  )
)
(assert
  (forall ( (A Bool) (B Bool) (C Int) (D Bool) (E Bool) (F Bool) (G Int) (H Bool) (I Int) (J Int) (K Bool) (L Int) (M Int) (N Bool) (O Bool) (P Bool) (Q Bool) (R Bool) (S Bool) (T Bool) (U Bool) ) 
    (=>
      (and
        (main@_bb1 J G)
        (let ((a!1 (or (not D) (= C (+ G (* (- 1) J)))))
      (a!2 (or (not D) (not (= (<= 1 C) E))))
      (a!3 (or (not P) (= L (+ J (* (- 1) M)))))
      (a!4 (or (not P) (not (= (<= 1 M) O))))
      (a!5 (or (not P) (not (= (<= 1 L) N)))))
  (and (= A (= G J))
       (or (and T S) (not T) (and R T))
       (or E (not S) (not D))
       (or (not P) (not H) (= I G))
       (or (not P) (not H) (= M I))
       (or (not H) (not F) (not D))
       (or (not P) F (not H))
       (or (not P) (not R) Q)
       (or (not S) (and D S))
       (or (not U) (and T U))
       a!1
       a!2
       (or (not D) (and H D))
       (or (not D) (not B))
       a!3
       a!4
       a!5
       (or (not P) (= Q (or O N)))
       (or (not P) (and H P))
       (or (not P) (not K))
       (or (not R) (and R P))
       (not A)
       (= U true)
       (not (= (<= J G) F))))
      )
      main@verifier.error.split
    )
  )
)
(assert
  (forall ( (A Int) (B Bool) (C Bool) (D Int) (E Int) (F Int) (G Bool) (H Int) ) 
    (=>
      (and
        (main@.lr.ph.i E F G A)
        (and (or (not C) (not B) (= H D))
     (or (not B) (and C B))
     (= B true)
     (or (not C) (not B) (= D A)))
      )
      (main@_bb E F G H)
    )
  )
)
(assert
  (forall ( (A Bool) (B Int) (C Bool) (D Int) (E Bool) (F Int) (G Int) (H Bool) (I Bool) (J Bool) (K Bool) (L Int) (M Int) (N Bool) (O Int) (P Bool) (Q Bool) (R Int) (S Int) (T Int) (U Bool) (V Int) ) 
    (=>
      (and
        (main@_bb S G A D)
        (let ((a!1 (or (not Q) (= M (+ G (* (- 1) L)))))
      (a!2 (or (not Q) (not (= (<= 1 L) H))))
      (a!3 (or (not Q) (not (= (<= 1 M) K)))))
  (and (not (= (<= G D) C))
       (or (not Q) (not E) (= F D))
       (or (not Q) (not E) (= L F))
       (or (not Q) (not E) C)
       (or (not Q) (not P) (= R M))
       (or (not Q) (not P) (= O L))
       (or (not Q) (not P) (= T R))
       (or (not Q) (not P) (= V O))
       (or (not Q) (not P) (= U N))
       (or (not Q) (not P) (= N K))
       (or (not Q) (not P) (not J))
       (or (not P) (and Q P))
       a!1
       a!2
       a!3
       (or (not Q) (= I (or K H)))
       (or (not Q) (= J (= M L)))
       (or (not Q) (and Q E))
       (or (not Q) (not I))
       (= P true)
       (= B (+ D (* (- 1) G)))))
      )
      (main@.lr.ph.i S T U V)
    )
  )
)
(assert
  (forall ( (A Int) (B Bool) (C Bool) (D Bool) (E Bool) (F Bool) (G Int) (H Bool) (I Bool) (J Int) (K Int) (L Int) (M Bool) (N Int) ) 
    (=>
      (and
        (main@_bb K L M A)
        (let ((a!1 (or (not I) (not (= (<= 1 G) D)))))
  (and (not (= (<= L A) C))
       (or (not I) (not H) (= J G))
       (or (not I) (not H) (= N J))
       (or (not I) (not C) (not B))
       (or (not I) (not F) (not H))
       (or (not H) (and I H))
       a!1
       (or (not I) (= E (or D M)))
       (or (not I) (= F (= L G)))
       (or (not I) (and B I))
       (or (not I) (not E))
       (= H true)
       (= G (+ A (* (- 1) L)))))
      )
      (main@_bb K L M N)
    )
  )
)
(assert
  (forall ( (A Bool) (B Bool) (C Bool) (D Int) (E Bool) (F Bool) (G Int) (H Bool) (I Int) (J Bool) (K Int) (L Int) (M Bool) (N Bool) (O Bool) (P Int) (Q Bool) (R Int) (S Bool) (T Int) (U Int) (V Int) (W Bool) (X Int) (Y Bool) (Z Int) (A1 Bool) (B1 Bool) (C1 Int) (D1 Bool) (E1 Bool) (F1 Int) (G1 Int) (H1 Int) ) 
    (=>
      (and
        (main@_bb C1 L A I)
        (let ((a!1 (or (not F) (not (= (<= 1 D) B))))
      (a!2 (or (not S) (= P (+ L (* (- 1) R)))))
      (a!3 (or (not S) (not (= (<= 1 R) M))))
      (a!4 (or (not S) (not (= (<= 1 P) N)))))
  (and (not (= (<= L I) H))
       (or (not J) (not H) (not F))
       (or (not A1) (and Y A1) (and W A1))
       (or (not S) (not J) (= K I))
       (or (not S) (not J) (= R K))
       (or (not S) (not J) H)
       (or (not A1) (not W) (= X U))
       (or (not A1) (not W) (= H1 X))
       (or (not W) (not S) (= T R))
       (or (not W) (not S) (= U T))
       (or (not W) (not S) Q)
       (or (not Y) (not F) (= G L))
       (or (not Y) (not F) (= V G))
       (or (not Y) (not F) E)
       (or (not A1) (not Y) (= Z V))
       (or (not A1) (= H1 Z) (not Y))
       (or (not E1) (not B1) (not A1))
       (or (not E1) (not D1) (= F1 C1))
       (or (not E1) (not D1) (= G1 F1))
       a!1
       (or (not F) (= C (or B A)))
       (or (not F) (= E (= L D)))
       (or (not F) (and J F))
       (or (not F) (not C))
       (or (not A1) (= B1 (= C1 0)))
       a!2
       a!3
       a!4
       (or (not S) (= O (or N M)))
       (or (not S) (= Q (= P R)))
       (or (not S) (and S J))
       (or (not S) (not O))
       (or (not W) (and W S))
       (or (not Y) (and Y F))
       (or (not D1) (and E1 D1))
       (or (not E1) (and E1 A1))
       (= D1 true)
       (= D (+ I (* (- 1) L)))))
      )
      (main@.lr.ph.i4 G1 H1)
    )
  )
)
(assert
  (forall ( (A Bool) (B Bool) (C Bool) (D Int) (E Bool) (F Bool) (G Int) (H Bool) (I Int) (J Bool) (K Int) (L Int) (M Bool) (N Bool) (O Bool) (P Int) (Q Bool) (R Int) (S Bool) (T Int) (U Int) (V Int) (W Bool) (X Int) (Y Bool) (Z Int) (A1 Int) (B1 Int) (C1 Bool) (D1 Bool) (E1 Bool) (F1 Bool) (G1 Bool) (H1 Bool) (I1 Bool) ) 
    (=>
      (and
        (main@_bb B1 L A I)
        (let ((a!1 (or (not F) (not (= (<= 1 D) B))))
      (a!2 (or (not S) (= P (+ L (* (- 1) R)))))
      (a!3 (or (not S) (not (= (<= 1 P) N))))
      (a!4 (or (not S) (not (= (<= 1 R) M)))))
  (and (not (= (<= L I) H))
       (or (not H) (not F) (not J))
       (or (not D1) (and Y D1) (and W D1))
       (or (not D1) (not G1) (= F1 E1))
       (or (not D1) C1 (not G1))
       (or (not S) (not J) (= K I))
       (or (not S) (not J) (= R K))
       (or (not S) (not J) H)
       (or (not D1) (not W) (= X U))
       (or (not D1) (not W) (= Z X))
       (or (not W) (not S) (= U T))
       (or (not W) (not S) (= T R))
       (or (not W) (not S) Q)
       (or (not Y) (not F) (= G L))
       (or (not Y) (not F) (= V G))
       (or (not Y) (not F) E)
       (or (not D1) (not Y) (= A1 V))
       (or (not D1) (not Y) (= Z A1))
       (or (not D1) (not E1) (not G1))
       a!1
       (or (not F) (= E (= L D)))
       (or (not F) (= C (or B A)))
       (or (not F) (and F J))
       (or (not F) (not C))
       (or (not G1) (and D1 G1))
       (or (not I1) (and H1 I1))
       (or (not D1) (= C1 (= B1 0)))
       (or (not H1) (and H1 G1))
       a!2
       a!3
       a!4
       (or (not S) (= O (or N M)))
       (or (not S) (= Q (= P R)))
       (or (not S) (and S J))
       (or (not S) (not O))
       (or (not W) (and W S))
       (or (not Y) (and Y F))
       (or F1 (not G1))
       (= I1 true)
       (= D (+ I (* (- 1) L)))))
      )
      main@verifier.error.split
    )
  )
)
(assert
  (forall ( (A Bool) (B Bool) (C Int) (D Bool) (E Int) (F Bool) (G Bool) (H Int) (I Int) (J Int) ) 
    (=>
      (and
        (main@.lr.ph.i4 C J)
        (let ((a!1 (or (not G) (= E (+ C (* (- 1) J))))))
  (and (or (not G) (not B) (not A))
       (or (not G) (not F) (= H E))
       (or (not G) (not F) (= I H))
       (or (not G) (not F) (not D))
       (or (not F) (and G F))
       a!1
       (or (not G) (= D (= C J)))
       (or (not G) (and G A))
       (= F true)
       (not (= (<= J C) B))))
      )
      (main@.lr.ph.i4 I J)
    )
  )
)
(assert
  (forall ( (A Int) (B Int) (C Int) (D Bool) (E Bool) (F Bool) (G Bool) (H Bool) (I Bool) (J Bool) (K Bool) (L Bool) (M Bool) (N Bool) (O Bool) (P Bool) (Q Bool) (R Bool) ) 
    (=>
      (and
        (main@.lr.ph.i4 B C)
        (let ((a!1 (or (not I) (= A (+ B (* (- 1) C))))))
  (and (or (not M) (and J I) (and G F))
       (or (not M) (not P) (= N L))
       (or (not M) (not P) (= O N))
       (or (not G) (not F) (= L H))
       (or (not G) (not F) D)
       (or H (not G) (not F))
       (or (not I) (not F) (not D))
       (or (not J) (not I) (= L K))
       (or (not J) (not K) (not I))
       (or (not J) (not I) E)
       (or (not P) (and M P))
       (or (not R) (and Q R))
       (or (not Q) (and Q P))
       (or (not G) F)
       a!1
       (or (not I) (= E (= B C)))
       (or (not I) (and I F))
       (or (not J) I)
       (or O (not P))
       (= R true)
       (not (= (<= C B) D))))
      )
      main@verifier.error.split
    )
  )
)
(assert
  (forall ( (CHC_COMP_UNUSED Bool) ) 
    (=>
      (and
        main@verifier.error.split
        true
      )
      false
    )
  )
)

(check-sat)
(exit)
