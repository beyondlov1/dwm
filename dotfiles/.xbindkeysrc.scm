;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Start of xbindkeys guile configuration ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This configuration is guile based.
;;   http://www.gnu.org/software/guile/guile.html
;; any functions that work in guile will work here.
;; see EXTRA FUNCTIONS:

;; Version: 1.8.6

;; If you edit this file, do not forget to uncomment any lines
;; that you change.
;; The semicolon(;) symbol may be used anywhere for comments.

;; To specify a key, you can use 'xbindkeys --key' or
;; 'xbindkeys --multikey' and put one of the two lines in this file.

;; A list of keys is in /usr/include/X11/keysym.h and in
;; /usr/include/X11/keysymdef.h
;; The XK_ is not needed.

;; List of modifier:
;;   Release, Control, Shift, Mod1 (Alt), Mod2 (NumLock),
;;   Mod3 (CapsLock), Mod4, Mod5 (Scroll).


;; The release modifier is not a standard X modifier, but you can
;; use it if you want to catch release instead of press events

;; By defaults, xbindkeys does not pay attention to modifiers
;; NumLock, CapsLock and ScrollLock.
;; Uncomment the lines below if you want to use them.
;; To dissable them, call the functions with #f


;;;;EXTRA FUNCTIONS: Enable numlock, scrolllock or capslock usage
;;(set-numlock! #t)
;;(set-scrolllock! #t)
;;(set-capslock! #t)

;;;;; Scheme API reference
;;;;
;; Optional modifier state:
;; (set-numlock! #f or #t)
;; (set-scrolllock! #f or #t)
;; (set-capslock! #f or #t)
;; 
;; Shell command key:
;; (xbindkey key "foo-bar-command [args]")
;; (xbindkey '(modifier* key) "foo-bar-command [args]")
;; 
;; Scheme function key:
;; (xbindkey-function key function-name-or-lambda-function)
;; (xbindkey-function '(modifier* key) function-name-or-lambda-function)
;; 
;; Other functions:
;; (remove-xbindkey key)
;; (run-command "foo-bar-command [args]")
;; (grab-all-keys)
;; (ungrab-all-keys)
;; (remove-all-keys)
;; (debug)


;; Examples of commands:

(xbindkey '(control shift q) "xbindkeys_show")

;; set directly keycode (here control + f with my keyboard)
;; (xbindkey '("m:0x4" "c:41") "xterm")

;; specify a mouse button
;; (xbindkey '(control "b:2") "xterm")

;;(xbindkey '(shift mod2 alt s) "xterm -geom 50x20+20+20")

;; set directly keycode (control+alt+mod2 + f with my keyboard)
; (xbindkey '(alt "m:4" mod2 "c:0x29") "xterm")

;; Control+Shift+a  release event starts rxvt
;;(xbindkey '(release control shift a) "rxvt")

;; Control + mouse button 2 release event starts rxvt
;;(xbindkey '(releace control "b:2") "rxvt")


;; Extra features
; (xbindkey-function '(m:4 a)
;      	   (lambda ()
;     	     (display "Hello from Scheme!")
;      	     (newline)))

(xbindkey-function '(mod4 v)
     	   (lambda ()
    	     (run-command "copyq show")))


;; (xbindkey-function '(shift p)
;;      	   (lambda ()
;;      	     (run-command "xterm")))


;;Double click test
 ;(xbindkey-function '(b:3)
		 ;(let ((count 0))
		   ;(lambda ()
		     ;(set! count (+ count 1))
		     ;(if (> count 1)
			 ;(begin
			  ;(set! count 0)
			  ;(run-command "sleep 0.2 && xte 'keydown Control_L' 'key w' 'keyup Control_L'"))))))

;(xbindkey-function '(shift b:3)
                ;(lambda ()
	     ;(run-command "xte 'keyup Shift_L'; sleep 0.02 && xte 'keydown Control_L' 'key w' 'keyup Control_L'")))

(xbindkey-function '(release F2)
     	   (lambda ()
	     (run-command "sleep 0.02 && xte 'key F5'")))


;(xbindkey-function '(release F3)
                ;(lambda ()
	     ;(run-command "xte 'keydown Control_L' 'key t' 'keyup Control_L'")))




; tile5 放大缩小
;(xbindkey-function '(alt b:4)
                ;(lambda ()
	     ;(run-command "sleep 0.02 && xte 'keydown Alt_L' 'key equal'")))

;(xbindkey-function '(alt b:5)
                ;(lambda ()
	     ;(run-command "sleep 0.02 && xte 'keydown Alt_L' 'key minus'")))



(xbindkey-function '(alt b:4)
		(lambda ()
	     (run-command "smartdispatch.sh smartleft")))

(xbindkey-function '(alt b:5)
		(lambda ()
	     (run-command "smartdispatch.sh smartright")))



;(xbindkey-function '(b:2)
                ;(lambda ()
	     ;(run-command "xte 'keyup Shift_L'; sleep 0.02 && xte 'keydown Control_L' 'key w' 'keyup Control_L'")))

(xbindkey-function '(b:2)
     	   (lambda ()
	     (run-command "smartclose.sh")))

(xbindkey-function '(alt b:1)
     	   (lambda ()
	     (run-command "smartleft.sh")))

(xbindkey-function '(alt b:3)
     	   (lambda ()
	     (run-command "smartright.sh")))

(xbindkey-function '(b:6)
     	   (lambda ()
	     (run-command "sleep 0.02 && xte 'keydown Control_L' 'keydown Shift_L' 'key Tab' 'keyup Shift_L' 'keyup Control_L'  ")))

(xbindkey-function '(b:7)
     	   (lambda ()
	     (run-command "sleep 0.02 && xte 'keydown Control_L'  'key Tab'  'keyup Control_L'  ")))


;(xbindkey-function '(release alt h)
                ;(lambda ()
	     ;(run-command "smartleft.sh")))

;(xbindkey-function '(release alt l)
                ;(lambda ()
	     ;(run-command "smartright.sh")))

;(xbindkey-function '(release alt Left)
           ;(lambda ()
	     ;(run-command "smartleft.sh")))

;(xbindkey-function '(release alt Right)
                ;(lambda ()
	     ;(run-command "smartright.sh")))

(xbindkey-function '(release F1)
		(lambda ()
	     (run-command "smartclose.sh")))

;; (xbindkey-function '(release F2)
;;      	   (lambda ()
;; 	     (run-command "smartrefresh.sh")))

(xbindkey-function '(release F3)
     	   (lambda ()
	     (run-command "smartdispatch.sh searchword")))

(xbindkey-function '(release F4)
     	   (lambda ()
	     (run-command "smartdispatch.sh highlight")))

(xbindkey-function '(release c:163)
     	   (lambda ()
	     (run-command "xdotool key 'alt+h'")))

(xbindkey-function '(release c:180)
     	   (lambda ()
	     (run-command "xdotool key 'alt+l'")))
(xbindkey-function '(release c:172)
     	   (lambda ()
	     (run-command "xdotool key 'alt+shift+m'")))
(xbindkey-function '(release c:225)
     	   (lambda ()
	     (run-command "xdotool key 'F1'")))

(xbindkey-function '(release c:158)
     	   (lambda ()
	     (run-command "xdotool key 'ctrl+alt+m'")))

;; (xbindkey-function '(alt c:22)
;;      	   (lambda ()
;; 	     (run-command "xdotool key 'ctrl+shift+1'")))

;; (xbindkey-function '(release c:127)
;;      	   (lambda ()
;; 	     (run-command "xdotool key 'ctrl+shift+1'")))

;; (xbindkey-function '(alt c:22)
;;      	   (lambda ()
;; 	     (run-command "smartdispatch.sh smartrecent")))

;(xbindkey-function '(shift b:1)
                ;(lambda ()
	     ;(run-command "xte 'keyup Shift_L'; sleep 0.02 && xte 'keydown Control_L' 'key t' 'keyup Control_L'")))

 ;(xbindkey-function '(b:2)
		 ;(let ((time (current-time))
		       ;(count 0))
		   ;(lambda ()
                       ;(begin
			  ;(if (< (- (current-time) time) 1)
			      ;(run-command "st"))
			  ;(set! count 0))
		       ;(set! time (current-time)))))

 ;(xbindkey-function '(b:3)
		 ;(let ((time (current-time))
		       ;(count 0))
		   ;(lambda ()
		     ;(set! count (+ count 1))
		     ;(if (> count 1)
			 ;(begin
			  ;(if (< (- (current-time) time) 1)
			      ;(run-command "st"))
			  ;(set! count 0)))
		     ;(set! time (current-time)))))

;; Double click test
;; (xbindkey-function '(control w)
;;      	   (let ((count 0))
;;      	     (lambda ()
;;      	       (set! count (+ count 1))
;;      	       (if (> count 1)
;;      		   (begin
;;      		    (set! count 0)
;;      		    (run-command "xterm"))))))

;; (xbindkey-function '(c:50)
;;      	   (let ((count 0))
;;      	     (lambda ()
;;      	       (set! count (+ count 1))
;;      	       (if (> count 1)
;;      		   (begin
;;      		    (set! count 0)
;;      		    (display "Hello from double click!"))))))

;; Time double click test:
;;  - short double click -> run an xterm
;;  - long  double click -> run an rxvt
;; (xbindkey-function '(shift w)
;;      	   (let ((time (current-time))
;;      		 (count 0))
;;      	     (lambda ()
;;      	       (set! count (+ count 1))
;;      	       (if (> count 1)
;;      		   (begin
;;      		    (if (< (- (current-time) time) 1)
;;      			(run-command "xterm")
;;      			(run-command "rxvt"))
;;      		    (set! count 0)))
;;      	       (set! time (current-time)))))

; (xbindkey-function '(control v)
;      	   (let ((time (current-time))
;      		 (count 0))
;      	     (lambda ()

;      	       (set! count (+ count 1))
;      	       (if (eq? count 1)
;      	       	(run-command "copyq paste"))
;      	       (if (> count 1)
;      		   (begin
;      		    (if (< (- (current-time) time) 1)
;      			(run-command "copyq show")
;      			; (run-command "rxvt")
;      			)
;      		    (if (>= (- (current-time) time) 1)
;      		    	(run-command "copyq paste")
;      			)
;      		    (set! count 0)))
;      	       (set! time (current-time)))))

;; Chording keys test: Start differents program if only one key is
;; pressed or another if two keys are pressed.
;; If key1 is pressed start cmd-k1
;; If key2 is pressed start cmd-k2
;; If both are pressed start cmd-k1-k2 or cmd-k2-k1 following the
;;   release order
 (define (define-chord-keys key1 key2 cmd-k1 cmd-k2 cmd-k1-k2 cmd-k2-k1)
     "Define chording keys"
   (let ((k1 #f) (k2 #f))
     (xbindkey-function key1 (lambda () (set! k1 #t)))
     (xbindkey-function key2 (lambda () (set! k2 #t)))
     (xbindkey-function (cons 'release key1)
		     (lambda ()
		       (if (and k1 k2)
			   (run-command cmd-k1-k2)
			   (if k1 (run-command cmd-k1)))
		       (set! k1 #f) (set! k2 #f)))
     (xbindkey-function (cons 'release key2)
		     (lambda ()
		       (if (and k1 k2)
			   (run-command cmd-k2-k1)
			   (if k2 (run-command cmd-k2)))
		       (set! k1 #f) (set! k2 #f)))))


;; Example:
;;   Shift + b:1                   start an xterm
;;   Shift + b:3                   start an rxvt
;;   Shift + b:1 then Shift + b:3  start gv
;;   Shift + b:3 then Shift + b:1  start xpdf

;; (define-chord-keys '(shift "b:1") '(shift "b:3")
;;   "xterm" "rxvt" "gv" "xpdf")
  
 ;(define-chord-keys '("b:1" "b:3") '( shift "b:3")
   ;"st" "rxvt" "gv" "xpdf")

;; Here the release order have no importance
;; (the same program is started in both case)
;; (define-chord-keys '(alt "b:1") '(alt "b:3")
;;   "gv" "xpdf" "xterm" "xterm")


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; End of xbindkeys guile configuration ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
