TESTS = \
        ./testX.sh

TESTS_ENVIRONMENT = \
                    SYMLINK_TRACK="../symlink-track$(EXEEXT)"

testX.sh.log: X0

X0: X1
	$(AM_V_GEN)$(LN_S) $< $@
X1: X2
	$(AM_V_GEN)$(LN_S) $< $@
X2: X3
	$(AM_V_GEN)$(LN_S) $< $@
X3: X4
	$(AM_V_GEN)$(LN_S) $< $@
X4: X5
	$(AM_V_GEN)$(LN_S) $< $@
X5: X6
	$(AM_V_GEN)$(LN_S) $< $@
X6: X7
	$(AM_V_GEN)$(LN_S) $< $@
X7: X8
	$(AM_V_GEN)$(LN_S) $< $@
X8: X9
	$(AM_V_GEN)$(LN_S) $< $@
X9: X10
	$(AM_V_GEN)$(LN_S) $< $@
X10:
	$(AM_V_GEN)echo test > $@

EXTRA_DIST = \
             testX.sh

CLEANFILES = \
             X0 \
             X1 \
             X2 \
             X3 \
             X4 \
             X5 \
             X6 \
             X7 \
             X8 \
             X9 \
             X10
