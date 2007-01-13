
;; initialize portmidi lib
(pm:portmidi)
;; timer testing
(pt:Start )
(pt:Started)
(pt:Time)
(pt:Time)
(pt:Time)
;; device testing
(pm:CountDevices)
(pprint (pm:GetDeviceInfo ))
(defparameter inid (pm:GetDefaultInputDeviceID ))
(pm:GetDeviceInfo inid)
(defparameter outid (pm:GetDefaultOutputDeviceID ))
(pm:GetDeviceInfo outid)
;; output testing
(defparameter outid 3) ; 3 = my soundcanvas
(defparameter outdev (pm:OpenOutput outid 100 1000))
(pm:getDeviceInfo 3) ; :OPEN should be T
;; message tests
(defun pm (m &optional (s t))
  (format s "#<message :op ~2,'0x :ch ~2,'0d :data1 ~3,'0d :data2 ~3,'0d>"
          (ash (logand (pm:Message.status m) #xf0) -4)
          (logand (pm:Message.status m) #x0f)
          (pm:Message.data1 m)
          (pm:Message.data2 m)))
(defparameter on (pm:message #b10010000 60 64))
(pm on)
(pm:Message.status on)
(logand (ash (pm:Message.status on) -4) #x0f)
(pm:Message.data1 on)
(pm:Message.data2 on)
(pm:WriteShort outdev (+ (pm:time) 100) on)
(defparameter off (pm:message #b10000000 60 64))
(pm off)
(pm:WriteShort outdev (+ (pm:time) 100) off)
(pm:Close outdev)
;; event buffer testing
(defparameter buff (pm:EventBufferNew 8))
(loop for i below 8 for x = (pm:EventBufferElt buff i) 
   ;; set buffer events
   do
     (pm:Event.message x (pm:message #b1001000 (+ 60 i) (+ 100 i)))
     (pm:Event.timestamp x (* 1000 i)))
(loop for i below 8 for x = (pm:EventBufferElt buff i) 
   ;; check buffer contents
   collect (list (pm:Event.timestamp x)
                 (pm:Message.data1 (pm:Event.message x))
                 (pm:Message.data2 (pm:Event.message x))))
(pm:EventBufferFree buff)
;; input testing -- requires external midi keyboard
(pprint (pm:GetDeviceInfo ))
(defparameter inid 1) ; 1 = my external keyboard
(defparameter indev (pm:OpenInput inid 256)) 
(pm:GetDeviceInfo inid) ; :OPEN should be T
(pm:SetFilter indev pm:filt-realtime) ; ignore active sensing etc.
(pm:Poll indev)
;;
;; ...play midi keyboard, then ...
;;
(pm:Poll indev)
(defparameter buff (pm:EventBufferNew 32))
(defparameter num (pm:Read indev buff 32))
(pm:EventBufferMap (lambda (a b) b (terpri) (pm a))
                   buff num)
(pm:Poll indev)
(pm:EventBufferFree buff)



;;; recv testing



